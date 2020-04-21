#include <gtest/gtest.h>
#include <algorithm>
#include "profiler.h"

struct FastForwardableProfiler {
  Profiler profiler;
  TimePoint now;
  FastForwardableProfiler(const TimePoint& start = TimePoint())
    : profiler(start), now(start) {}
  void fastForward(double secs) {
    now += secs;
  }
  void event(const std::string& event) {
    profiler.event(event, now);
  }
};

class ProfilerTest : public ::testing::Test {
protected:
  FastForwardableProfiler profiler;
  ProfilerTest() {
  }
  ~ProfilerTest() override {}
  void SetUp() override {}
  void TearDown() override {
  }
  void fastForward(double secs) {
    profiler.fastForward(secs);
  }
  void event(const std::string& event) {
    profiler.event(event);
  }
  std::vector<std::pair<std::string, double>> events() {
    rapidjson::Document doc;
    auto events = profiler.profiler.json(doc.GetAllocator());
    std::vector<std::pair<std::string, double>> event_list;
    for ( auto& event : events.GetArray() ) {
      auto pair = event.GetArray();
      event_list.push_back(std::make_pair(pair[0].GetString(),
            pair[1].GetDouble()));
    }
    return event_list;
  }
};

TEST_F(ProfilerTest, profile) {
  EXPECT_TRUE(events().empty());
  event("start");
  fastForward(1.0);
  event("end");
  auto ev = events();
  ASSERT_EQ(ev.size(), 2);
  EXPECT_STREQ(ev[0].first.c_str(), "start");
  EXPECT_NEAR(ev[0].second, 0.0, 0.0001);
  EXPECT_STREQ(ev[1].first.c_str(), "end");
  EXPECT_NEAR(ev[1].second, 1.0, 0.0001);
}

TEST_F(ProfilerTest, combine) {
  EXPECT_TRUE(events().empty());
  FastForwardableProfiler ffp(profiler.now + 0.5);
  event("start");
  fastForward(1.0);
  event("end");
  ffp.event("start2");
  ffp.fastForward(1.0);
  ffp.event("end2");
  profiler.profiler.join(ffp.profiler, "ffp_");
  auto ev = events();
  ASSERT_EQ(ev.size(), 4);
  EXPECT_STREQ(ev[0].first.c_str(), "start");
  EXPECT_NEAR(ev[0].second, 0.0, 0.0001);
  EXPECT_STREQ(ev[1].first.c_str(), "ffp_start2");
  EXPECT_NEAR(ev[1].second, 0.5, 0.0001);
  EXPECT_STREQ(ev[2].first.c_str(), "end");
  EXPECT_NEAR(ev[2].second, 1.0, 0.0001);
  EXPECT_STREQ(ev[3].first.c_str(), "ffp_end2");
  EXPECT_NEAR(ev[3].second, 1.5, 0.0001);
}

