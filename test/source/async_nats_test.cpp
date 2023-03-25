#include <string>

#include "async_nats/async_nats.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Name is async_nats", "[library]")
{
  auto const exported = exported_class {};
  REQUIRE(std::string("async_nats") == exported.name());
}
