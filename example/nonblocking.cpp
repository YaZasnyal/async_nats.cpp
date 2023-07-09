#include <iostream>

#include <boost/asio.hpp>

#include <async_nats/async_nats.hpp>

auto main(int /*argc*/, char** /*argv*/) -> int
{
  try {
    const async_nats::TokioRuntime rt;
    const boost::asio::io_context ctx;

    // connect to the nats server
    async_nats::ConnectionOptions options;
    options.name("test_app").address("nats://localhost:4222");
    async_nats::Connection connection =
        async_nats::connect(rt, options, boost::asio::use_future).get();

    // generate random topic
    const async_nats::OwnedString mailbox = connection.new_mailbox();

    // subscribe for new events
    // NOTE: creating subscribtion is an async operation and is going to block current thread.
    // Other completion handlers like callback can be used instead.
    async_nats::Subscribtion sub = connection.subcribe(mailbox, boost::asio::use_future).get();
    const async_nats::nonblocking::Receiver recv(std::move(sub));

    // create sender
    // This operation does not block
    const async_nats::nonblocking::Sender sender(mailbox, connection);

    // send a message
    std::string message = "Hello world!";
    sender.send(boost::asio::const_buffer(message.data(), message.size()));

    // receive a message
    // this method MAY block if there isn't any message available right now
    // use try_receive() for real non-blocking experience
    const async_nats::Message msg = recv.receive();
    std::cout << msg.topic() << ": " << msg.data() << std::endl;
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
