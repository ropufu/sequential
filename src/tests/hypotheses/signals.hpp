
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIGNALS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIGNALS_HPP_INCLUDED

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include "../core.hpp"
#include "../../hypotheses/signals.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <random>     // std::mt19937
#include <stdexcept>  // std::logic_error
#include <string>     // std::string
#include <variant>    // std::variant


#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_CONSTANT_SIGNAL_TYPES  \
    ropufu::sequential::hypotheses::constant_signal<std::size_t>, \
    ropufu::sequential::hypotheses::constant_signal<float>,       \
    ropufu::sequential::hypotheses::constant_signal<double>,      \
    ropufu::sequential::hypotheses::constant_signal<char>         \

#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_TRANSITIONARY_SIGNAL_TYPES     \
    ropufu::sequential::hypotheses::transitionary_signal<std::size_t, 0>, \
    ropufu::sequential::hypotheses::transitionary_signal<std::size_t, 1>, \
    ropufu::sequential::hypotheses::transitionary_signal<float, 0>,       \
    ropufu::sequential::hypotheses::transitionary_signal<float, 2>,       \
    ropufu::sequential::hypotheses::transitionary_signal<double, 0>,      \
    ropufu::sequential::hypotheses::transitionary_signal<double, 3>,      \
    ropufu::sequential::hypotheses::transitionary_signal<char, 0>,        \
    ropufu::sequential::hypotheses::transitionary_signal<char, 4>         \


TEST_CASE_TEMPLATE("testing constant signal", tested_t, ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_CONSTANT_SIGNAL_TYPES)
{
    using value_type = typename tested_t::value_type;

    value_type zero {};
    value_type one = static_cast<value_type>(1);
    value_type two = static_cast<value_type>(2);

    tested_t no_signal {};
    tested_t constant_one {one};
    tested_t constant_two {two};

    for (std::size_t i = 0; i < 10; ++i)
    {
        CHECK(no_signal(i) == zero);
        CHECK(constant_one(i) == one);
        CHECK(constant_two(i) == two);
    } // for (...)

    std::string xxx {};
    std::string yyy {};

    ropufu::tests::does_json_round_trip(no_signal, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(constant_one, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(constant_two, xxx, yyy);
    CHECK_EQ(xxx, yyy);
} // TEST_CASE_TEMPLATE(...)

TEST_CASE_TEMPLATE("testing transitionary signal", tested_t, ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_TRANSITIONARY_SIGNAL_TYPES)
{
    using engine_type = std::mt19937;
    using value_type = typename tested_t::value_type;
    using container_type = typename tested_t::transition_container_type;
    CAPTURE(tested_t::transition_size);

    value_type zero {};
    value_type one = static_cast<value_type>(1);
    value_type two = static_cast<value_type>(2);

    engine_type engine {};
    engine.seed(1729);
    container_type transition_a {};
    container_type transition_b {};
    for (value_type& x : transition_a) x = static_cast<value_type>(engine());
    for (value_type& x : transition_b) x = static_cast<value_type>(engine());

    tested_t no_signal {};
    tested_t constant_one {one};
    tested_t constant_two {two};
    tested_t jitter_to_one {one, transition_a};
    tested_t jitter_to_two {two, transition_b};

    for (std::size_t i = 0; i < tested_t::transition_size; ++i)
    {
        CHECK(no_signal(i) == zero);
        CHECK(constant_one(i) == one);
        CHECK(constant_two(i) == two);
        CHECK(jitter_to_one(i) == transition_a[i]);
        CHECK(jitter_to_two(i) == transition_b[i]);
    } // for (...)

    for (std::size_t i = tested_t::transition_size; i < 10 + tested_t::transition_size; ++i)
    {
        CHECK(no_signal(i) == zero);
        CHECK(constant_one(i) == one);
        CHECK(constant_two(i) == two);
        CHECK(jitter_to_one(i) == one);
        CHECK(jitter_to_two(i) == two);
    } // for (...)

    std::string xxx {};
    std::string yyy {};

    ropufu::tests::does_json_round_trip(no_signal, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(constant_one, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(constant_two, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(jitter_to_one, xxx, yyy);
    CHECK_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(jitter_to_two, xxx, yyy);
    CHECK_EQ(xxx, yyy);
} // TEST_CASE_TEMPLATE(...)

TEST_CASE_TEMPLATE("testing signal discrimination", value_t, float, long double)
{
    using a_type = ropufu::sequential::hypotheses::constant_signal<value_t>;
    using b_type = ropufu::sequential::hypotheses::transitionary_signal<value_t, 1>;
    using c_type = ropufu::sequential::hypotheses::transitionary_signal<value_t, 2>;
    using x_type = ropufu::sequential::hypotheses::transitionary_signal<value_t, 3>;

    using variant_type = std::variant<a_type, b_type, c_type>;

    a_type a {1729};
    b_type b {3, {2}};
    c_type c {13, {5, 8}};
    x_type x {89, {21, 34, 55}};

    variant_type a_variant = a;
    variant_type b_variant = b;
    variant_type c_variant = c;
    variant_type v {};

    nlohmann::json a_json = a;
    nlohmann::json b_json = b;
    nlohmann::json c_json = c;
    nlohmann::json x_json = x;

    REQUIRE(ropufu::noexcept_json::try_get(a_json, v));
    REQUIRE(std::holds_alternative<a_type>(v));
    CHECK(std::get<a_type>(v) == a);

    REQUIRE(ropufu::noexcept_json::try_get(b_json, v));
    REQUIRE(std::holds_alternative<b_type>(v));
    CHECK(std::get<b_type>(v) == b);

    REQUIRE(ropufu::noexcept_json::try_get(c_json, v));
    REQUIRE(std::holds_alternative<c_type>(v));
    CHECK(std::get<c_type>(v) == c);

    REQUIRE(!ropufu::noexcept_json::try_get(x_json, v));
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIGNALS_HPP_INCLUDED
