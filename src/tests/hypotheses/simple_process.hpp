
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMPLE_PROCESS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMPLE_PROCESS_HPP_INCLUDED

#include <doctest/doctest.h>

#include "../core.hpp"
#include "../../hypotheses/signals.hpp"
#include "../../hypotheses/noises.hpp"
#include "../../hypotheses/simple_process.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <random>     // std::mt19937
#include <stdexcept>  // std::logic_error


#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_PROCESS_NOISE_TYPES                \
    ropufu::sequential::hypotheses::white_noise<std::ranlux24, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::ranlux48, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937, double>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937_64, long double> \


TEST_CASE_TEMPLATE("testing simple process", noise_type, ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_PROCESS_NOISE_TYPES)
{
    using engine_type = typename noise_type::engine_type;
    using value_type = typename noise_type::value_type;
    using signal_t = ropufu::sequential::hypotheses::constant_signal<value_type>;
    using process_t = ropufu::sequential::hypotheses::simple_process<engine_type, value_type>;

    engine_type engine {};
    ropufu::tests::seed(engine);

    signal_t signal_one {1};
    noise_type white_two {2};
    double actual_mu = 1.729;
    process_t process {signal_one, white_two, static_cast<value_type>(actual_mu)};

    constexpr std::size_t sample_size = 1'000;
    for (std::size_t i = 0; i < sample_size; ++i)
    {
        process.tic(engine);
    } // for (...)
    REQUIRE(process.count() == sample_size);

    CHECK(static_cast<double>(process.estimate_signal_strength()) == doctest::Approx(actual_mu).epsilon(0.5));
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMPLE_PROCESS_HPP_INCLUDED
