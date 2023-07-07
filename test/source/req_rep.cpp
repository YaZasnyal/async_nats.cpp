#include <boost/asio/use_future.hpp>

#include "nats_fixture.hpp"

TEST_F(NatsFixture, ReqRep)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();

  std::string request = "test";
  auto req = c.request(m,
                       std::move(async_nats::RequestBuilder().data(
                           boost::asio::const_buffer(request.data(), request.size()))),
                       boost::asio::use_future);

  std::string reply = "test reply";
  auto msg = sub.receive(boost::asio::use_future).get();
  GTEST_ASSERT_EQ(msg, true);
  GTEST_ASSERT_EQ(msg.topic(), m);
  GTEST_ASSERT_EQ(msg.data(), request);

  c.publish(msg.reply_to().value(),
            boost::asio::const_buffer(reply.data(), reply.size()),
            boost::asio::use_future)
      .get();

  auto response = req.get();
  GTEST_ASSERT_EQ(response, true);
  GTEST_ASSERT_EQ(response.data(), reply);
}

TEST_F(NatsFixture, ReqRepNoResponder)
{
  auto m = c.new_mailbox();
  std::string request = "test";
  auto req = c.request(m,
                       std::move(async_nats::RequestBuilder().data(
                           boost::asio::const_buffer(request.data(), request.size()))),
                       boost::asio::use_future);

  bool exception = false;
  try {
    auto response = req.get();
  } catch (const async_nats::RequestError& e) {
    GTEST_ASSERT_EQ(e.kind(), AsyncNatsRequestErrorKind::AsyncNats_Request_NoResponders);
    exception = true;
  }
  GTEST_ASSERT_EQ(exception, true);
}

TEST_F(NatsFixture, ReqRepTimeout)
{
  auto m = c.new_mailbox();
  auto sub = c.subcribe(m, boost::asio::use_future).get();

  std::string request = "test";
  bool exception = false;
  try {
    auto req =
        c.request(m,
                  std::move(async_nats::RequestBuilder()
                                .data(boost::asio::const_buffer(request.data(), request.size()))
                                .timeout(default_sleep)),
                  boost::asio::use_future)
            .get();
  } catch (const async_nats::RequestError& e) {
    GTEST_ASSERT_EQ(e.kind(), AsyncNatsRequestErrorKind::AsyncNats_Request_TimedOut);
    exception = true;
  }
  GTEST_ASSERT_EQ(exception, true);
}
