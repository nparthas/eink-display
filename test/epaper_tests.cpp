#define CATCH_CONFIG_MAIN

#include <stdio.h>

#include "../include/project/catch.hpp"
#include "../include/project/rangefinder.h"

using namespace rangefinder;


TEST_CASE("sanity", "[sanity]") {
    // ensure that neighbourhood[0] is the lowest cost
    REQUIRE(1 == 1);
    REQUIRE(0x05 == 0x05);
}

