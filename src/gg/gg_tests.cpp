#include "common/log.h"
#include "gg/gg.h"
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <optional>

TEST(gg_tests, gpt_latlon_convertions) {
    // this lat/lons expected to be already projected to mercator
    EXPECT_EQ(gg::lat_to_y(+180.0), std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(gg::lat_to_y(-180.0), 0);
    EXPECT_EQ(gg::lon_to_x(+180.0), std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(gg::lon_to_x(-180.0), 0);
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
    EXPECT_EQ(gg::mercator::lon_to_xu(+180.0), std::numeric_limits<uint32_t>::max());
}

TEST(gg_tests, eliminate_parallel_segments_test) {
    using vertice_t = std::tuple<double, double>;
    std::vector<vertice_t> test_segments = {{3.7756453790000819, -85.051128779806589}, // 0
                                            {3.8215438160000872, -85.051128779806589}, // 1
                                            {4.3893335300000444, -85.051128779806589}, // 2
                                            {4.3997501960000704, -85.051128779806589}, // 3
                                            {6.1833602220000898, -85.051128779806589}, // 4
                                            {6.1989852220000898, -85.051128779806589}, // 5
                                            {6.7927352220000898, -85.051128779806589}, // 6
                                            {6.8078719410000872, -85.051128779806589}, // 7
                                            {7.2424422540000819, -85.051128779806589}, // 8
                                            {7.2504988940000885, -85.051128779806589}, // 9
                                            {7.6463322270000731, -85.051128779806589}, // 10
                                            {7.4150496750000912, -85.051128779806589}, // 11
                                            {7.9170028000000912, -85.051128779806589}, // 12
                                            {7.8369246750000912, -85.051128779806589}, // 13
                                            {9.1574813160000872, -85.051128779806589}, // 14
                                            {8.9382430350000845, -85.051128779806589}, // 15
                                            {9.6557723320000832, -85.051128779806589}, // 16
                                            {9.6621199880000858, -85.051128779806589}, // 17
                                            {12.046071811000047, -85.051128779806589}, // 18
                                            {12.08171634200005, -85.051128779806589},  // 19
                                            {12.074473504000082, -85.051128779806589}};
    std::vector<gg ::p32> projected_segments;
    for (auto [x, y] : test_segments) {
        x = gg::mercator::project_lon(gg::mercator::clamp_lon_to_valid(x));
        y = gg::mercator::project_lat(gg::mercator::clamp_lat_to_valid(y));
        gg::gpt_units_t ux = gg::lon_to_x(x);
        gg::gpt_units_t uy = gg::lat_to_y(y);
        projected_segments.emplace_back(gg::p32(ux, uy));
    }

    auto result = gg::utils::eliminate_parallel_segments(projected_segments);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], projected_segments[0]);
    EXPECT_EQ(result[1], projected_segments.back());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}