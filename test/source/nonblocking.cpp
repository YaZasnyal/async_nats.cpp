#include <boost/asio/use_future.hpp>

#include "nats_fixture.hpp"

TEST_F(NatsFixture, NonblockingSendRecv)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();
  auto token = sub.get_cancellation_token();

  const async_nats::nonblocking::Receiver nb_recv(std::move(sub));

  const async_nats::nonblocking::Sender nb_snd(m, c);

  std::string message = "test";
  nb_snd.try_send(boost::asio::const_buffer(message.data(), message.size()));
  nb_snd.send(boost::asio::const_buffer(message.data(), message.size()));

  // give some time for server to send message back
  std::this_thread::sleep_for(default_sleep);

  auto msg = nb_recv.receive();
  GTEST_ASSERT_EQ(msg, true);
  GTEST_ASSERT_EQ(msg.topic(), m);
  GTEST_ASSERT_EQ(msg.data(), message);
  GTEST_ASSERT_EQ(msg.reply_to(), std::nullopt);
  GTEST_ASSERT_EQ(msg.headers(), false);

  msg = nb_recv.try_receive();
  GTEST_ASSERT_EQ(msg, true);
  GTEST_ASSERT_EQ(msg.topic(), m);
  GTEST_ASSERT_EQ(msg.data(), message);
  GTEST_ASSERT_EQ(msg.reply_to(), std::nullopt);
  GTEST_ASSERT_EQ(msg.headers(), false);

  msg = nb_recv.try_receive();
  GTEST_ASSERT_EQ(msg, false);

  // check receive after cancel
  token.cancel();
  msg = nb_recv.receive();
  GTEST_ASSERT_EQ(msg, false);
}
