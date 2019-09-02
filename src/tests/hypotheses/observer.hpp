
#ifndef ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_OBSERVER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_OBSERVER_HPP_INCLUDED

#include <doctest/doctest.h>
#include <ropufu/algebra/matrix.hpp>

#include "../core.hpp"
#include "../../draft/probability/moment_statistic.hpp"

#include "../../hypotheses/signals.hpp"
#include "../../hypotheses/noises.hpp"
#include "../../hypotheses/simple_process.hpp"
#include "../../hypotheses/change_of_measure.hpp"

#include "../../hypotheses/model.hpp"
#include "../../hypotheses/likelihood.hpp"
#include "../../hypotheses/observer.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <random>     // std::mt19937
#include <stdexcept>  // std::logic_error
#include <vector>     // std::vector

namespace ropufu::sequential::tests
{
    template <typename t_engine_type, typename t_value_type, std::size_t t_stopping_time>
    struct september : hypotheses::observer<t_engine_type, t_value_type>
    {
        using process_type = hypotheses::simple_process<t_engine_type, t_value_type>;
        using likelihood_type = hypotheses::likelihood<t_value_type>;
        using change_of_measure_type = hypotheses::change_of_measure<t_value_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        using moment_statistic_type = aftermath::probability::moment_statistic<matrix_t<t_value_type>>;

        std::size_t count = 0;
        std::vector<t_value_type> empty_vector = {};
        moment_statistic_type empty_statistic = {};

        bool is_listening() const noexcept override { return this->count < t_stopping_time; }
        void reset() override { this->count = 0; }
        void clean_up() override { this->count = 0; }
        void tic(const process_type&, const likelihood_type&) override { if (this->is_listening()) ++(this->count); }
        void toc(const process_type&, const likelihood_type&, const change_of_measure_type&) override { }
        
        const std::vector<t_value_type>& unscaled_null_thresholds() const noexcept override { return this->empty_vector; }
        const std::vector<t_value_type>& unscaled_alt_thresholds() const noexcept override { return this->empty_vector; }

        const moment_statistic_type& decision_errors() const noexcept override { return this->empty_statistic; }
        const moment_statistic_type& run_lengths() const noexcept override { return this->empty_statistic; }
    }; // struct september
} // namespace ropufu::sequential::tests

#define ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_PROCESS_NOISE_TYPES                \
    ropufu::sequential::hypotheses::white_noise<std::ranlux24, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::ranlux48, float>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937, double>,        \
    ropufu::sequential::hypotheses::white_noise<std::mt19937_64, long double> \


TEST_CASE_TEMPLATE("testing observer", noise_type, ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_PROCESS_NOISE_TYPES)
{
    using engine_type = typename noise_type::engine_type;
    using value_type = typename noise_type::value_type;
    using signal_type = ropufu::sequential::hypotheses::constant_signal<value_type>;
    using process_type = ropufu::sequential::hypotheses::simple_process<engine_type, value_type>;
    using model_type = ropufu::sequential::hypotheses::model<value_type>;
    using likelihood_type = ropufu::sequential::hypotheses::likelihood<value_type>;
    using change_of_measure_type = ropufu::sequential::hypotheses::change_of_measure<value_type>;
    using observer_type = ropufu::sequential::hypotheses::observer<engine_type, value_type>;

    engine_type engine {};
    ropufu::sequential::tests::seed(engine);

    signal_type signal_one {1};
    noise_type white_two {2};
    model_type mod {0, 3};
    likelihood_type like {mod};
    process_type process {signal_one, white_two, mod.mu_under_null()};
    change_of_measure_type change_of_measure {mod.mu_under_null(), process.signal_strength()};

    REQUIRE(ropufu::sequential::tests::does_json_round_trip(mod));
    
    ropufu::sequential::tests::september<engine_type, value_type, 17> o1 {};
    ropufu::sequential::tests::september<engine_type, value_type, 8> o2 {};
    std::vector<observer_type*> os {&o1, &o2};

    bool is_listening = observer_type::any_listening(os);
    while (is_listening && process.count() < 1729)
    {
        process.tic(engine);
        like.tic(process);
        for (observer_type* o : os) o->tic(process, like);
        
        is_listening = observer_type::any_listening(os);
    } // for (...)
    for (observer_type* o : os) o->toc(process, like, change_of_measure);

    REQUIRE(process.count() == 17);
    REQUIRE(o1.count == 17);
    REQUIRE(o2.count == 8);
} // TEST_CASE_TEMPLATE(...)

#endif // ROPUFU_SEQUENTIAL_TESTS_HYPOTHESES_OBSERVER_HPP_INCLUDED
