#include <gtest/gtest.h>

#include <async_nats/async_nats.hpp>

class NatsFixture : public ::testing::Test
{
public:
  inline static async_nats::TokioRuntime rt = async_nats::TokioRuntime();
  inline static constexpr auto test_timeout = std::chrono::seconds(1);

  NatsFixture();
  ~NatsFixture();

  void SetUp();
  void TearDown();

  async_nats::Connection c;
};
