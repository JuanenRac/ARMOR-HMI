// A.R.M.O.R. - how the panel trusts the server it talks to over HTTPS: a certificate fingerprint fixed when the panel is set up. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// There is no "accept any certificate" any more. Over http:// on the house network nothing is checked (that is how the server runs by default and it is said so in the docs). Over
// https:// the connection is accepted only when the server presents the certificate whose SHA-256 fingerprint is stored in the settings (a server with its own certificate), or, with
// no fingerprint, when the certificate is vouched for by the usual certificate authorities (a server with a public name). A panel with https:// and a certificate nobody vouches for and
// nobody pinned simply does not connect: it never falls back to not looking. The connection itself is main/server_link.cpp.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace armor::server_pin {

constexpr std::size_t kDigestBytes = 32;
constexpr std::size_t kHexLength = 64;

enum class Trust {
  kNone,          // http://: nothing to check (the house network)
  kFingerprint,   // https:// and the stored fingerprint must match the certificate presented
  kChain,         // https:// and the certificate chain must end at the stored certificate (PEM)
  kAuthorities,   // https:// and the usual certificate authorities must vouch for the certificate
};

// What a link with these settings does. There is no value that means "https without looking".
inline Trust trust_for(bool tls, std::string_view fingerprint, std::string_view cert_pem) {
  if (!tls) return Trust::kNone;
  if (!fingerprint.empty()) return Trust::kFingerprint;
  if (!cert_pem.empty()) return Trust::kChain;
  return Trust::kAuthorities;
}

inline bool verifies(Trust trust) { return trust != Trust::kNone; }

inline int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

// What a person typed or pasted ("AB:CD:..", "ab cd ..", "ABCD..") made the way it is stored: 64 lowercase hex digits. Anything else gives "" (the caller says "invalid").
inline std::string normalize(std::string_view text) {
  std::string out;
  out.reserve(kHexLength);
  for (const char c : text) {
    if (c == ':' || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '-') continue;
    const int v = hex_value(c);
    if (v < 0) return "";
    out.push_back(static_cast<char>(v < 10 ? '0' + v : 'a' + (v - 10)));
    if (out.size() > kHexLength) return "";
  }
  return out.size() == kHexLength ? out : std::string();
}

inline bool is_valid(std::string_view stored) {
  if (stored.size() != kHexLength) return false;
  for (const char c : stored) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  return true;
}

inline std::string to_hex(const std::uint8_t* digest, std::size_t length) {
  static const char kDigits[] = "0123456789abcdef";
  std::string out;
  out.reserve(length * 2);
  for (std::size_t i = 0; i < length; ++i) { out.push_back(kDigits[digest[i] >> 4]); out.push_back(kDigits[digest[i] & 15]); }
  return out;
}

// True when the SHA-256 digest of the certificate presented is the stored fingerprint. The comparison takes the same time wherever the first difference is.
inline bool matches(const std::uint8_t* digest, std::string_view stored) {
  if (digest == nullptr || !is_valid(stored)) return false;
  unsigned diff = 0;
  for (std::size_t i = 0; i < kDigestBytes; ++i) {
    const int high = hex_value(stored[i * 2]);
    const int low = hex_value(stored[i * 2 + 1]);
    diff |= static_cast<unsigned>(digest[i] ^ static_cast<std::uint8_t>((high << 4) | low));
  }
  return diff == 0;
}

}  // namespace armor::server_pin
