#include "nats_fixture.hpp"

/// Test if two mailboxes are different
TEST_F(NatsFixture, Mailbox)
{
  auto box1 = c.new_mailbox();
  auto box2 = c.new_mailbox();
  GTEST_ASSERT_EQ(static_cast<std::string_view>(box1) == static_cast<std::string_view>(box2),
                  false);
}
