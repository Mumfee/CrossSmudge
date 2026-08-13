#include <gtest/gtest.h>

#include <string>

#include "util/TextPool.h"

TEST(TextPool, GrowsForLongFirstEntryAndKeepsOffsetsTerminated) {
  std::string pool;
  const std::string first(300, 'a');
  const uint16_t firstOffset = TextPool::append(pool, first.data(), first.size());
  const std::string second = "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D";  // Hebrew UTF-8 bytes
  const uint16_t secondOffset = TextPool::append(pool, second.data(), second.size());

  EXPECT_EQ(firstOffset, 0);
  EXPECT_EQ(secondOffset, first.size() + 1);
  EXPECT_STREQ(pool.data() + firstOffset, first.c_str());
  EXPECT_STREQ(pool.data() + secondOffset, second.c_str());
  EXPECT_GE(pool.capacity(), pool.size());
}

TEST(TextPool, GrowsPastExistingCapacityForLargeAppend) {
  std::string pool;
  TextPool::append(pool, "small", 5);
  const size_t priorCapacity = pool.capacity();
  const std::string large(priorCapacity + 300, 'x');
  const uint16_t offset = TextPool::append(pool, large.data(), large.size());

  EXPECT_STREQ(pool.data() + offset, large.c_str());
  EXPECT_GE(pool.capacity(), pool.size());
}
