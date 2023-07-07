#include <gtest/gtest.h>

#include <async_nats/async_nats.hpp>

class NatsFixture : public ::testing::Test
{
public:
  inline static constexpr auto test_timeout = std::chrono::seconds(1);
  inline static constexpr auto default_sleep = std::chrono::milliseconds(20);

  NatsFixture();
  NatsFixture(const NatsFixture&) = delete;
  NatsFixture(NatsFixture&&) = delete;
  ~NatsFixture() override;

  NatsFixture& operator=(const NatsFixture&) = delete;
  NatsFixture& operator=(NatsFixture&&) = delete;

  void SetUp() override;
  void TearDown() override;

  async_nats::TokioRuntime rt;
  async_nats::Connection c;
};
