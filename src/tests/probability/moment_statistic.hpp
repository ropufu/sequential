
#ifndef ROPUFU_SEQUENTIAL_TESTS_PROBABILITY_MOMENT_STATISTIC_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_PROBABILITY_MOMENT_STATISTIC_TEST_HPP_INCLUDED

#include <doctest/doctest.h>

#include <ropufu/algebra/elementwise.hpp>
#include <ropufu/algebra/matrix.hpp>

#include "../core.hpp"
#include "../../draft/probability/moment_statistic.hpp"

#include <cstddef>      // std::size_t
#include <system_error> // std::error_code, std::errc
#include <vector>       // std::vector

#define ROPUFU_SEQUENTIAL_TESTS_PROBABILITY_VALUE_STATISTIC_TYPES  \
    ropufu::sequential::tests::type_pair<std::size_t, double>,     \
    ropufu::sequential::tests::type_pair<float, double>,           \
    ropufu::sequential::tests::type_pair<double, long double>      \


TEST_CASE_TEMPLATE("testing moment statistic for scalar types", pair_t, ROPUFU_SEQUENTIAL_TESTS_PROBABILITY_VALUE_STATISTIC_TYPES)
{
    using observation_type = typename pair_t::left_type;
    using statistic_type = typename pair_t::right_type;
    using tested_type_a = ropufu::aftermath::probability::moment_statistic<observation_type, statistic_type>;
    using tested_type_b = ropufu::aftermath::probability::moment_statistic<observation_type, statistic_type, 0>;
    using tested_type_c = ropufu::aftermath::probability::moment_statistic<observation_type, statistic_type, 8>;
    
    // 0, 1, 2, ..., 194.
    std::vector<std::size_t> data = ropufu::aftermath::algebra::elementwise::identity_permutation(195);
    double mean = 97;
    double var = 3185;
    for (statistic_type anticipated_mean = 0; anticipated_mean < 100; anticipated_mean += 13)
    {
        tested_type_a stat_a {anticipated_mean};
        tested_type_b stat_b {anticipated_mean};
        tested_type_c stat_c {anticipated_mean};
        for (std::size_t x : data)
        {
            observation_type y = static_cast<observation_type>(x);
            stat_a.observe(y);
            stat_b.observe(y);
            stat_c.observe(y);
        } // for (...)
        CHECK(static_cast<double>(stat_a.mean()) == doctest::Approx(mean).epsilon(0.0001)); // 0.01% tolerance.
        CHECK(static_cast<double>(stat_b.mean()) == doctest::Approx(mean).epsilon(0.0001)); // 0.01% tolerance.
        CHECK(static_cast<double>(stat_c.mean()) == doctest::Approx(mean).epsilon(0.0001)); // 0.01% tolerance.

        CHECK(static_cast<double>(stat_a.variance()) == doctest::Approx(var).epsilon(0.0001)); // 0.01% tolerance.
        CHECK(static_cast<double>(stat_b.variance()) == doctest::Approx(var).epsilon(0.0001)); // 0.01% tolerance.
        CHECK(static_cast<double>(stat_c.variance()) == doctest::Approx(var).epsilon(0.0001)); // 0.01% tolerance.
    } // for (...)
} // TEST_CASE_TEMPLATE(...)

TEST_CASE_TEMPLATE("testing moment statistic for matrix types", pair_t, ROPUFU_SEQUENTIAL_TESTS_PROBABILITY_VALUE_STATISTIC_TYPES)
{
    using observation_scalar_type = typename pair_t::left_type;
    using statistic_scalar_type = typename pair_t::right_type;
    using observation_type = ropufu::aftermath::algebra::matrix<observation_scalar_type>;
    using statistic_type = ropufu::aftermath::algebra::matrix<statistic_scalar_type>;
    using tested_type = ropufu::aftermath::probability::moment_statistic<observation_type, statistic_type>;
    
    // 0, 1, 2, ..., 194.
    std::vector<std::size_t> data = ropufu::aftermath::algebra::elementwise::identity_permutation(195);
    double mean = 97;
    double var = 3185;

    statistic_type anticipated_mean_a {7, 1};
    statistic_type anticipated_mean_b {5, 39};
    anticipated_mean_a.fill(100);
    anticipated_mean_b.fill(200);

    tested_type stat_a {anticipated_mean_a};
    tested_type stat_b {anticipated_mean_b};
    for (std::size_t k = 0; k < data.size(); ++k)
    {
        std::size_t offset = 0;
        observation_type observation_a {7, 1};
        observation_type observation_b {5, 39};
        for (observation_scalar_type& x : observation_a) x = static_cast<observation_scalar_type>(data[(k + (++offset)) % data.size()]);
        for (observation_scalar_type& x : observation_b) x = static_cast<observation_scalar_type>(data[(k + (++offset)) % data.size()]);

        stat_a.observe(observation_a);
        stat_b.observe(observation_b);
    } // for (...)

    for (statistic_scalar_type x : stat_a.mean()) CHECK(static_cast<double>(x) == doctest::Approx(mean).epsilon(0.0001)); // 0.01% tolerance.
    for (statistic_scalar_type x : stat_b.mean()) CHECK(static_cast<double>(x) == doctest::Approx(mean).epsilon(0.0001)); // 0.01% tolerance.

    for (statistic_scalar_type x : stat_a.variance()) CHECK(static_cast<double>(x) == doctest::Approx(var).epsilon(0.0001)); // 0.01% tolerance.
    for (statistic_scalar_type x : stat_b.variance()) CHECK(static_cast<double>(x) == doctest::Approx(var).epsilon(0.0001)); // 0.01% tolerance.
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_PROBABILITY_MOMENT_STATISTIC_TEST_HPP_INCLUDED
