#include <gtest/gtest.h>
#include "subprocess.h"

TEST(SubprocessTest, output) {
  std::string str("test");
  Subprocess sp({"/usr/bin/echo", "-n", str});
  std::string out, err;
  sp.finish(out, err);
  EXPECT_EQ(out, str);
  EXPECT_TRUE(err.empty());
}

TEST(SubprocessTest, input) {
  std::string str("test 1 2 \n3 4 5");
  Subprocess sp({"/usr/bin/cat"});
  sp.write(str);
  sp.write(str);
  sp.write(str);
  std::string out, err;
  sp.finish(out, err);
  EXPECT_EQ(out, str+str+str);
  EXPECT_TRUE(err.empty());
}

TEST(SubprocessTest, xsel) {
  std::string str("xsel \n data");
  std::string out, err;
  Subprocess sp({"/usr/bin/xsel", "-is"});
  sp.write(str);
  sp.finish(out, err);
  EXPECT_TRUE(out.empty());
  EXPECT_TRUE(err.empty());
  Subprocess sp2({"/usr/bin/xsel", "-os"});
  sp2.finish(out, err);
  EXPECT_EQ(out, str);
  EXPECT_TRUE(err.empty());
}
