#define CATCH_CONFIG_MAIN

#include <stdio.h>

#include "../include/project/catch.hpp"
#include "../include/project/epaper.h"

using namespace rangefinder;


TEST_CASE("sanity", "[sanity]") {
    REQUIRE(1 == 1);
    REQUIRE(0x05 == 0x05);
}

