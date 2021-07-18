#include <gtest/gtest.h>
#include <optional>
#include "gg.h"

TEST(gg_tests, continuality_test) {
  gg::punits_t prev;
  bool first = true;
  for (auto lat = -90.00000001; lat < 90.0; lat += 1.0) {
    if (first) {
      first = false;
      prev = gg::lat_to_y_pu(lat);
    } else {
      EXPECT_GT(gg::lat_to_y_pu(lat), prev) << "lat: " << lat;
      std::cout << gg::lat_to_y_pu(lat) << std::endl;
    }
  }
}
