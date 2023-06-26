#include "nats_fixture.hpp"

#include <boost/asio/use_future.hpp>

NatsFixture::NatsFixture() {}

NatsFixture::~NatsFixture() {}

void NatsFixture::SetUp()
{
  c = async_nats::connect(rt,
                          async_nats::ConnectionOptions().address("nats://localhost:4222"),
                          boost::asio::use_future)
          .get();
  GTEST_ASSERT_NE(c.get_raw(), nullptr);
}

void NatsFixture::TearDown() {}
