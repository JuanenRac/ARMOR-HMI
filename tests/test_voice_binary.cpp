// ARMOR-NODE-HMI - host tests of the binary voice turn: the framing of the request, the reading of the multipart/mixed answer and the 43 cases every implementation shares.
// Run it with the folder of the shared vectors (ARMOR-COMMON/conformance) as the argument; without it only the checks that need no file are made.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../core/voice_multipart.hpp"
#include "check.hpp"

using namespace armor::voice;
namespace json = armor::json;

static bool read_file(const std::string& path, std::string& out) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return false;
  std::ostringstream text;
  text << file.rdbuf();
  out = text.str();
  return true;
}

static bool b64(const std::string& text, std::string& out) {
  out.clear();
  std::uint32_t buffer = 0;
  int bits = 0;
  for (const char c : text) {
    int v;
    if (c >= 'A' && c <= 'Z') v = c - 'A';
    else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
    else if (c >= '0' && c <= '9') v = c - '0' + 52;
    else if (c == '+') v = 62;
    else if (c == '/') v = 63;
    else if (c == '=') break;
    else return false;
    buffer = (buffer << 6) | static_cast<std::uint32_t>(v);
    bits += 6;
    if (bits >= 8) { bits -= 8; out += static_cast<char>((buffer >> bits) & 0xFF); }
  }
  return true;
}

static std::string audio_of(std::size_t bytes) {
  std::string out;
  for (std::size_t i = 0; i < bytes; ++i) out += static_cast<char>(i * 7 + 3);
  return out;
}

static void test_framing_round_trip() {
  RequestFraming framing;
  const std::string boundary = make_boundary(0x01234567u, 0x89abcdefu, 0xdeadbeefu);
  CHECK(boundary == "armor0123456789abcdefdeadbeef" && boundary.size() == 29);
  CHECK(build_request_framing(boundary, "pt", ReplyAudio::kBinary, framing));
  CHECK(framing.content_type == "multipart/form-data; boundary=" + boundary);
  const std::string audio = audio_of(3200);
  const std::string body = framing.head + audio + framing.tail;
  RequestRead read;
  Refusal why;
  CHECK(parse_request(framing.content_type, body, kMaxRequestAudioBytes, read, why));
  CHECK(read.meta.present && read.meta.language == "pt" && read.meta.reply_audio == ReplyAudio::kBinary);
  CHECK(read.audio == audio);
  // every language of the contract, and the other two ways of asking for the answer
  for (const char* language : {"en", "es", "de", "fr", "it", "ja", "zh", "pt"}) {
    for (const ReplyAudio mode : {ReplyAudio::kBase64, ReplyAudio::kBinary, ReplyAudio::kNone}) {
      CHECK(build_request_framing(boundary, language, mode, framing));
      CHECK(parse_request(framing.content_type, framing.head + audio + framing.tail, kMaxRequestAudioBytes, read, why) && read.meta.language == language && read.meta.reply_audio == mode);
    }
  }
  CHECK(!build_request_framing(boundary, "xx", ReplyAudio::kBinary, framing));        // a language the contract does not have
  CHECK(!build_request_framing("bad boundary", "en", ReplyAudio::kBinary, framing));  // a space is not allowed
  CHECK(!build_request_framing(std::string(71, 'b'), "en", ReplyAudio::kBinary, framing));
  // the largest recording the server takes by default
  CHECK(build_request_framing(boundary, "en", ReplyAudio::kBinary, framing));
  const std::string biggest = audio_of(kMaxRequestAudioBytes);
  CHECK(parse_request(framing.content_type, framing.head + biggest + framing.tail, kMaxRequestAudioBytes, read, why) && read.audio.size() == kMaxRequestAudioBytes);
  CHECK(!parse_request(framing.content_type, framing.head + audio_of(kMaxRequestAudioBytes + 2) + framing.tail, kMaxRequestAudioBytes, read, why) && why.status == 413);
}

static void test_boundary_and_audio_search() {
  const std::string audio = audio_of(1000);
  CHECK(!audio_holds(reinterpret_cast<const std::uint8_t*>(audio.data()), audio.size(), "--armor0123"));
  const std::string with = audio + "--armor0123" + audio;
  CHECK(audio_holds(reinterpret_cast<const std::uint8_t*>(with.data()), with.size(), "--armor0123"));
  CHECK(!audio_holds(reinterpret_cast<const std::uint8_t*>(with.data()), 3, "--armor0123"));   // shorter than the marker
  CHECK(!audio_holds(nullptr, 0, "x"));
  CHECK(make_boundary(0, 0, 0) == "armor000000000000000000000000");
  CHECK(make_boundary(0xFFFFFFFFu, 1, 2) == "armorffffffff0000000100000002");
}

static std::string mixed(const std::string& result_json, const std::string* audio, const std::string& boundary = "BND") {
  std::string out = "--" + boundary + "\r\nContent-Disposition: inline; name=\"result\"\r\nContent-Type: application/json\r\n\r\n" + result_json;
  if (audio != nullptr) out += "\r\n--" + boundary + "\r\nContent-Disposition: inline; name=\"audio\"\r\nContent-Type: audio/L16;rate=16000\r\n\r\n" + *audio;
  return out + "\r\n--" + boundary + "--\r\n";
}

static void test_reply() {
  BinaryAnswer answer;
  Refusal why;
  const std::string audio = audio_of(4000);
  CHECK(parse_reply("multipart/mixed; boundary=BND", mixed("{\"heard\":\"a\",\"reply\":\"b\"}", &audio), kMaxReplyAudioBytes, answer, why));
  CHECK(answer.heard == "a" && answer.reply == "b" && !answer.audio_omitted && answer.audio == audio);
  CHECK(parse_reply("multipart/mixed; boundary=BND", mixed("{\"heard\":\"\",\"reply\":\"b\"}", nullptr), kMaxReplyAudioBytes, answer, why) && answer.audio.empty());
  CHECK(parse_reply("multipart/mixed; boundary=BND", mixed("{\"heard\":\"x\",\"reply\":\"y\",\"audio_omitted\":\"too_large\"}", nullptr), kMaxReplyAudioBytes, answer, why) && answer.audio_omitted);
  CHECK(!parse_reply("multipart/mixed; boundary=BND", mixed("{\"heard\":\"x\",\"reply\":\"y\",\"audio_omitted\":\"other\"}", nullptr), kMaxReplyAudioBytes, answer, why) && why.code == "bad_meta");
  CHECK(!parse_reply("multipart/mixed; boundary=BND", mixed("{\"heard\":\"x\"}", nullptr), kMaxReplyAudioBytes, answer, why) && why.code == "bad_meta");
  CHECK(!parse_reply("multipart/mixed; boundary=BND", mixed("{\"heard\":1,\"reply\":\"y\"}", nullptr), kMaxReplyAudioBytes, answer, why));
  CHECK(!parse_reply("multipart/mixed; boundary=BND", mixed("{\"heard\":\"x\",\"reply\":\"y\",\"extra\":1}", nullptr), kMaxReplyAudioBytes, answer, why));
  const std::string odd = audio_of(3);
  CHECK(!parse_reply("multipart/mixed; boundary=BND", mixed("{\"heard\":\"x\",\"reply\":\"y\"}", &odd), kMaxReplyAudioBytes, answer, why) && why.status == 411);
  // JSON (the answer of a server that did not understand the request) is not a multipart answer
  CHECK(!parse_reply("application/json", "{\"heard\":\"x\",\"reply\":\"y\"}", kMaxReplyAudioBytes, answer, why) && why.code == "bad_multipart");
  // the words may hold anything a JSON string can, including the delimiter text and non-ASCII
  const std::string words = "{\"heard\":\"\\u00e1 --BND\",\"reply\":\"\xE3\x81\x82\"}";
  CHECK(parse_reply("multipart/mixed; boundary=BND", mixed(words, &audio), kMaxReplyAudioBytes, answer, why) && answer.reply == "\xE3\x81\x82");
}

static void test_never_crashes_on_damaged_bodies() {
  RequestFraming framing;
  const std::string boundary = make_boundary(1, 2, 3);
  CHECK(build_request_framing(boundary, "en", ReplyAudio::kBinary, framing));
  const std::string good = framing.head + audio_of(64) + framing.tail;
  RequestRead read;
  Refusal why;
  int accepted = 0;
  for (std::size_t cut = 0; cut + 2 < good.size(); ++cut) accepted += parse_request(framing.content_type, good.substr(0, cut), kMaxRequestAudioBytes, read, why) ? 1 : 0;
  CHECK(accepted == 0);   // a body cut anywhere before its closing delimiter is refused (the line end after it is optional)
  for (std::size_t at = 0; at < good.size(); ++at) {
    std::string bent = good;
    bent[at] = static_cast<char>(bent[at] ^ 0x55);
    parse_request(framing.content_type, bent, kMaxRequestAudioBytes, read, why);   // only that it returns
  }
  CHECK(true);
  BinaryAnswer answer;
  const std::string reply = mixed("{\"heard\":\"a\",\"reply\":\"b\"}", nullptr);
  for (std::size_t cut = 0; cut + 2 < reply.size(); ++cut) CHECK(!parse_reply("multipart/mixed; boundary=BND", reply.substr(0, cut), kMaxReplyAudioBytes, answer, why));
}

static void test_policy() {
  BinaryTurnPolicy p;
  CHECK(p.should_try(0));
  p.on_refused(1000);
  CHECK(!p.should_try(1000) && !p.should_try(1000 + BinaryTurnPolicy::kHoldMs - 1));   // not asked again for ten minutes
  CHECK(p.should_try(1000 + BinaryTurnPolicy::kHoldMs));                                // then it tries once more (the administrator may have turned it on)
  p.on_refused(5000000);
  p.on_accepted();
  CHECK(p.should_try(5000001));
  BinaryTurnPolicy off(false);
  CHECK(!off.should_try(0));                                                            // the panel's own setting wins
  off.set_allowed(true);
  CHECK(off.should_try(0));
  CHECK(BinaryTurnPolicy::refuses_binary(415) && BinaryTurnPolicy::refuses_binary(400) && BinaryTurnPolicy::refuses_binary(422));
  CHECK(!BinaryTurnPolicy::refuses_binary(200) && !BinaryTurnPolicy::refuses_binary(0) && !BinaryTurnPolicy::refuses_binary(401) && !BinaryTurnPolicy::refuses_binary(403) && !BinaryTurnPolicy::refuses_binary(500) && !BinaryTurnPolicy::refuses_binary(413));
  BinaryTurnPolicy q;                                                                   // a 400 or a 422 is remembered for the same ten minutes as a 415
  if (BinaryTurnPolicy::refuses_binary(422)) q.on_refused(2000);
  CHECK(!q.should_try(2001) && q.should_try(2000 + BinaryTurnPolicy::kHoldMs));
  // the clock wraps around after 49 days: the difference of two unsigned numbers is still the time that passed
  BinaryTurnPolicy w;
  w.on_refused(0xFFFFFF00u);
  CHECK(!w.should_try(0x100u) && w.should_try(0xFFFFFF00u + BinaryTurnPolicy::kHoldMs));
}

static int run_shared_cases(const std::string& dir) {
  std::string text;
  json::Value doc;
  if (!read_file(dir + "/voice/turn_multipart.json", text) || !json::parse(text, doc)) { std::printf("FAIL the shared cases cannot be read at %s\n", dir.c_str()); ++failures; return 0; }
  const json::Value* cases = doc.get("cases");
  CHECK(cases != nullptr && cases->is_array() && cases->items.size() == 43);
  if (cases == nullptr) return 0;
  int made = 0;
  for (const json::Value& item : cases->items) {
    const std::string name = item.string_or("name", "?");
    const std::string direction = item.string_or("direction", "");
    const std::string content_type = item.string_or("content_type", "");
    std::string body;
    CHECK(b64(item.string_or("body", ""), body));
    const json::Value* expect = item.get("expect");
    CHECK(expect != nullptr);
    if (expect == nullptr) continue;
    const json::Value* error = expect->get("error");
    const std::size_t limit = static_cast<std::size_t>(item.number_or("max_audio_bytes", direction == "request" ? static_cast<double>(kMaxRequestAudioBytes) : static_cast<double>(kMaxReplyAudioBytes)));
    Refusal why;
    bool ok;
    std::string want_audio;
    if (direction == "request") {
      RequestRead read;
      ok = parse_request(content_type, body, limit, read, why);
      if (ok) {
        const json::Value* meta = expect->get("meta");
        const bool have_meta = meta != nullptr && meta->is_object();
        CHECK(read.meta.present == have_meta);
        if (have_meta) {
          CHECK(read.meta.language == meta->string_or("language", "") && std::string(to_text(read.meta.reply_audio)) == meta->string_or("reply_audio", "base64"));
        }
        CHECK(b64(expect->string_or("audio", ""), want_audio) && std::string(read.audio) == want_audio);
      }
    } else {
      BinaryAnswer answer;
      ok = parse_reply(content_type, body, limit, answer, why);
      if (ok) {
        const json::Value* result = expect->get("result");
        CHECK(result != nullptr && answer.heard == result->string_or("heard", "!") && answer.reply == result->string_or("reply", "!") && answer.audio_omitted == (result->get("audio_omitted") != nullptr));
        const json::Value* audio = expect->get("audio");
        if (audio != nullptr && audio->is_string()) { CHECK(b64(audio->text, want_audio) && std::string(answer.audio) == want_audio); }
        else CHECK(answer.audio.empty());
      }
    }
    if (error != nullptr) {
      CHECK(!ok);
      if (ok) std::printf("  case accepted that must be refused: %s\n", name.c_str());
      else if (why.status != static_cast<int>(error->number_or("status", -1)) || why.code != error->string_or("code", "?")) {
        std::printf("  case %s: %d %s, wanted %d %s\n", name.c_str(), why.status, why.code.c_str(), static_cast<int>(error->number_or("status", -1)), error->string_or("code", "?").c_str());
        CHECK(false);
      }
    } else {
      CHECK(ok);
      if (!ok) std::printf("  case refused that must be accepted: %s (%d %s)\n", name.c_str(), why.status, why.code.c_str());
    }
    ++made;
  }
  return made;
}

int main(int argc, char** argv) {
  test_framing_round_trip();
  test_boundary_and_audio_search();
  test_reply();
  test_never_crashes_on_damaged_bodies();
  test_policy();
  if (argc > 1) {
    const int made = run_shared_cases(argv[1]);
    CHECK(made == 43);
  }
  FINISH("voice binary");
}
