
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_CHANGE_OF_MEASURE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_CHANGE_OF_MEASURE_HPP_INCLUDED

#include <doctest/doctest.h>

#include "../core.hpp"
#include "../../hypotheses/change_of_measure.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <stdexcept>  // std::logic_error
#include <string>     // std::string


TEST_CASE_TEMPLATE("testing change of measure", value_t, float, double, long double)
{
    using change_of_measure_type = ropufu::sequential::hypotheses::change_of_measure<value_t>;

    change_of_measure_type a {};
    change_of_measure_type b {0, 1};
    change_of_measure_type c {1, 0};
    change_of_measure_type d {2, 2};

    CHECK(b != c);
    CHECK(d.is_identity());

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

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_CHANGE_OF_MEASURE_HPP_INCLUDED
