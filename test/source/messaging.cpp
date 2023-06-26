#include <boost/asio/use_future.hpp>

#include "nats_fixture.hpp"

TEST_F(NatsFixture, client_send_recv_future)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();

  std::string message = "test";
  c.publish(m, boost::asio::const_buffer(message.data(), message.size()), boost::asio::use_future)
      .get();

  auto msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, true);
  GTEST_ASSERT_EQ(msg.topic(), m);
  GTEST_ASSERT_EQ(msg.data(), message);
  GTEST_ASSERT_EQ(msg.reply_to(), std::nullopt);
  GTEST_ASSERT_EQ(msg.headers(), false);
}
