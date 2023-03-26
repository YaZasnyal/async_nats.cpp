#include <iostream>

#include <async_nats/async_nats.hpp>

#define BOOST_ASIO_HAS_CO_AWAIT

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

boost::asio::awaitable<void> sender(async_nats::Connection conn)
{
  std::string msg = "Hello world!";
  co_await conn.publish(
      "test", boost::asio::const_buffer(msg.data(), msg.size()), boost::asio::use_awaitable);
}

boost::asio::awaitable<void> receiver(async_nats::Connection conn)
{
  async_nats::Subscribtion sub = co_await conn.subcribe("test", boost::asio::use_awaitable);
  async_nats::Message msg = co_await sub.receive(boost::asio::use_awaitable);
  if(!msg)
  {
    std::cerr << "Unable to receive message" << std::endl;
    co_return;
  }
  std::cout << msg.Topic() << ": " << msg.Data() << std::endl;
}

/**
 * Run the nats-server executable.
 *
 * Run:
 * ```sh
 * nats subscribe test
 * ```
 *
 * Execute this example and observe received message.
 */
auto main() -> int
{
  async_nats::TokioRuntime rt;
  boost::asio::io_context ctx;

  // connect using promise/future
  async_nats::ConnectionOptions options;
  options.name("test_app");
  options.address("nats://localhost:4222");
  async_nats::Connection conn = async_nats::connect(rt, options, boost::asio::use_future).get();

  // launch two coroutines to send a receive data
  boost::asio::co_spawn(ctx, receiver(conn), boost::asio::detached);
  boost::asio::co_spawn(ctx, sender(conn), boost::asio::detached);

  ctx.run();

  return 0;
}
