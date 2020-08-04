
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_NOISES_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_NOISES_HPP_INCLUDED

#include <doctest/doctest.h>

#include "../core.hpp"
#include "../../hypotheses/noises.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <random>     // std::mt19937
#include <stdexcept>  // std::logic_error
#include <variant>    // std::variant


#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_WHITE_NOISE_TYPES                  \
    ropufu::sequential::hypotheses::white_noise<std::ranlux24, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::ranlux48, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937, double>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937_64, long double> \

#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_AR_NOISE_TYPES                                \
    ropufu::sequential::hypotheses::auto_regressive_noise<std::ranlux24, float, 0>,      \
    ropufu::sequential::hypotheses::auto_regressive_noise<std::ranlux24, float, 1>,      \
    ropufu::sequential::hypotheses::auto_regressive_noise<std::ranlux48, double, 2>,     \
    ropufu::sequential::hypotheses::auto_regressive_noise<std::mt19937, long double, 3>, \
    ropufu::sequential::hypotheses::auto_regressive_noise<std::mt19937_64, double, 4>    \


TEST_CASE_TEMPLATE("testing white noise", tested_t, ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_WHITE_NOISE_TYPES)
{
    using engine_type = typename tested_t::engine_type;
    // using value_type = typename tested_t::value_type;

    engine_type engine {};
    ropufu::sequential::tests::seed(engine);

    tested_t no_noise {};   // Trivially exact representation.
    tested_t white_one {1}; // Exact representation.
    tested_t white_two {2}; // Exact representation.

    constexpr std::size_t sample_size = 1'000;
    long double var_one = 0;
    long double var_two = 0;
    for (std::size_t i = 0; i < sample_size; ++i)
    {
        no_noise.tic(engine);
        white_one.tic(engine);
        white_two.tic(engine);

        REQUIRE(no_noise.current_value() == 0);
        var_one += white_one.current_value() * white_one.current_value();
        var_two += white_two.current_value() * white_two.current_value();
    } // for (...)
    var_one /= static_cast<long double>(sample_size);
    var_two /= static_cast<long double>(sample_size);

    CHECK(static_cast<double>(var_one) == doctest::Approx(1.0).epsilon(0.10));
    CHECK(static_cast<double>(var_two) == doctest::Approx(4.0).epsilon(0.10));

    // The following are represented exactly by the floating point type.
    CHECK(ropufu::sequential::tests::does_json_round_trip(no_noise));
    CHECK(ropufu::sequential::tests::does_json_round_trip(white_one));
    CHECK(ropufu::sequential::tests::does_json_round_trip(white_two));
} // TEST_CASE_TEMPLATE(...)

TEST_CASE_TEMPLATE("testing auto-regressive noise", tested_t, ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_AR_NOISE_TYPES)
{
    using engine_type = typename tested_t::engine_type;
    using value_type = typename tested_t::value_type;
    using container_type = typename tested_t::ar_container_type;
    using white_type = typename tested_t::white_noise_type;
    CAPTURE(tested_t::ar_size);

    container_type ar_parameters_a {};
    container_type ar_parameters_b {};
    for (std::size_t i = 0; i < tested_t::ar_size; ++i) ar_parameters_a[i] = 1 / static_cast<value_type>(2 * (i + 1) * (i + 1));
    for (std::size_t i = 0; i < tested_t::ar_size; ++i) ar_parameters_b[i] = ((i % 2) == 0 ? -1 : 1) / static_cast<value_type>(2 * (i + 1) * (i + 1));

    for (std::size_t i = 0; i < tested_t::ar_size; ++i)
    {
        CAPTURE(ar_parameters_a[i]);
        CAPTURE(ar_parameters_b[i]);
    } // for (...)

    engine_type engine {};
    ropufu::sequential::tests::seed(engine);

    white_type white_zero {};
    white_type white_one {1};
    white_type white_two {2};

    tested_t no_noise_a {};
    tested_t no_noise_b {white_zero};
    tested_t ar_one_positive {white_one, ar_parameters_a};
    tested_t ar_two_alternating {white_two, ar_parameters_b};

    constexpr std::size_t sample_size = 5'000;
    long double var_one = 0;
    long double var_two = 0;
    for (std::size_t i = 0; i < sample_size; ++i)
    {
        no_noise_a.tic(engine);
        no_noise_b.tic(engine);
        ar_one_positive.tic(engine);
        ar_two_alternating.tic(engine);

        REQUIRE(no_noise_a.current_value() == 0);
        REQUIRE(no_noise_b.current_value() == 0);
        var_one += ar_one_positive.current_value() * ar_one_positive.current_value();
        var_two += ar_two_alternating.current_value() * ar_two_alternating.current_value();
    } // for (...)
    var_one /= static_cast<long double>(sample_size);
    var_two /= static_cast<long double>(sample_size);

    CHECK(static_cast<double>(var_one) > 0.9); // Theoretical value should be greater than 1---the no-AR case.
    CHECK(static_cast<double>(var_two) > 3.6); // Theoretical value should be greater than 4---the no-AR case.

    // The following are represented exactly by the floating point type.
    CHECK(ropufu::sequential::tests::does_json_round_trip(no_noise_a));
    CHECK(ropufu::sequential::tests::does_json_round_trip(no_noise_b));

    // The following are not represented exactly by the floating point type.
    CHECK(ropufu::sequential::tests::does_json_round_trip(ar_one_positive, 0.01));
    CHECK(ropufu::sequential::tests::does_json_round_trip(ar_two_alternating, 0.01));
} // TEST_CASE_TEMPLATE(...)

TEST_CASE_TEMPLATE("testing noise discrimination", value_t, float, long double)
{
    using engine_type = std::mt19937;
    using white_type = ropufu::sequential::hypotheses::white_noise<engine_type, value_t>;

    using a_type = white_type;
    using b_type = ropufu::sequential::hypotheses::auto_regressive_noise<engine_type, value_t, 1>;
    using c_type = ropufu::sequential::hypotheses::auto_regressive_noise<engine_type, value_t, 2>;
    using x_type = ropufu::sequential::hypotheses::auto_regressive_noise<engine_type, value_t, 3>;

    using variant_type = std::variant<a_type, b_type, c_type>;

    a_type a {1729};
    b_type b {white_type(3), {value_t(0.5)}};
    c_type c {white_type(13), {value_t(0.05), value_t(0.08)}};
    x_type x {white_type(89), {value_t(0.21), value_t(0.34), value_t(0.55)}};

    variant_type a_variant = a;
    variant_type b_variant = b;
    variant_type c_variant = c;
    variant_type v {};

    nlohmann::json a_json = a;
    nlohmann::json b_json = b;
    nlohmann::json c_json = c;
    nlohmann::json x_json = x;

    REQUIRE(ropufu::sequential::hypotheses::try_discriminate_noise(a_json, v));
    REQUIRE(std::holds_alternative<a_type>(v));
    CHECK(std::get<a_type>(v) == a);

    REQUIRE(ropufu::sequential::hypotheses::try_discriminate_noise(b_json, v));
    REQUIRE(std::holds_alternative<b_type>(v));
    CHECK(std::get<b_type>(v) == b);

    REQUIRE(ropufu::sequential::hypotheses::try_discriminate_noise(c_json, v));
    REQUIRE(std::holds_alternative<c_type>(v));
    CHECK(std::get<c_type>(v) == c);

    REQUIRE(!ropufu::sequential::hypotheses::try_discriminate_noise(x_json, v));
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_NOISES_HPP_INCLUDED
