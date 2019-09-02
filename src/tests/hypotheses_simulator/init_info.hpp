
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED

#include <doctest/doctest.h>

#include "../core.hpp"
#include "../../hypotheses_simulator/init_info.hpp"

#include <cstddef> // std::size_t
#include <vector>  // std::vector


TEST_CASE_TEMPLATE("testing init info", value_t, float, double, long double)
{
    using init_info_type = ropufu::sequential::hypotheses::init_info<value_t>;

    init_info_type a {};
    init_info_type b {1};
    init_info_type c {1};

    CHECK(a != b);
    CHECK(b == c);

    CHECK(ropufu::sequential::tests::does_json_round_trip(a));
    CHECK(ropufu::sequential::tests::does_json_round_trip(b));

    c.set_threshold_range({1, 2}, {3, 4});
    CHECK(b != c);
    CHECK(ropufu::sequential::tests::does_json_round_trip(c));

    a.set_anticipated_run_length(1729);
    CHECK(ropufu::sequential::tests::does_json_round_trip(a));

    std::size_t m = 17;
    std::size_t n = 29;
    std::vector<value_t> null {};
    std::vector<value_t> alt {};
    c.make_thresholds({m, n}, ropufu::sequential::hypotheses::spacing::logarithmic, null, alt);

    CHECK(null.size() == m);
    CHECK(alt.size() == n);
    
    CHECK(null.front() == 1);
    CHECK(null.back() == 2);

    CHECK(alt.front() == 3);
    CHECK(alt.back() == 4);
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED
