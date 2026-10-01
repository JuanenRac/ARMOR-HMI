// ARMOR-HMI - host tests of what the panel knows of the server: the summary, the state of the link, what is new and the colour of the screen.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <string>
#include <vector>

#include "../core/server_view.hpp"
#include "check.hpp"

using namespace armor::hmi;

static const char* kSummary =
    "{\"mode\":\"armed\",\"revision\":42,\"time_ms\":1790000000000,\"nodes\":{\"online\":3,\"total\":4},"
    "\"alarms\":{\"active\":2,\"unacknowledged\":1,\"items\":["
    "{\"id\":\"a1\",\"code\":\"intrusion\",\"severity\":\"critical\",\"source_type\":\"node\",\"source_id\":\"radar-1\",\"raised_at\":\"2026-10-01T10:00:00Z\",\"acknowledged\":false},"
    "{\"id\":\"a2\",\"code\":\"low_battery\",\"severity\":\"warning\",\"source_type\":\"device\",\"source_id\":\"d9\",\"raised_at\":\"2026-10-01T09:00:00Z\",\"acknowledged\":true}]}}";

static void test_summary() {
  Summary s;
  CHECK(parse_summary(kSummary, s));
  CHECK(s.mode == Mode::kArmed && s.revision == 42 && s.nodes_online == 3 && s.nodes_total == 4 && s.alarms_active == 2 && s.alarms_unacknowledged == 1);
  CHECK(s.alarms.size() == 2 && s.alarms[0].code == "intrusion" && s.alarms[0].severity == Severity::kCritical && !s.alarms[0].acknowledged && s.alarms[1].acknowledged);
  CHECK(std::string(to_text(s.mode)) == "armed");
  // what is not the server's answer is refused and leaves the old summary alone
  Summary kept = s;
  for (const char* bad : {"", "[]", "{}", "{\"mode\":\"maybe\",\"nodes\":{},\"alarms\":{}}", "{\"mode\":\"armed\"}", "{\"mode\":\"armed\",\"nodes\":[],\"alarms\":{}}", "not json"}) CHECK(!parse_summary(bad, kept));
  CHECK(kept.revision == 42);
  // at most six alarms, and an alarm without an id or a code is skipped
  std::string many = "{\"mode\":\"disarmed\",\"nodes\":{\"online\":0,\"total\":0},\"alarms\":{\"active\":9,\"unacknowledged\":9,\"items\":[";
  for (int i = 0; i < 9; ++i) many += std::string(i ? "," : "") + "{\"id\":\"x" + std::to_string(i) + "\",\"code\":\"motion\"}";
  many += ",{\"code\":\"motion\"},{\"id\":\"y\"}]}}";
  Summary big;
  CHECK(parse_summary(many, big) && big.alarms.size() == 6 && big.alarms_active == 9 && big.mode == Mode::kDisarmed);
}

static void test_link() {
  LinkTracker link(3);
  CHECK(link.link() == Link::kNotConfigured);
  link.configure(true, 3);
  link.set_network(false);
  link.update(1000);
  CHECK(link.link() == Link::kNoNetwork);
  link.set_network(true);
  link.update(2000);
  CHECK(link.link() == Link::kConnecting);
  Summary s;
  CHECK(parse_summary(kSummary, s));
  link.on_summary(3000, s);
  CHECK(link.link() == Link::kOnline && link.has_summary() && link.next_delay_ms() == 3000);
  link.update(3000 + 4 * 3000);
  CHECK(link.link() == Link::kOnline);          // four polls without an answer is still within the limit
  link.update(3000 + 4 * 3000 + 1);
  CHECK(link.link() == Link::kStale);           // one more and what is shown is marked old
  link.on_summary(20000, s);
  CHECK(link.link() == Link::kOnline);
  // the delay between tries grows with the failures and stops at a minute
  link.on_failure(21000, Failure::kNoReply);
  CHECK(link.next_delay_ms() == 3000);
  link.on_failure(24000, Failure::kNoReply);
  CHECK(link.next_delay_ms() == 6000);
  for (int i = 0; i < 20; ++i) link.on_failure(30000 + i * 1000, Failure::kNoReply);
  CHECK(link.next_delay_ms() == 60000);
}

static void test_link_denied() {
  LinkTracker link(3);
  link.configure(true, 3);
  link.set_network(true);
  link.on_signed_in(1000);
  CHECK(link.signed_in());
  link.on_failure(2000, Failure::kUnauthorized);   // the session ended: sign in again
  CHECK(!link.signed_in() && link.link() != Link::kDenied);
  link.on_failure(3000, Failure::kUnauthorized);   // refused twice in a row: the login itself is wrong
  CHECK(link.link() == Link::kDenied);
  link.update(4000);
  CHECK(link.link() == Link::kDenied);             // it stays denied until the settings change
  link.configure(true, 3);
  LinkTracker forbidden(3);
  forbidden.configure(true, 3);
  forbidden.set_network(true);
  forbidden.on_failure(1000, Failure::kForbidden);
  CHECK(forbidden.link() == Link::kDenied);
  // a login that works clears the count of refusals
  LinkTracker good(3);
  good.configure(true, 3); good.set_network(true);
  good.on_failure(1000, Failure::kUnauthorized);
  good.on_login_accepted();
  good.on_failure(2000, Failure::kUnauthorized);
  CHECK(good.link() != Link::kDenied);
  // switching the link off shows "not set up"
  good.configure(false, 3);
  CHECK(good.link() == Link::kNotConfigured);
}

static void test_new_alarms_and_tone() {
  Summary s;
  CHECK(parse_summary(kSummary, s));
  // the first summary after a start only teaches: nothing rings for old alarms
  CHECK(new_unacknowledged({}, true, s).empty());
  const std::vector<std::string> known = ids_of(s);
  CHECK(known.size() == 2);
  CHECK(new_unacknowledged(known, false, s).empty());
  Summary later = s;
  AlarmLine fresh;
  fresh.id = "a3"; fresh.code = "smoke"; fresh.severity = Severity::kHigh;
  later.alarms.insert(later.alarms.begin(), fresh);
  const std::vector<AlarmLine> rings = new_unacknowledged(known, false, later);
  CHECK(rings.size() == 1 && rings[0].id == "a3");
  later.alarms[0].acknowledged = true;           // one that is already acknowledged does not ring
  CHECK(new_unacknowledged(known, false, later).empty());
  // the colour
  CHECK(tone_of(s) == Tone::kAlert);             // an unacknowledged critical alarm
  Summary calm;
  CHECK(tone_of(calm) == Tone::kCalm);
  Summary review;
  AlarmLine light; light.id = "b"; light.code = "motion"; light.severity = Severity::kWarning;
  review.alarms.push_back(light);
  CHECK(tone_of(review) == Tone::kReview);
  review.alarms[0].acknowledged = true;
  CHECK(tone_of(review) == Tone::kCalm);
  review.alarms_unacknowledged = 7;              // more than the six that were sent
  CHECK(tone_of(review) == Tone::kReview);
}

static void test_night_brightness() {
  CHECK(effective_brightness(12, 80, 15, 22, 7) == 80);
  CHECK(effective_brightness(22, 80, 15, 22, 7) == 15 && effective_brightness(23, 80, 15, 22, 7) == 15 && effective_brightness(3, 80, 15, 22, 7) == 15);
  CHECK(effective_brightness(7, 80, 15, 22, 7) == 80 && effective_brightness(21, 80, 15, 22, 7) == 80);
  CHECK(effective_brightness(13, 80, 15, 12, 14) == 15 && effective_brightness(14, 80, 15, 12, 14) == 80);   // a night that does not cross midnight
  CHECK(effective_brightness(3, 80, 15, 5, 5) == 80);                                                         // an empty night
}

int main() {
  test_summary();
  test_link();
  test_link_denied();
  test_new_alarms_and_tone();
  test_night_brightness();
  FINISH("test_view");
}
