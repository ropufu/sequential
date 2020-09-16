
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED

#include <doctest/doctest.h>

#include "../core.hpp"
#include "../../hypotheses/hypothesis_pair.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <stdexcept>  // std::logic_error
#include <string>     // std::string


TEST_CASE_TEMPLATE("testing hypothesis pair", value_t, float, double, long double)
{
    using hypothesis_pair_type = ropufu::sequential::hypotheses::hypothesis_pair<value_t>;

    hypothesis_pair_type a {};
    hypothesis_pair_type b {0, 1};
    hypothesis_pair_type c {1, 0};
    hypothesis_pair_type d {2, 2};

    CHECK(b != c);

    std::string xxx {};
    std::string yyy {};

    ropufu::tests::does_json_round_trip(a, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(b, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(c, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(d, xxx, yyy);
    CHECK_EQ(xxx, yyy);
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
