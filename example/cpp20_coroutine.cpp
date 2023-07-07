#include <exception>
#include <iostream>
#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

#include <async_nats/async_nats.hpp>

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
auto main(int /*argc*/, char** /*argv*/) -> int
{
  try {
    const async_nats::TokioRuntime rt;
    boost::asio::io_context ctx;

    const async_nats::Connection connection =
        async_nats::connect(
            rt,
            async_nats::ConnectionOptions().name("test_app").address("nats://localhost:4222"),
            boost::asio::use_future)
            .get();

    boost::asio::co_spawn(ctx, example_task(connection), boost::asio::detached);
    ctx.run();
  } catch (const async_nats::ConnectionError& e) {
    std::cerr << "ConnectionError: type=" << e.kind() << "; text='" << e.what() << "'"
              << std::endl;
    return -1;
  } catch (const std::exception& e) {
    std::cerr << "Exception: text='" << e.what() << "'" << std::endl;
    return -2;
  }

  return 0;
}

boost::asio::awaitable<void> example_task(async_nats::Connection conn)
{
  // subscribe for messages
  async_nats::Subscribtion sub = co_await conn.subcribe("test", boost::asio::use_awaitable);

  // prepare and send a message
  std::string message = "Hello world!";
  co_await conn.publish("test",
                        boost::asio::const_buffer(message.data(), message.size()),
                        boost::asio::use_awaitable);

  // read a message from the sub
  async_nats::Message msg = co_await sub.receive(boost::asio::use_awaitable);
  if (!msg) {
    co_return;
  }
  std::cout << msg.topic() << ": " << msg.data() << std::endl;
  co_return;
}
