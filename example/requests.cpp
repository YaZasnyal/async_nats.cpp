#include <iostream>

#include <boost/asio.hpp>

#include <async_nats/async_nats.hpp>

auto main(int /*argc*/, char** /*argv*/) -> int
{
  try {
    const async_nats::TokioRuntime rt;
    // connect to the nats server
    async_nats::Connection connection =
        async_nats::connect(
            rt,
            async_nats::ConnectionOptions().name("test_app").address("nats://localhost:4222"),
            boost::asio::use_future)
            .get();

    // generate random topic
    const async_nats::OwnedString mailbox = connection.new_mailbox();

    // subscribe for requests
    async_nats::Subscribtion sub = connection.subcribe(mailbox, boost::asio::use_future).get();

    // send a request
    std::string message = "world";
    auto req = connection.request(mailbox,
                                  boost::asio::const_buffer(message.data(), message.size()),
                                  boost::asio::use_future);

    // receive the request
    const async_nats::Message msg = sub.receive(boost::asio::use_future).get();
    if (!msg.reply_to()) {
      return 1;
    }
    std::cout << "Received request: topic=" << msg.topic() << "; text='" << msg.data()
              << "'; reply_to='" << msg.reply_to().value() << "'" << std::endl;

    // format a response and send it back
    std::string reply_text = std::string("Hello, ").append(msg.data()).append("!");
    std::cout << "Sending reply to: " << msg.reply_to().value() << std::endl;
    connection
        .publish(msg.reply_to().value(),
                 boost::asio::const_buffer(reply_text.data(), reply_text.size()),
                 boost::asio::use_future)
        .get();

    // await and print the response
    const async_nats::Message response = req.get();
    std::cout << response.topic() << ": " << response.data() << std::endl;
  } catch (const async_nats::ConnectionError& e) {
    std::cerr << "ConnectionError: type=" << e.kind() << "; text='" << e.what() << "'"
              << std::endl;
    return -1;
  } catch (const async_nats::RequestError& e) {
    std::cerr << "RequestError: type=" << e.kind() << "; text='" << e.what() << "'" << std::endl;
    return -1;
  } catch (const std::exception& e) {
    std::cerr << "Exception: text='" << e.what() << "'" << std::endl;
    return -2;
  }

  return 0;
}
