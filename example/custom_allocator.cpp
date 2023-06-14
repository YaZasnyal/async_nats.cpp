#include <iostream>
#include <thread>

#include <boost/asio/recycling_allocator.hpp>

#include <async_nats/async_nats.hpp>

#define BOOST_ASIO_HAS_CO_AWAIT

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

boost::asio::awaitable<void> example_task(async_nats::TokioRuntime& conn);

auto main(int, char**) -> int
{
  async_nats::TokioRuntime rt;
  boost::asio::io_context ctx;

  try {
    auto res = boost::asio::co_spawn(ctx, example_task(rt), boost::asio::use_future);
    ctx.run();
    res.get();
  } catch (const async_nats::ConnectionError& e) {
    std::cerr << "ConnectionError: type=" << e.kind() << "; text='" << e.what() << "'"
              << std::endl;
    return -1;
  }

  return 0;
}

/**
 * @brief allocator attach recycling_allocator to the completion token
 */
inline auto allocator(auto&& token)
{
  return boost::asio::bind_allocator(boost::asio::recycling_allocator<void>(),
                                     std::forward<decltype(token)>(token));
}

/**
 * @brief token returns boost::asio::use_awaitable with recycling_allocator attached
 */
inline auto token()
{
  return allocator(boost::asio::use_awaitable);
}

boost::asio::awaitable<void> example_task(async_nats::TokioRuntime& rt)
{
  async_nats::Connection conn = co_await async_nats::connect(
      rt, async_nats::ConnectionOptions().address("nats://localhost:4222"), token());

  // subscribe for messages
  async_nats::Subscribtion sub = co_await conn.subcribe("test", token());

  // prepare and send a message
  std::string message = "Hello world!";
  co_await conn.publish(
      "test", boost::asio::const_buffer(message.data(), message.size()), token());

  // read the message from the sub
  async_nats::Message msg = co_await sub.receive(token());
  if (!msg)
    co_return;
  std::cout << msg.topic() << ": " << msg.data() << std::endl;
}
