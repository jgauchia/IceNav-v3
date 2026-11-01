/**
 * @file gpsMath_reference_test.cpp
 * @brief Host test: calcDist()/calcCourse() with the trig LUT vs a double-precision reference
 *
 * @details Runs on the PC, not on the board. The ESP-IDF headers used by gpsMath
 *          are replaced by the stubs in test/host/stubs. The reference is
 *          geo-utils-cpp (header-only, https://github.com/gistrec/geo-utils-cpp),
 *          used only by this test.
 *
 * Build and run from the repository root:
 *
 *   g++ -std=gnu++17 -O2 -DBOARD_HAS_PSRAM \
 *       -Itest/host/stubs -Ilib/utils/src -I<geo-utils-cpp>/include \
 *       test/host/gpsMath_reference_test.cpp lib/utils/src/gpsMath.cpp \
 *       -o gpsMath_reference_test && ./gpsMath_reference_test
 *
 * Exit code 0 = PASS, 1 = FAIL.
 */

#include <cmath>
#include <cstdio>

#include <geo/geo.hpp>

#include "gpsMath.hpp"

static double refDist(float lat1, float lon1, float lat2, float lon2)
{
    return geo::angle_between({lat1, lon1}, {lat2, lon2}) * EARTH_RADIUS;
}

static double refCourse(float lat1, float lon1, float lat2, float lon2)
{
    double h = geo::heading({lat1, lon1}, {lat2, lon2});
    return h < 0.0 ? h + 360.0 : h;
}

static double courseDiff(double a, double b)
{
    double d = fabs(a - b);
    return d > 180.0 ? 360.0 - d : d;
}

/**
 * @brief Worst calcDist()/calcCourse() error for 10 m steps in 24 directions
 *        from positions between 60S and 60N
 *
 * @details Inputs are rounded to float first, so the reference sees the same points.
 */
static void sweep10m(double* worstDist, double* worstCourse)
{
    *worstDist = 0.0;
    *worstCourse = 0.0;
    for (int i = 0; i < 400; i++)
    {
        float lat = -60.0f + 120.0f * i / 400.0f;
        float lon = -179.0f + 358.0f * ((i * 7919) % 400) / 400.0f;
        for (int h = 0; h < 360; h += 15)
        {
            geo::LatLng to = geo::offset({lat, lon}, 10.0, h);
            float lat2 = (float)to.lat;
            float lon2 = (float)to.lng;

            double distErr = fabs(calcDist(lat, lon, lat2, lon2) - refDist(lat, lon, lat2, lon2));
            double courseErr = courseDiff(calcCourse(lat, lon, lat2, lon2), refCourse(lat, lon, lat2, lon2));
            if (distErr > *worstDist)
                *worstDist = distErr;
            if (courseErr > *worstCourse)
                *worstCourse = courseErr;
        }
    }
}

/**
 * @brief Sum of calcDist() over a 1 km track with a point every 2 m, heading 225 (south-west)
 */
static double track1km(double* reference)
{
    double sum = 0.0;
    geo::LatLng p = {40.4168, -3.7038};
    *reference = 0.0;
    for (int i = 0; i < 500; i++)
    {
        geo::LatLng q = geo::offset(p, 2.0, 225.0);
        sum += calcDist((float)p.lat, (float)p.lng, (float)q.lat, (float)q.lng);
        *reference += refDist((float)p.lat, (float)p.lng, (float)q.lat, (float)q.lng);
        p = q;
    }
    return sum;
}

int main()
{
    if (!initTrigLUT())
    {
        printf("FAIL: initTrigLUT() did not enable the LUT (build with -DBOARD_HAS_PSRAM)\n");
        return 1;
    }

    // Baseline: the same code with sinf()/cosf(). Its error comes from float32
    // inputs and arithmetic, which the LUT can't fix but must not make worse.
    double baseDist, baseCourse, lutDist, lutCourse, ref;
    lutInit = false;
    sweep10m(&baseDist, &baseCourse);
    lutInit = true;
    sweep10m(&lutDist, &lutCourse);
    double track = track1km(&ref);

    int failures = 0;
    printf("10 m steps, worst calcDist error:   sinf/cosf %.2f m, LUT %.2f m\n", baseDist, lutDist);
    printf("10 m steps, worst calcCourse error: sinf/cosf %.1f deg, LUT %.1f deg\n", baseCourse, lutCourse);
    // calcCourse() loses precision in float even without the LUT (cancellation in x
    // for short legs), so the course only gets an absolute limit here.
    if (lutDist > baseDist + 0.1 || lutCourse > 10.0)
        failures++;

    printf("1 km track, 2 m steps, heading 225: LUT sum %.0f m, reference %.0f m\n", track, ref);
    if (fabs(track - ref) > 0.01 * ref)
        failures++;

    printf("%s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
