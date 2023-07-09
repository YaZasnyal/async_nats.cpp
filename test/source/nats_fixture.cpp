#include "nats_fixture.hpp"

#include <boost/asio/use_future.hpp>

NatsFixture::NatsFixture() = default;

NatsFixture::~NatsFixture() = default;

void NatsFixture::SetUp()
{
  async_nats::ConnectionOptions options;
  options.address("nats://localhost:4222");
  c = async_nats::connect(rt, options, boost::asio::use_future).get();
  GTEST_ASSERT_NE(c.get_raw(), nullptr);
}

void NatsFixture::TearDown() {}
