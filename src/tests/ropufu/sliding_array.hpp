
#ifndef ROPUFU_SEQUENTIAL_TESTS_ROPUFU_SLIDING_ARRAY_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_ROPUFU_SLIDING_ARRAY_HPP_INCLUDED

#include <doctest/doctest.h>

#include "../core.hpp"
#include "../../draft/ropufu/sliding_array.hpp"

#include <array>      // std::array
#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <random>     // std::mt19937
#include <stdexcept>  // std::logic_error

#define ROPUFU_SEQUENTIAL_TESTS_ROPUFU_SLIDING_ARRAY_TYPES     \
    ropufu::aftermath::sliding_array<std::size_t, 0>,          \
    ropufu::aftermath::sliding_array<float, 1>,                \
    ropufu::aftermath::sliding_array<double, 2>,               \
    ropufu::aftermath::sliding_array<long double, 3>,          \
    ropufu::aftermath::sliding_array<char, 4>                  \


TEST_CASE_TEMPLATE("testing sliding array", tested_t, ROPUFU_SEQUENTIAL_TESTS_ROPUFU_SLIDING_ARRAY_TYPES)
{
    using engine_type = std::mt19937;
    using value_type = typename tested_t::value_type;

    constexpr std::size_t sample_size = 80;

    engine_type engine {};
    engine.seed(1729);

    std::array<value_type, sample_size> sequence {};
    for (value_type& x : sequence) x = static_cast<value_type>(engine());

    tested_t window {};
    // Initial value.
    for (std::size_t i = 0; i < window.size(); ++i) REQUIRE(window[i] == 0);
    // Transitionary period.
    for (std::size_t i = 0; i < window.size(); ++i)
    {
        window.push_back(sequence[i]);
        std::size_t index_of_first_observation = window.size() - i - 1;
        for (std::size_t k = 0; k < index_of_first_observation; ++k) REQUIRE(window[k] == 0);
        for (std::size_t k = index_of_first_observation; k < window.size(); ++k)
            REQUIRE(window[k] == sequence[k - index_of_first_observation]);
    } // for (...)
    // Stationary run.
    for (std::size_t i = window.size(); i < sample_size; ++i)
    {
        window.push_back(sequence[i]);
        for (std::size_t k = 0; k < window.size(); ++k)
            REQUIRE(window[k] == sequence[(k + i + 1) - window.size()]);
    } // for (...)
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_ROPUFU_SLIDING_ARRAY_HPP_INCLUDED
