#include <gtest/gtest.h>
#include <optional>
#include "gg.h"

TEST(gg_tests, gpt_latlon_convertions) {
  EXPECT_EQ(gg::lat_to_y(+90.0), std::numeric_limits<uint32_t>::max() / 2);
  EXPECT_EQ(gg::lat_to_y(-90.0), 0);
  EXPECT_EQ(gg::lon_to_x(+180.0), std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(gg::lon_to_x(-180.0), 0);

  for (double step = 0.1, prev_lat = -90.0, lat = prev_lat + step; lat <= 90.0;
       lat += step) {
    EXPECT_GT(gg::lat_to_y(lat), gg::lat_to_y(prev_lat))
        << "failed at: " << lat << "," << prev_lat;
    prev_lat = lat;
  }
  for (double step = 0.1, prev_lon = -180.0, lon = prev_lon + step;
       lon <= 180.0; lon += step) {
    EXPECT_GT(gg::lon_to_x(lon), gg::lon_to_x(prev_lon))
        << "failed at: " << lon << "," << prev_lon;
    prev_lon = lon;
  }

  EXPECT_NEAR(gg::y_to_lat(std::numeric_limits<uint32_t>::max() / 2), 90.0,
              0.0001);
  EXPECT_NEAR(gg::y_to_lat(0), -90.0, 0.0001);

  for (double v : {-90.0, -45.0, 0.0, 45.0, 90.0}) {
    EXPECT_NEAR(gg::y_to_lat(gg::lat_to_y(v)), v, 0.0001);
  }
  for (double v : {-180.0, -45.0, 0.0, 45.0, 180.0}) {
    EXPECT_NEAR(gg::x_to_lon(gg::lon_to_x(v)), v, 0.0001);
  }
}
