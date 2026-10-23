// ARMOR-HMI - the binary voice turn: the multipart request that carries the audio as raw bytes and the multipart/mixed answer, as the shared contract describes them.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The older request (the whole body is the audio, the answer is JSON with the spoken reply as base64) keeps working and is what the panel falls back to. The binary form saves the third of
// the answer that base64 adds and the decoding on a small chip. The reader here is as strict as the reference reader of the contract (exactly the parts of the contract, each once, the
// headers it needs and nothing else, sizes checked before anything is copied) and is tested on a computer with the cases every implementation shares. Nothing here touches the network.
//
// Everything that is read is a view into the body the caller holds: nothing is copied, so a 200 KB spoken answer is not duplicated in memory.
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "json.hpp"

namespace armor::voice {

constexpr std::size_t kMaxMetaBytes = 1024;
constexpr std::size_t kMaxResultBytes = 8192;
constexpr std::size_t kMaxPartHeaderBytes = 512;
constexpr std::size_t kMaxBoundaryChars = 70;
constexpr std::size_t kMaxRequestAudioBytes = 480000;   // 15 seconds of 16 kHz 16-bit mono: what the server takes unless an administrator raised it
constexpr std::size_t kMaxReplyAudioBytes = 200000;     // the spoken answer the server hands on; above it the answer is text only

// Why a request or an answer is refused: the status and the short word of the contract.
struct Refusal {
  int status = 0;
  std::string code;
};

enum class Framing { kFormData, kMixed };

struct Part {
  std::string name;
  std::string content_type;
  std::string_view data;   // inside the body that was read
};

namespace detail {

inline bool refuse(Refusal& why, int status, const char* code) {
  why.status = status;
  why.code = code;
  return false;
}

inline bool boundary_is_valid(std::string_view boundary) {
  if (boundary.empty() || boundary.size() > kMaxBoundaryChars || boundary.back() == '.') return false;
  for (const char c : boundary) {
    const bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || std::string_view("'()+_,-./:=?").find(c) != std::string_view::npos;
    if (!ok) return false;
  }
  return true;
}

inline std::string_view trim(std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
  return text;
}

inline std::string lower(std::string_view text) {
  std::string out(text);
  for (char& c : out) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  return out;
}

inline bool name_is_valid(std::string_view name) {
  if (name.empty() || name.size() > 16) return false;
  for (const char c : name) if (c < 'a' || c > 'z') return false;
  return true;
}

// `Content-Disposition` of a part: form-data; name="x" (a file name is tolerated and ignored) or inline/attachment; name="x". False when it is anything else.
inline bool read_disposition(std::string_view value, Framing framing, std::string& name) {
  value = trim(value);
  const std::size_t semicolon = value.find(';');
  if (semicolon == std::string_view::npos) return false;
  const std::string kind = lower(trim(value.substr(0, semicolon)));
  if (framing == Framing::kFormData ? kind != "form-data" : (kind != "inline" && kind != "attachment")) return false;
  std::string_view rest = trim(value.substr(semicolon + 1));
  if (rest.substr(0, 5) != "name=") {
    // `name` may have spaces around the equals sign in the reference reader's pattern
    std::size_t at = 0;
    if (rest.substr(0, 4) != "name") return false;
    at = 4;
    while (at < rest.size() && (rest[at] == ' ' || rest[at] == '\t')) ++at;
    if (at >= rest.size() || rest[at] != '=') return false;
    ++at;
    while (at < rest.size() && (rest[at] == ' ' || rest[at] == '\t')) ++at;
    rest = rest.substr(at);
  } else {
    rest.remove_prefix(5);
  }
  if (rest.empty() || rest.front() != '"') return false;
  const std::size_t close = rest.find('"', 1);
  if (close == std::string_view::npos) return false;
  const std::string_view found = rest.substr(1, close - 1);
  if (!name_is_valid(found)) return false;
  std::string_view tail = trim(rest.substr(close + 1));
  if (!tail.empty()) {
    if (framing != Framing::kFormData || tail.front() != ';') return false;
    tail = trim(tail.substr(1));
    if (tail.substr(0, 9) != "filename=") return false;
    tail.remove_prefix(9);
    if (tail.size() < 2 || tail.front() != '"') return false;
    const std::size_t end = tail.find('"', 1);
    if (end == std::string_view::npos || trim(tail.substr(end + 1)).size() != 0) return false;
    const std::string_view file = tail.substr(1, end - 1);
    if (file.size() > 64 || file.find('\r') != std::string_view::npos || file.find('\n') != std::string_view::npos) return false;
  }
  name.assign(found);
  return true;
}

}  // namespace detail

// The boundary of a `Content-Type` that has to be multipart/<form-data or mixed>. False with bad_multipart for anything else (a second boundary included).
inline bool boundary_of(std::string_view content_type, Framing framing, std::string& boundary, Refusal& why) {
  const char* wanted = framing == Framing::kFormData ? "multipart/form-data" : "multipart/mixed";
  std::size_t at = 0;
  bool first = true, have = false;
  std::string found;
  while (at <= content_type.size()) {
    std::size_t end = content_type.find(';', at);
    if (end == std::string_view::npos) end = content_type.size();
    const std::string_view item = detail::trim(content_type.substr(at, end - at));
    if (first) {
      if (detail::lower(item) != wanted) return detail::refuse(why, 400, "bad_multipart");
      first = false;
    } else {
      const std::size_t equals = item.find('=');
      const std::string_view key = detail::trim(equals == std::string_view::npos ? item : item.substr(0, equals));
      if (detail::lower(key) == "boundary") {
        if (have) return detail::refuse(why, 400, "bad_multipart");
        have = true;
        std::string_view value = detail::trim(equals == std::string_view::npos ? std::string_view() : item.substr(equals + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') value = value.substr(1, value.size() - 2);
        found.assign(value);
      }
    }
    if (end == content_type.size()) break;
    at = end + 1;
  }
  if (first || !detail::boundary_is_valid(found)) return detail::refuse(why, 400, "bad_multipart");
  boundary = found;
  return true;
}

// Cuts a body into its parts. `allowed` are the part names of the contract; `max_audio` and `max_json` the sizes of the part `audio` and of the JSON part (`meta` or `result`).
inline bool split_parts(std::string_view body, std::string_view boundary, Framing framing, const std::vector<std::string>& allowed, std::size_t max_audio, std::size_t max_json,
                        std::vector<Part>& parts, Refusal& why) {
  parts.clear();
  const std::string opening = "--" + std::string(boundary);
  const std::string delimiter = "\r\n" + opening;
  if (body.substr(0, opening.size() + 2) != opening + "\r\n") return detail::refuse(why, 400, "bad_multipart");
  std::size_t position = opening.size() + 2;
  for (;;) {
    // the headers end at the first empty line; an empty block of headers is the empty line right at the start
    const std::size_t header_end = body.find("\r\n\r\n", position - 2);
    if (header_end == std::string_view::npos || (header_end >= position ? header_end - position : 0) > kMaxPartHeaderBytes) return detail::refuse(why, 400, "bad_multipart");
    const std::string_view block = header_end >= position ? body.substr(position, header_end - position) : std::string_view();
    const std::size_t data_start = header_end + 4;
    const std::size_t end = body.find(delimiter, data_start);
    if (end == std::string_view::npos) return detail::refuse(why, 400, "bad_multipart");
    Part part;
    bool have_name = false, have_type = false;
    std::size_t line_at = 0;
    while (!block.empty() && line_at <= block.size()) {
      std::size_t line_end = block.find("\r\n", line_at);
      if (line_end == std::string_view::npos) line_end = block.size();
      const std::string_view line = block.substr(line_at, line_end - line_at);
      for (const char c : line) if (static_cast<unsigned char>(c) > 0x7F) return detail::refuse(why, 400, "bad_multipart");
      const std::size_t colon = line.find(':');
      if (colon == std::string_view::npos) return detail::refuse(why, 400, "bad_multipart");
      const std::string key = detail::lower(detail::trim(line.substr(0, colon)));
      const std::string_view value = line.substr(colon + 1);
      if (key == "content-type") {
        if (have_type) return detail::refuse(why, 400, "bad_multipart");
        have_type = true;
        part.content_type.assign(detail::trim(value));
      } else if (key == "content-disposition") {
        if (have_name || !detail::read_disposition(value, framing, part.name)) return detail::refuse(why, 400, "bad_multipart");
        have_name = true;
      } else {
        return detail::refuse(why, 400, "bad_multipart");
      }
      if (line_end >= block.size()) break;
      line_at = line_end + 2;
    }
    if (!have_name) return detail::refuse(why, 400, "bad_multipart");
    if (std::find(allowed.begin(), allowed.end(), part.name) == allowed.end()) return detail::refuse(why, 422, "unknown_part");
    for (const Part& seen : parts) if (seen.name == part.name) return detail::refuse(why, 422, "duplicate_part");
    part.data = body.substr(data_start, end - data_start);
    if (part.data.size() > (part.name == "audio" ? max_audio : max_json)) return part.name == "audio" ? detail::refuse(why, 413, "audio_too_large") : detail::refuse(why, 422, "bad_meta");
    parts.push_back(std::move(part));
    position = end + delimiter.size();
    const std::string_view tail = body.substr(position, 2);
    if (tail == "--") {
      const std::string_view after = body.substr(position + 2);
      if (!after.empty() && after != "\r\n") return detail::refuse(why, 400, "bad_multipart");
      return true;
    }
    if (tail != "\r\n") return detail::refuse(why, 400, "bad_multipart");
    position += 2;
  }
}

inline bool audio_type_is_ok(std::string_view content_type) {
  std::string normal;
  for (const char c : content_type) if (c != ' ') normal += c;
  normal = detail::lower(normal);
  return normal == "audio/l16;rate=16000" || normal == "application/octet-stream";
}

inline const Part* find_part(const std::vector<Part>& parts, std::string_view name) {
  for (const Part& part : parts) if (part.name == name) return &part;
  return nullptr;
}

// ---- the request ----------------------------------------------------------------------------------------------------------------------

enum class ReplyAudio { kBase64, kBinary, kNone };

struct Meta {
  std::string language = "en";
  ReplyAudio reply_audio = ReplyAudio::kBase64;
  bool present = false;
};

inline bool language_is_in_contract(std::string_view code) {
  for (const char* known : {"en", "es", "de", "fr", "it", "ja", "zh", "pt"}) if (code == known) return true;
  return false;
}

// The `meta` part of a request: language, encoding, sample_rate, channels and optionally reply_audio, and nothing else.
inline bool read_meta(std::string_view text, Meta& out) {
  json::Value document;
  if (!json::parse(text, document) || !document.is_object()) return false;
  for (const std::string& name : document.names) {
    if (name != "language" && name != "encoding" && name != "sample_rate" && name != "channels" && name != "reply_audio") return false;
  }
  const json::Value* language = document.get("language");
  const json::Value* encoding = document.get("encoding");
  const json::Value* rate = document.get("sample_rate");
  const json::Value* channels = document.get("channels");
  if (language == nullptr || !language->is_string() || !language_is_in_contract(language->text)) return false;
  if (encoding == nullptr || !encoding->is_string() || encoding->text != "pcm_s16le") return false;
  if (rate == nullptr || !rate->is_number() || rate->number != 16000.0) return false;
  if (channels == nullptr || !channels->is_number() || channels->number != 1.0) return false;
  Meta meta;
  meta.present = true;
  meta.language = language->text;
  if (const json::Value* reply = document.get("reply_audio"); reply != nullptr) {
    if (!reply->is_string()) return false;
    if (reply->text == "base64") meta.reply_audio = ReplyAudio::kBase64;
    else if (reply->text == "binary") meta.reply_audio = ReplyAudio::kBinary;
    else if (reply->text == "none") meta.reply_audio = ReplyAudio::kNone;
    else return false;
  }
  out = std::move(meta);
  return true;
}

struct RequestRead {
  Meta meta;
  std::string_view audio;
};

// The reference reading of a binary request (the server reads the same way). The panel only sends one; this is how its framing is checked against the shared cases.
inline bool parse_request(std::string_view content_type, std::string_view body, std::size_t max_audio, RequestRead& out, Refusal& why) {
  std::string boundary;
  if (!boundary_of(content_type, Framing::kFormData, boundary, why)) return false;
  if (body.size() > max_audio + kMaxMetaBytes + 4096) return detail::refuse(why, 413, "audio_too_large");
  std::vector<Part> parts;
  if (!split_parts(body, boundary, Framing::kFormData, {"meta", "audio"}, max_audio, kMaxMetaBytes, parts, why)) return false;
  const Part* audio = find_part(parts, "audio");
  if (audio == nullptr) return detail::refuse(why, 422, "missing_audio");
  if (!audio->content_type.empty() && !audio_type_is_ok(audio->content_type)) return detail::refuse(why, 415, "unsupported_audio_type");
  if (audio->data.empty() || audio->data.size() % 2 != 0) return detail::refuse(why, 411, "bad audio length");
  RequestRead read;
  if (const Part* meta = find_part(parts, "meta"); meta != nullptr) {
    if (!meta->content_type.empty() && detail::lower(detail::trim(meta->content_type.substr(0, meta->content_type.find(';')))) != "application/json") return detail::refuse(why, 422, "bad_meta");
    if (!read_meta(meta->data, read.meta)) return detail::refuse(why, 422, "bad_meta");
  }
  read.audio = audio->data;
  out = std::move(read);
  return true;
}

// What the panel sends around the audio: `head` goes first, then the audio bytes as they are, then `tail`. The audio is never copied into the body (it can be 480 KB on a chip with a
// few hundred kilobytes of fast memory).
struct RequestFraming {
  std::string content_type;
  std::string head;
  std::string tail;
};

// A boundary the audio does not contain: "armor" and 24 hex digits made of the three numbers the caller drew at random. False when the audio holds the boundary (draw again).
inline std::string make_boundary(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
  static const char digits[] = "0123456789abcdef";
  std::string out = "armor";
  for (const std::uint32_t word : {a, b, c}) for (int shift = 28; shift >= 0; shift -= 4) out += digits[(word >> shift) & 0xF];
  return out;
}

inline bool audio_holds(const std::uint8_t* audio, std::size_t size, std::string_view marker) {
  if (marker.empty() || size < marker.size()) return false;
  const std::uint8_t* end = audio + size;
  return std::search(audio, end, marker.begin(), marker.end(), [](std::uint8_t byte, char c) { return byte == static_cast<std::uint8_t>(c); }) != end;
}

inline const char* to_text(ReplyAudio mode) { return mode == ReplyAudio::kBinary ? "binary" : mode == ReplyAudio::kNone ? "none" : "base64"; }

// The framing of a request that asks for the spoken answer as `reply_audio`. `language` must be one of the contract's.
inline bool build_request_framing(const std::string& boundary, std::string_view language, ReplyAudio reply_audio, RequestFraming& out) {
  if (!detail::boundary_is_valid(boundary) || !language_is_in_contract(language)) return false;
  json::Writer meta;
  meta.begin_object().field("language", std::string(language)).field("encoding", "pcm_s16le").field("sample_rate", 16000).field("channels", 1).field("reply_audio", to_text(reply_audio)).end_object();
  const std::string marker = "--" + boundary;
  out.content_type = "multipart/form-data; boundary=" + boundary;
  out.head = marker + "\r\nContent-Disposition: form-data; name=\"meta\"\r\nContent-Type: application/json\r\n\r\n" + meta.str() + "\r\n" + marker +
             "\r\nContent-Disposition: form-data; name=\"audio\"\r\nContent-Type: audio/L16;rate=16000\r\n\r\n";
  out.tail = "\r\n" + marker + "--\r\n";
  return true;
}

// ---- the answer -----------------------------------------------------------------------------------------------------------------------

struct BinaryAnswer {
  std::string heard, reply;
  bool audio_omitted = false;       // the server left the spoken answer out because it was too long: the words are still there
  std::string_view audio;           // raw 16-bit PCM, empty when the answer is text only
};

// The `multipart/mixed` answer of a request that asked for `reply_audio` binary: the part `result` (JSON without an `audio` field) and, when there is one, the part `audio`.
inline bool parse_reply(std::string_view content_type, std::string_view body, std::size_t max_audio, BinaryAnswer& out, Refusal& why) {
  std::string boundary;
  if (!boundary_of(content_type, Framing::kMixed, boundary, why)) return false;
  std::vector<Part> parts;
  if (!split_parts(body, boundary, Framing::kMixed, {"result", "audio"}, max_audio, kMaxResultBytes, parts, why)) return false;
  const Part* result = find_part(parts, "result");
  if (result == nullptr) return detail::refuse(why, 422, "bad_meta");
  json::Value document;
  if (!json::parse(result->data, document) || !document.is_object() || document.get("audio") != nullptr) return detail::refuse(why, 422, "bad_meta");
  for (const std::string& name : document.names) if (name != "heard" && name != "reply" && name != "audio_omitted") return detail::refuse(why, 422, "bad_meta");
  const json::Value* heard = document.get("heard");
  const json::Value* reply = document.get("reply");
  if (heard == nullptr || !heard->is_string() || reply == nullptr || !reply->is_string()) return detail::refuse(why, 422, "bad_meta");
  BinaryAnswer answer;
  answer.heard = heard->text;
  answer.reply = reply->text;
  if (const json::Value* omitted = document.get("audio_omitted"); omitted != nullptr) {
    if (!omitted->is_string() || omitted->text != "too_large") return detail::refuse(why, 422, "bad_meta");
    answer.audio_omitted = true;
  }
  if (const Part* audio = find_part(parts, "audio"); audio != nullptr) {
    if (audio->data.empty() || audio->data.size() % 2 != 0) return detail::refuse(why, 411, "bad audio length");
    answer.audio = audio->data;
  }
  out = std::move(answer);
  return true;
}

// ---- when to ask for the binary form ----------------------------------------------------------------------------------------------------

// The server (or the speech service the panel is pointed at) may not take the binary request: the administrator turned it off, or the address is a service that never learned it. It
// answers 415 (or, for a speech service that reads a request it does not know as malformed, 400 or 422). The panel then sends the same recording the older way and does not try the binary form again for a while, so the next turn does not cost a refused request.
class BinaryTurnPolicy {
 public:
  static constexpr std::uint32_t kHoldMs = 10 * 60 * 1000;

  explicit BinaryTurnPolicy(bool allowed = true) : allowed_(allowed) {}
  void set_allowed(bool allowed) { allowed_ = allowed; if (!allowed) refused_ = false; }
  bool should_try(std::uint32_t now_ms) const { return allowed_ && !(refused_ && now_ms - refused_at_ms_ < kHoldMs); }
  void on_refused(std::uint32_t now_ms) { refused_ = true; refused_at_ms_ = now_ms; }
  void on_accepted() { refused_ = false; }

  // Whether an answer means "I do not take the binary request": 415, and also 400 or 422, which is what a service of another make answers to a multipart body it does not know.
  // The recording is sent again the older way, so a request that was refused for another reason costs one more request and nothing else.
  static bool refuses_binary(int status) { return status == 415 || status == 400 || status == 422; }

 private:
  bool allowed_;
  bool refused_ = false;
  std::uint32_t refused_at_ms_ = 0;
};

}  // namespace armor::voice
