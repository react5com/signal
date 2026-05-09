#include <catch2/catch_test_macros.hpp>
#include "Sample.h"

TEST_CASE("Sample::calculate") {
    Sample sample;
    REQUIRE(sample.calculate(2) == 4);
    REQUIRE(sample.calculate(3) == 6);
}
