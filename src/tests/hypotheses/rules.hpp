
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_RULES_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_RULES_HPP_INCLUDED

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include "../core.hpp"
#include "../../hypotheses/signals.hpp"
#include "../../hypotheses/noises.hpp"
#include "../../hypotheses/simple_process.hpp"
#include "../../hypotheses/change_of_measure.hpp"

#include "../../hypotheses/model.hpp"
#include "../../hypotheses/likelihood.hpp"
#include "../../hypotheses/observer.hpp"
#include "../../hypotheses/rules.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <random>     // std::mt19937
#include <stdexcept>  // std::logic_error
#include <string>     // std::string
#include <vector>     // std::vector


#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_RULES_NOISE_TYPES                  \
    ropufu::sequential::hypotheses::white_noise<std::ranlux24, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::ranlux48, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937, double>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937_64, long double> \


TEST_CASE_TEMPLATE("testing rule designs", value_type, float, double, long double)
{
    ropufu::sequential::hypotheses::adaptive_sprt_design<value_type> asprt_simple_design {ropufu::sequential::hypotheses::adaptive_sprt_flavor::simple, 0};
    ropufu::sequential::hypotheses::adaptive_sprt_design<value_type> asprt_general_design {ropufu::sequential::hypotheses::adaptive_sprt_flavor::general, 1};
    ropufu::sequential::hypotheses::generalized_sprt_design<value_type> gsprt_cutoff_design {ropufu::sequential::hypotheses::generalized_sprt_flavor::cutoff, 2};
    ropufu::sequential::hypotheses::generalized_sprt_design<value_type> gsprt_general_design {ropufu::sequential::hypotheses::generalized_sprt_flavor::general, 3};
    ropufu::sequential::hypotheses::double_sprt_design<value_type> dsprt_asymp_design {4};
    ropufu::sequential::hypotheses::double_sprt_design<value_type> dsprt_huffman_design {5};

    dsprt_asymp_design.set_asymptotic_init(true);
    dsprt_huffman_design.set_huffman_correction(true);

    std::string xxx {};
    std::string yyy {};
    
    ropufu::tests::does_json_round_trip(asprt_simple_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(asprt_general_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(gsprt_cutoff_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(gsprt_general_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(dsprt_asymp_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(dsprt_huffman_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    asprt_simple_design.set_relative_init(value_type(0.2), value_type(0.8));
    asprt_general_design.set_relative_init(value_type(0.3), value_type(0.7));
    gsprt_cutoff_design.set_relative_mu_cutoff(value_type(0.4));
    gsprt_general_design.set_relative_mu_cutoff(value_type(0.6));
    dsprt_asymp_design.set_relative_mu_intermediate(value_type(0.1));
    dsprt_huffman_design.set_relative_mu_intermediate(value_type(0.9));
    
    ropufu::tests::does_json_round_trip(asprt_simple_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(asprt_general_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(gsprt_cutoff_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(gsprt_general_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(dsprt_asymp_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);

    ropufu::tests::does_json_round_trip(dsprt_huffman_design, xxx, yyy);
    REQUIRE_EQ(xxx, yyy);
} // TEST_CASE_TEMPLATE(...)

TEST_CASE_TEMPLATE("testing rules", noise_type, ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_RULES_NOISE_TYPES)
{
    using engine_type = typename noise_type::engine_type;
    using value_type = typename noise_type::value_type;
    using signal_type = ropufu::sequential::hypotheses::constant_signal<value_type>;
    using process_type = ropufu::sequential::hypotheses::simple_process<engine_type, value_type>;
    using model_type = ropufu::sequential::hypotheses::model<value_type>;
    using likelihood_type = ropufu::sequential::hypotheses::likelihood<value_type>;
    using change_of_measure_type = ropufu::sequential::hypotheses::change_of_measure<value_type>;
    using observer_type = ropufu::sequential::hypotheses::observer<engine_type, value_type>;

    using rule_asprt_a_type = ropufu::sequential::hypotheses::adaptive_sprt<engine_type, value_type>;
    using rule_asprt_b_type = ropufu::sequential::hypotheses::adaptive_sprt<engine_type, value_type, ropufu::sequential::hypotheses::adaptive_sprt_flavor::general>;
    using rule_gsprt_a_type = ropufu::sequential::hypotheses::generalized_sprt<engine_type, value_type>;
    using rule_gsprt_b_type = ropufu::sequential::hypotheses::generalized_sprt<engine_type, value_type, ropufu::sequential::hypotheses::generalized_sprt_flavor::general>;
    using rule_dsprt_type = ropufu::sequential::hypotheses::double_sprt<engine_type, value_type>;

    engine_type engine {};
    ropufu::tests::seed(engine);

    ropufu::sequential::hypotheses::adaptive_sprt_design<value_type> asprt_simple_design {ropufu::sequential::hypotheses::adaptive_sprt_flavor::simple, 0};
    ropufu::sequential::hypotheses::adaptive_sprt_design<value_type> asprt_general_design {ropufu::sequential::hypotheses::adaptive_sprt_flavor::general, 1};
    ropufu::sequential::hypotheses::generalized_sprt_design<value_type> gsprt_cutoff_design {ropufu::sequential::hypotheses::generalized_sprt_flavor::cutoff, 2};
    ropufu::sequential::hypotheses::generalized_sprt_design<value_type> gsprt_general_design {ropufu::sequential::hypotheses::generalized_sprt_flavor::general, 3};
    ropufu::sequential::hypotheses::double_sprt_design<value_type> dsprt_asymp_design {4};
    ropufu::sequential::hypotheses::double_sprt_design<value_type> dsprt_huffman_design {5};

    dsprt_asymp_design.set_asymptotic_init(true);
    dsprt_huffman_design.set_huffman_correction(true);
    
    signal_type signal {1};
    noise_type noise {1};
    model_type mod {0, 1};
    likelihood_type like {mod};
    value_type actual_mu = mod.mu_under_null(); // smallest_mu_under_alt
    value_type analyzed_mu = actual_mu;
    process_type proc {signal, noise, actual_mu};
    change_of_measure_type signal_strength {analyzed_mu, actual_mu};

    rule_asprt_a_type asprt_simple {asprt_simple_design};
    rule_gsprt_a_type gsprt_cutoff {gsprt_cutoff_design};
    rule_asprt_b_type asprt_general {asprt_general_design};
    rule_gsprt_b_type gsprt_general {gsprt_general_design};
    rule_dsprt_type dsprt_asymp {dsprt_asymp_design};
    rule_dsprt_type dsprt_huffman {dsprt_huffman_design};

    value_type anticipated_run_length = 10.0;
    std::vector<value_type> null_thresholds = {3, 4, 5};
    std::vector<value_type> alt_thresholds = {4, 5, 6, 7};

    asprt_simple.initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    gsprt_cutoff.initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    asprt_general.initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    gsprt_general.initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    dsprt_asymp.initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    dsprt_huffman.initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);

    std::vector<observer_type*> os {
        &asprt_simple, &gsprt_cutoff,
        &asprt_general, &gsprt_general,
        &dsprt_asymp, &dsprt_huffman};

    for (observer_type* o : os) o->reset();
    bool is_listening = observer_type::any_listening(os);
    while (is_listening)
    {
        proc.tic(engine);
        like.tic(proc);
        for (observer_type* o : os) o->tic(proc, like);
        
        is_listening = observer_type::any_listening(os);
    } // for (...)

    REQUIRE(proc.count() > 0);

    REQUIRE(asprt_simple.state() == ropufu::sequential::hypotheses::two_sprt_state::decided);
    REQUIRE(gsprt_cutoff.state() == ropufu::sequential::hypotheses::two_sprt_state::decided);
    REQUIRE(asprt_general.state() == ropufu::sequential::hypotheses::two_sprt_state::decided);
    REQUIRE(gsprt_general.state() == ropufu::sequential::hypotheses::two_sprt_state::decided);
    REQUIRE(dsprt_asymp.state() == ropufu::sequential::hypotheses::two_sprt_state::decided);
    REQUIRE(dsprt_huffman.state() == ropufu::sequential::hypotheses::two_sprt_state::decided);

    for (observer_type* o : os) o->toc(proc, like, signal_strength);

    CHECK(asprt_simple.state() == ropufu::sequential::hypotheses::two_sprt_state::finalized);
    CHECK(gsprt_cutoff.state() == ropufu::sequential::hypotheses::two_sprt_state::finalized);
    CHECK(asprt_general.state() == ropufu::sequential::hypotheses::two_sprt_state::finalized);
    CHECK(gsprt_general.state() == ropufu::sequential::hypotheses::two_sprt_state::finalized);
    CHECK(dsprt_asymp.state() == ropufu::sequential::hypotheses::two_sprt_state::finalized);
    CHECK(dsprt_huffman.state() == ropufu::sequential::hypotheses::two_sprt_state::finalized);
} // TEST_CASE_TEMPLATE(...)

TEST_CASE_TEMPLATE("testing rule discrimination", value_t, float, long double)
{
    using a_type = ropufu::sequential::hypotheses::adaptive_sprt_design<value_t>;
    using b_type = ropufu::sequential::hypotheses::generalized_sprt_design<value_t>;
    using c_type = ropufu::sequential::hypotheses::double_sprt_design<value_t>;

    using variant_type = std::variant<a_type, b_type, c_type>;
    using w_type = ropufu::sequential::hypotheses::rule_design_variant<value_t>;

    a_type a {ropufu::sequential::hypotheses::adaptive_sprt_flavor::general, 5};
    b_type b {ropufu::sequential::hypotheses::generalized_sprt_flavor::general, 8};
    c_type c {13};

    variant_type a_variant = a;
    variant_type b_variant = b;
    variant_type c_variant = c;
    variant_type v {};

    nlohmann::json a_json = a;
    nlohmann::json b_json = b;
    nlohmann::json c_json = c;

    w_type a_var {};
    w_type b_var {};
    w_type c_var {};
    
    REQUIRE(ropufu::noexcept_json::try_get(a_json, a_var));
    REQUIRE(ropufu::noexcept_json::try_get(b_json, b_var));
    REQUIRE(ropufu::noexcept_json::try_get(c_json, c_var));
    REQUIRE(std::holds_alternative<a_type>(a_var));
    REQUIRE(std::holds_alternative<b_type>(b_var));
    REQUIRE(std::holds_alternative<c_type>(c_var));
    // CHECK(a_var.id() == a.id());
    // CHECK(b_var.id() == b.id());
    // CHECK(c_var.id() == c.id());

    REQUIRE(ropufu::noexcept_json::try_get(a_json, v));
    REQUIRE(std::holds_alternative<a_type>(v));
    CHECK(std::get<a_type>(v) == a);

    REQUIRE(ropufu::noexcept_json::try_get(b_json, v));
    REQUIRE(std::holds_alternative<b_type>(v));
    CHECK(std::get<b_type>(v) == b);

    REQUIRE(ropufu::noexcept_json::try_get(c_json, v));
    REQUIRE(std::holds_alternative<c_type>(v));
    CHECK(std::get<c_type>(v) == c);
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_RULES_HPP_INCLUDED
