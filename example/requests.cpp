#include <format>
#include <iostream>

#include <boost/asio.hpp>

#include <async_nats/async_nats.hpp>

auto main(int, char**) -> int
{
  async_nats::TokioRuntime rt;

  try {
    // connect to the nats server
    async_nats::Connection connection =
        async_nats::connect(
            rt,
            async_nats::ConnectionOptions().name("test_app").address("nats://localhost:4222"),
            boost::asio::use_future)
            .get();

    // generate random topic
    async_nats::detail::OwnedString mailbox = connection.new_mailbox();

    // subscribe for requests
    async_nats::Subscribtion sub = connection.subcribe(mailbox, boost::asio::use_future).get();

    // send a request
    std::string message = "world";
    auto req = connection.request(mailbox,
                                  boost::asio::const_buffer(message.data(), message.size()),
                                  boost::asio::use_future);

    // receive the request
    async_nats::Message msg = sub.receive(boost::asio::use_future).get();
    std::cout << std::format("Received request: topic='{}'; text='{}'; reply_to='{}'",
                             msg.topic(),
                             msg.data(),
                             *msg.reply_to())
              << std::endl;

    // format a response and send it back
    std::string reply_text = std::string("Hello, ").append(msg.data()).append("!");
    std::cout << std::format("Sending reply to: {}", msg.reply_to().value()) << std::endl;
    connection
        .publish(msg.reply_to().value(),
                 boost::asio::const_buffer(reply_text.data(), reply_text.size()),
                 boost::asio::use_future)
        .get();

    // await and print the response
    async_nats::Message response = req.get();
    std::cout << std::format("{}: {}", response.topic(), response.data()) << std::endl;
  } catch (const async_nats::ConnectionError& e) {
    std::cerr << std::format(
        "ConnectionError: type={}; text='{}'", static_cast<int>(e.kind()), e.what())
              << std::endl;
    return -1;
  } catch (const async_nats::RequestError& e) {
    std::cerr << std::format(
        "RequestError: type={}; text='{}'", static_cast<int>(e.kind()), e.what())
              << std::endl;
    return -2;
  }

  return 0;
}
