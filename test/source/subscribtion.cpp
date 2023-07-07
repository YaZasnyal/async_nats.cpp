#include <boost/asio/use_future.hpp>

#include "nats_fixture.hpp"

/// Check if cancel works if subscribtion is cancelled instantly
TEST_F(NatsFixture, SubscribtionCancellationTokenInstant)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();

  sub.get_cancellation_token().cancel();
  auto msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, false);
}

/// Check if cancel works if subscribtion is cancelled from another thread
TEST_F(NatsFixture, SubscribtionCancellationToken)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();
  auto token = sub.get_cancellation_token();

  std::thread t([&, token = std::move(token)]() { token.cancel(); });

  auto msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, false);
  t.join();
}

/// Check if cancel allows to read remaining messages
TEST_F(NatsFixture, SubscribtionCancellationTokenDrain)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();
  auto token = sub.get_cancellation_token();

  c.publish(m, boost::asio::const_buffer(), boost::asio::use_future).get();
  // give some time for server to send message back
  std::this_thread::sleep_for(default_sleep);

  token.cancel();
  auto msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, true);
  msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, false);
}
