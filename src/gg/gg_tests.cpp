#include "common/log.h"
#include "gg/gg.h"
#include <gtest/gtest.h>
#include <optional>

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

TEST(gg_tests, mercator_tests) {
  double eps = 0.1;

  EXPECT_NEAR(gg::mercator::project_lat(-85.0), -179.41, eps);
  EXPECT_NEAR(gg::mercator::project_lat(-70.0), -99.43, eps);
  EXPECT_NEAR(gg::mercator::project_lat(-35.0), -37.40, eps);
  EXPECT_NEAR(gg::mercator::project_lat(-15.0), -15.17, eps);
  EXPECT_NEAR(gg::mercator::project_lat(0.00), 0.0, eps);
  EXPECT_NEAR(gg::mercator::project_lat(+15.0), +15.17, eps);
  EXPECT_NEAR(gg::mercator::project_lat(+35.0), +37.40, eps);
  EXPECT_NEAR(gg::mercator::project_lat(+70.0), +99.43, eps);
  EXPECT_NEAR(gg::mercator::project_lat(+85.0), +179.41, eps);

  EXPECT_NEAR(gg::mercator::project_lon(-180.0), -180.0, eps);
  EXPECT_NEAR(gg::mercator::project_lon(-35.0), -35.0, eps);
  EXPECT_NEAR(gg::mercator::project_lon(0.00), 0.0, eps);
  EXPECT_NEAR(gg::mercator::project_lon(+35.0), +35.0, eps);
  EXPECT_NEAR(gg::mercator::project_lon(+180.0), +180.0, eps);

  // not sure if internal floating point math are deterministic across different
  // platforms, so these test can be flaky and might requires smth like
  // EXPECT_NEAR for ints with precision up to some number of units.
  EXPECT_EQ(gg::mercator::lat_to_yu(-85.0), 7034790);
  EXPECT_EQ(gg::mercator::lat_to_yu(-70.0), 961214103);
  EXPECT_EQ(gg::mercator::lat_to_yu(-35.0), 1701227231);
  EXPECT_EQ(gg::mercator::lat_to_yu(-15.0), 1966446683);
  EXPECT_EQ(gg::mercator::lat_to_yu(0.000), 2147483647);
  EXPECT_EQ(gg::mercator::lat_to_yu(+15.0), 2328520611);
  EXPECT_EQ(gg::mercator::lat_to_yu(+35.0), 2593740063);
  EXPECT_EQ(gg::mercator::lat_to_yu(+70.0), 3333753191);
  EXPECT_EQ(gg::mercator::lat_to_yu(+85.0), 4287932504);

  EXPECT_EQ(gg::mercator::lon_to_xu(-180.0), 0);
  EXPECT_EQ(gg::mercator::lon_to_xu(-85.0), 1133394147);
  EXPECT_EQ(gg::mercator::lon_to_xu(-70.0), 1312351117);
  EXPECT_EQ(gg::mercator::lon_to_xu(-35.0), 1729917382);
  EXPECT_EQ(gg::mercator::lon_to_xu(-15.0), 1968526676);
  EXPECT_EQ(gg::mercator::lon_to_xu(0.000), 2147483647);
  EXPECT_EQ(gg::mercator::lon_to_xu(+15.0), 2326440618);
  EXPECT_EQ(gg::mercator::lon_to_xu(+35.0), 2565049912);
  EXPECT_EQ(gg::mercator::lon_to_xu(+70.0), 2982616177);
  EXPECT_EQ(gg::mercator::lon_to_xu(+85.0), 3161573147);
  EXPECT_EQ(gg::mercator::lon_to_xu(+180.0),
            std::numeric_limits<uint32_t>::max());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}