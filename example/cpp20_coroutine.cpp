#include <iostream>
#include <thread>

#include <async_nats/async_nats.hpp>

#define BOOST_ASIO_HAS_CO_AWAIT

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

boost::asio::awaitable<void> example_task(async_nats::Connection conn);

/**
 * Run the nats-server executable.
 *
 * You can also run:
 * ```sh
 * nats subscribe test
 * ```
 *
 * Execute this example and observe received message.
 */
auto main(int, char**) -> int
{
  async_nats::TokioRuntime rt;
  boost::asio::io_context ctx;

  try {
    auto connection =
        async_nats::connect(
            rt,
            async_nats::ConnectionOptions().name("test_app").address("nats://localhost:4222"),
            boost::asio::use_future)
            .get();

    boost::asio::co_spawn(ctx, example_task(connection), boost::asio::detached);
    ctx.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  return 0;
}

boost::asio::awaitable<void> example_task(async_nats::Connection conn)
{
  // subscribe for messages
  auto sub = co_await conn.subcribe("test", boost::asio::use_awaitable);

  // prepare and send a message
  std::string message = "Hello world!";
  co_await conn.publish("test",
                        boost::asio::const_buffer(message.data(), message.size()),
                        boost::asio::use_awaitable);

  // read a message from the sub
  async_nats::Message msg = co_await sub.receive(boost::asio::use_awaitable);
  if (!msg)
    co_return;
  std::cout << msg.Topic() << ": " << msg.Data() << std::endl;
}
