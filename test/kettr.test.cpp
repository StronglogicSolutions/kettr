#include "kettr.test.hpp"

TEST(KettrTest, Login)
{
  EXPECT_TRUE((kettr{"email", "password"}.login()));
}
