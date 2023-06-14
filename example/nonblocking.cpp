#include <iostream>

#include <boost/asio.hpp>

#include <async_nats/async_nats.hpp>

auto main(int, char**) -> int
{
  async_nats::TokioRuntime rt;
  boost::asio::io_context ctx;

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

    // subscribe for new events
    // NOTE: creating subscribtion is an async operation and is going to block current thread.
    // Other completion handlers like callback can be used instead.
    async_nats::Subscribtion sub = connection.subcribe(mailbox, boost::asio::use_future).get();
    async_nats::nonblocking::Receiver recv(std::move(sub));

    // create sender
    // This operation does not block
    async_nats::nonblocking::Sender sender(mailbox, connection);

    // send a message
    std::string message = "Hello world!";
    sender.send(boost::asio::const_buffer(message.data(), message.size()));

    // receive a message
    // this method MAY block if there is no message available right now
    // use try_receive for read non-blocking receiving
    auto msg = recv.receive();
    std::cout << msg.topic() << ": " << msg.data() << std::endl;
  } catch (const async_nats::ConnectionError& e) {
    std::cerr << "ConnectionError: type=" << e.kind() << "; text='" << e.what() << "'"
              << std::endl;
    return -1;
  }

  return 0;
}
