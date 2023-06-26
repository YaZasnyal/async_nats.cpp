#include <boost/asio/use_future.hpp>

#include "nats_fixture.hpp"

/// Check if cancel works if subscribtion is cancelled instantly
TEST_F(NatsFixture, subscribtion_cancellation_token_instant)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();

  sub.get_cancellation_token().cancel();
  auto msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, false);
}

/// Check if cancel works if subscribtion is cancelled from another thread
TEST_F(NatsFixture, subscribtion_cancellation_token)
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
TEST_F(NatsFixture, subscribtion_cancellation_token_drain)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();
  auto token = sub.get_cancellation_token();

  c.publish(m, boost::asio::const_buffer(), boost::asio::use_future).get();
  // give some time for server to send message back
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  token.cancel();
  auto msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, true);
  msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, false);
}
