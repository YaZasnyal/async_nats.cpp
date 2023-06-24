#include <async_nats/async_nats.hpp>

#define BOOST_ASIO_HAS_CO_AWAIT

#include <format>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

boost::asio::awaitable<void> requester(boost::asio::io_context& ctx, async_nats::Connection conn);
boost::asio::awaitable<void> replier(async_nats::Connection conn, async_nats::Subscribtion sub);

auto main(int, char**) -> int
{
  async_nats::TokioRuntime rt;
  boost::asio::io_context ctx;

  try {
    async_nats::Connection conn =
        async_nats::connect(
            rt,
            async_nats::ConnectionOptions().name("test_app").address("nats://localhost:4222"),
            boost::asio::use_future)
            .get();
    auto req = boost::asio::co_spawn(ctx, requester(ctx, conn), boost::asio::use_future);
    ctx.run();
    req.get();
  } catch (const async_nats::ConnectionError& e) {
    std::cerr << "ConnectionError: type=" << e.kind() << "; text='" << e.what() << "'"
              << std::endl;
    return -1;
  }

  return 0;
}

boost::asio::awaitable<void> requester(boost::asio::io_context& ctx, async_nats::Connection conn)
{
  // create subscription and launch replier task
  async_nats::Subscribtion sub = co_await conn.subcribe("test", boost::asio::use_awaitable);
  boost::asio::co_spawn(ctx, replier(conn, std::move(sub)), boost::asio::detached);

  // create and subscribe a reply inbox
  auto reply_to = conn.new_mailbox();
  auto reply_sub = co_await conn.subcribe(reply_to, boost::asio::use_awaitable);

  // send a request
  std::string request = "world";
  co_await conn.publish("test",
                        reply_to,
                        boost::asio::const_buffer(request.data(), request.size()),
                        boost::asio::use_awaitable);

  // receive the response
  async_nats::Message reply = co_await reply_sub.receive(boost::asio::use_awaitable);
  std::cout << reply.to_string() << std::endl;
}

boost::asio::awaitable<void> replier(async_nats::Connection conn, async_nats::Subscribtion sub)
{
  // listen for requests
  async_nats::Message msg = co_await sub.receive(boost::asio::use_awaitable);
  if (!msg)
    co_return;

  // send back a response
  std::string reply = std::format("Hello, {}!", msg.data());
  co_await conn.publish(msg.reply_to().value(),
                        boost::asio::const_buffer(reply.data(), reply.size()),
                        boost::asio::use_awaitable);
}
