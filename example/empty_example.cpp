#include <async_nats/async_nats.hpp>

#include <iostream>

#define BOOST_ASIO_HAS_CO_AWAIT

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

boost::asio::awaitable<void> sender(async_nats::Connection conn)
{
  std::string message;
  message.resize(10);
  while(true)
  {
    co_await conn.publish("test", boost::asio::const_buffer(message.data(), message.size()), boost::asio::use_awaitable);
  }
}

auto main() -> int
{
  async_nats::TokioRuntime rt;

  boost::asio::io_context ctx;

//  boost::asio::co_spawn(
//      ctx,
//      [&]() -> boost::asio::awaitable<void>
//      {
//        async_nats::ConnectionOptions options;
//        options.name("test_app");
//        options.address("nats://localhost:4222");
//        std::string message = "Hello Worldghjghjgjhghjgjhgj!";
//        auto connection = co_await async_nats::connect(rt, options, boost::asio::use_awaitable);
//        co_await connection.publish("test", boost::asio::const_buffer(message.data(), message.size()), boost::asio::use_awaitable);
//      },
//      boost::asio::detached);
//  ctx.run();


  async_nats::ConnectionOptions options;
  options.name("test_app");
  options.address("nats://localhost:4222");

  auto conn_future = async_nats::connect(rt, options, boost::asio::use_future);
  try
  {
    auto connection = conn_future.get();
    boost::asio::co_spawn(ctx, sender(connection), boost::asio::detached);
    ctx.run();
  }
  catch(std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }

  return 0;
}
