#define CATCH_CONFIG_MAIN

#include <stdio.h>

#include "../include/project/epaper.h"
#include "../include/third-party/catch.hpp"

using namespace epaper;

TEST_CASE("sanity", "[sanity]") {
    REQUIRE(1 == 1);
    REQUIRE(0x05 == 0x05);
}
