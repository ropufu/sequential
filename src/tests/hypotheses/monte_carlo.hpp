
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED

#include <doctest/doctest.h>

#include "../core.hpp"
#include "../../hypotheses/signals.hpp"
#include "../../hypotheses/noises.hpp"
#include "../../hypotheses/simple_process.hpp"
#include "../../hypotheses/change_of_measure.hpp"

#include "../../hypotheses/model.hpp"
#include "../../hypotheses/likelihood.hpp"
#include "../../hypotheses/observer.hpp"
#include "../../hypotheses/rules.hpp"
#include "../../hypotheses/monte_carlo.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <memory>     // std::shared_ptr, std::make_shared
#include <random>     // std::mt19937
#include <stdexcept>  // std::logic_error
#include <vector>     // std::vector


#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_MONTE_CARLO_NOISE_TYPES            \
    ropufu::sequential::hypotheses::white_noise<std::ranlux24, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::ranlux48, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937, double>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937_64, long double> \


TEST_CASE_TEMPLATE("testing monte carlo", noise_type, ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_MONTE_CARLO_NOISE_TYPES)
{
    using engine_type = typename noise_type::engine_type;
    using value_type = typename noise_type::value_type;
    using signal_type = ropufu::sequential::hypotheses::constant_signal<value_type>;
    using process_type = ropufu::sequential::hypotheses::simple_process<engine_type, value_type>;
    using model_type = ropufu::sequential::hypotheses::model<value_type>;
    using likelihood_type = ropufu::sequential::hypotheses::likelihood<value_type>;
    using change_of_measure_type = ropufu::sequential::hypotheses::change_of_measure<value_type>;
    using observer_type = ropufu::sequential::hypotheses::observer<engine_type, value_type>;
    using monte_carlo_type = ropufu::sequential::hypotheses::monte_carlo<engine_type, value_type>;

    using rule_asprt_a_type = ropufu::sequential::hypotheses::adaptive_sprt<engine_type, value_type, ropufu::sequential::hypotheses::adaptive_sprt_flavor::simple>;
    using rule_asprt_b_type = ropufu::sequential::hypotheses::adaptive_sprt<engine_type, value_type, ropufu::sequential::hypotheses::adaptive_sprt_flavor::general>;
    using rule_gsprt_a_type = ropufu::sequential::hypotheses::generalized_sprt<engine_type, value_type, ropufu::sequential::hypotheses::generalized_sprt_flavor::cutoff>;
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
    change_of_measure_type change_of_measure {analyzed_mu, actual_mu};
    monte_carlo_type mc {100};

    auto asprt_simple = std::make_shared<rule_asprt_a_type>(asprt_simple_design);
    auto gsprt_cutoff = std::make_shared<rule_gsprt_a_type>(gsprt_cutoff_design);
    auto asprt_general = std::make_shared<rule_asprt_b_type>(asprt_general_design);
    auto gsprt_general = std::make_shared<rule_gsprt_b_type>(gsprt_general_design);
    auto dsprt_asymp = std::make_shared<rule_dsprt_type>(dsprt_asymp_design);
    auto dsprt_huffman = std::make_shared<rule_dsprt_type>(dsprt_huffman_design);

    value_type anticipated_run_length = 10.0;
    std::vector<value_type> null_thresholds = {3, 4, 5};
    std::vector<value_type> alt_thresholds = {4, 5, 6, 7};

    asprt_simple->initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    gsprt_cutoff->initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    asprt_general->initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    gsprt_general->initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    dsprt_asymp->initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);
    dsprt_huffman->initialize(mod, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds);

    std::vector<std::shared_ptr<observer_type>> os {
        asprt_simple, gsprt_cutoff,
        asprt_general, gsprt_general,
        dsprt_asymp, dsprt_huffman};

    mc.run(engine, proc, mod, change_of_measure, os);

    REQUIRE(proc.count() > 0);
    for (const std::shared_ptr<observer_type>& o : os) REQUIRE(o->decision_errors().count() == o->run_lengths().count());
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED
