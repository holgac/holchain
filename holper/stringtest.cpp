#include <gtest/gtest.h>
#include <algorithm>
#include "string.h"

TEST(StringUtilsTest, to) {
  int val;
  EXPECT_TRUE(St::to("123", val));
  EXPECT_EQ(val, 123);
  EXPECT_FALSE(St::to("123a", val));
  EXPECT_FALSE(St::to("xyz", val));
  std::string str;
  EXPECT_TRUE(St::to("xyz", str));
  EXPECT_EQ(str, "xyz");
  EXPECT_TRUE(St::to("", str));
  EXPECT_EQ(str, "");
  nullptr_t n;
  EXPECT_TRUE(St::to("", n));
  EXPECT_FALSE(St::to("a", n));
  EXPECT_FALSE(St::to("1", n));
}

namespace {
  template <typename T>
  void expectVectorEqual(const std::vector<T>& v1, const std::vector<T>& v2) {
    EXPECT_EQ(v1.size(), v2.size());
    for (size_t i = 0; i<std::min(v1.size(), v2.size()); ++i) {
      EXPECT_EQ(v1[i], v2[i]);
    }
  }
}

TEST(StringUtilsTest, split) {
  expectVectorEqual(St::split<int>("1,2,3,4", ','), std::vector<int>{1,2,3,4});
  EXPECT_ANY_THROW(St::split<int>(",1,2,3,4", ','));
  EXPECT_ANY_THROW(St::split<int>("1,2,3,4,", ','));
  expectVectorEqual(St::split<std::string>(",1,2,3,4,", ','),
      std::vector<std::string>{"", "1", "2", "3", "4", ""});
}

