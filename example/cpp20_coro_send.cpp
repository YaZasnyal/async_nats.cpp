#include <iostream>

#include <async_nats/async_nats.hpp>

#define BOOST_ASIO_HAS_CO_AWAIT

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

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

  boost::asio::co_spawn(
      ctx,
      [&]() -> boost::asio::awaitable<void>
      {
        async_nats::ConnectionOptions options;
        options.name("test_app");
        options.address("nats://localhost:4222");
        auto connection = co_await async_nats::connect(rt, options, boost::asio::use_awaitable);
        std::string message = "Hello World!";
        co_await connection.publish("test",
                                    boost::asio::const_buffer(message.data(), message.size()),
                                    boost::asio::use_awaitable);
      },
      boost::asio::detached);
  ctx.run();

  return 0;
}
