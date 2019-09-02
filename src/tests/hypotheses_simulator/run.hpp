
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED

#include <doctest/doctest.h>

#include "../core.hpp"
#include "../../hypotheses/model.hpp"
#include "../../hypotheses/hypothesis_pair.hpp"
#include "../../hypotheses_simulator/init_info.hpp"
#include "../../hypotheses_simulator/run.hpp"

#include <cstddef> // std::size_t
#include <vector>  // std::vector


TEST_CASE_TEMPLATE("testing run", value_t, float, double, long double)
{
    using model_type = ropufu::sequential::hypotheses::model<value_t>;
    using hypothesis_pair_type = ropufu::sequential::hypotheses::hypothesis_pair<std::size_t>;
    using init_info_type = ropufu::sequential::hypotheses::init_info<value_t>;
    using run_type = ropufu::sequential::hypotheses::run<value_t>;

    init_info_type initi_a {};
    init_info_type initi_b {1};
    init_info_type initi_c {2};
    initi_a.set_anticipated_run_length(1729);
    initi_c.set_threshold_range({1, 2}, {3, 4});

    model_type mod {5, 13};

    run_type a {};
    run_type b {mod};
    run_type c {mod};

    CHECK(a != b);

    CHECK(ropufu::sequential::tests::does_json_round_trip(a));
    CHECK(ropufu::sequential::tests::does_json_round_trip(b));

    b.study(1, 1); b.study(2, 3);
    c.study(2, 3); c.study(1, 1);
    CHECK(b == c);

    a.study(initi_a);
    b.study(initi_b);
    c.study(initi_c);
    CHECK(b != c);

    a.set_thresholds(5, 8, ropufu::sequential::hypotheses::spacing::linear);
    CHECK(a.threshold_count() == hypothesis_pair_type(5, 8));

    CHECK(ropufu::sequential::tests::does_json_round_trip(a));
    CHECK(ropufu::sequential::tests::does_json_round_trip(b));
    CHECK(ropufu::sequential::tests::does_json_round_trip(c));
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED
