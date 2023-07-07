#include <boost/asio/use_future.hpp>

#include "nats_fixture.hpp"

TEST_F(NatsFixture, ReplyTo)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();

  auto reply_m = c.new_mailbox();
  auto reply_sub = c.subcribe(reply_m, boost::asio::use_future).get();

  std::string request = "test";
  c.publish(m,
            reply_m,
            boost::asio::const_buffer(request.data(), request.size()),
            boost::asio::use_future)
      .get();

  auto req = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(req, true);
  GTEST_ASSERT_EQ(req.topic(), m);
  GTEST_ASSERT_EQ(req.data(), request);
  GTEST_ASSERT_EQ(req.reply_to(), reply_m);

  std::string response = "test reply";
  c.publish(req.reply_to().value(),
            boost::asio::const_buffer(response.data(), response.size()),
            boost::asio::use_future)
      .get();

  auto res = reply_sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(res, true);
  GTEST_ASSERT_EQ(res.topic(), reply_m);
  GTEST_ASSERT_EQ(res.data(), response);
  GTEST_ASSERT_EQ(res.reply_to(), std::nullopt);
}

TEST_F(NatsFixture, ReplyToNoResponders)
{
  auto m = c.new_mailbox();
  auto reply_m = c.new_mailbox();
  auto reply_sub = c.subcribe(reply_m, boost::asio::use_future).get();

  std::string request = "test";
  c.publish(m,
            reply_m,
            boost::asio::const_buffer(request.data(), request.size()),
            boost::asio::use_future)
      .get();

  auto res = reply_sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(res, true);
  GTEST_ASSERT_EQ(res.topic(), reply_m);
  GTEST_ASSERT_EQ(res.status(), async_nats::Message::StatusCodes::NoResponders);
}
