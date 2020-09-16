
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_AUTOMATOR_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_AUTOMATOR_INCLUDED

#include <ropufu/number_traits.hpp>
#include <ropufu/algebra/matrix.hpp>
#include <ropufu/algebra/interval.hpp>
#include <ropufu/probability/moment_statistic.hpp>

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/simple_process.hpp"
#include "../hypotheses/operating_characteristic.hpp"
#include "../hypotheses/model.hpp"
#include "../hypotheses/change_of_measure.hpp"
#include "../hypotheses/observer.hpp"
#include "../hypotheses/rules.hpp"
#include "../hypotheses/monte_carlo.hpp"

#include "config.hpp"
#include "sprt_factory.hpp"
#include "asprt_limiting_distribution.hpp"

#include "init_info.hpp"
#include "run.hpp"
#include "writer.hpp"
#include "matrix_printer.hpp"

#include <chrono>   // std::chrono::steady_clock, std::chrono::duration_cast
#include <cstddef>  // std::size_t
#include <filesystem>   // std::filesystem::path
#include <iostream> // std::cout, std::endl
#include <map>      // std::map
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string, std::to_string
#include <system_error> // std::error_code, std::make_error_code, std::errc
#include <utility>  // std::swap
#include <vector>   // std::vector

namespace ropufu::sequential::hypotheses
{
    /** @brief Class for reading and writing configurtation setting. */
    template <typename t_engine_type, typename t_value_type>
    struct automator
    {
        using type = automator<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using signal_type = hypotheses::constant_signal<value_type>;
        using noise_type = hypotheses::white_noise<engine_type, value_type>;
        using process_type = hypotheses::simple_process<engine_type, value_type>;
        using model_type = hypotheses::model<value_type>;
        using change_of_measure_type = hypotheses::change_of_measure<value_type>;
        using monte_carlo_type = hypotheses::monte_carlo<engine_type, value_type>;

        using design_variant_type = hypotheses::rule_design_variant<value_type>;
        
        using observer_type = hypotheses::observer<engine_type, value_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        using moment_statistic_type = aftermath::probability::moment_statistic<matrix_t<value_type>>;
        using oc_statistic_type = hypotheses::oc_array_t<moment_statistic_type>;
        
        using init_info_type = hypotheses::init_info<value_type>;
        using run_type = run<value_type>;
        using congif_type = config<engine_type, value_type>;
        using writer_type = writer<engine_type, value_type>;
        using sprt_factory_type = sprt_factory<engine_type, value_type>;
        using observer_ptr_type = typename sprt_factory_type::observer_ptr_type;

    private:
        congif_type m_config = {};
        std::filesystem::path m_config_path = {};
        monte_carlo_type m_monte_carlo = {}; // Monte carlo.

        void execute(engine_type& engine, const run_type& r, const change_of_measure_type& mu_pair, std::vector<observer_ptr_type>& observer_pointers, bool is_verbal) noexcept
        {
            if (is_verbal)
            {
                std::cout << "Simulation start." << std::endl;
                std::cout << "-- Analyzed / simulated mu: " <<
                    mu_pair.analyzed() << " / " << mu_pair.simulated() << std::endl;
            } // if (...)

            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            process_type process {this->m_config.signal(), this->m_config.noise(), mu_pair.simulated()};
            this->m_monte_carlo.run(engine, process, r.model(), mu_pair, observer_pointers);
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

            if (is_verbal)
            {
                for (observer_ptr_type& o : observer_pointers)
                {
                    std::size_t m = o->run_lengths().mean().height();
                    std::size_t n = o->run_lengths().mean().width();

                    // Report.
                    std::string rule_name = o->to_path_string(2);

                    // ESS.
                    value_type ess_a = o->run_lengths().mean().at(0, 0);
                    value_type ess_b = o->run_lengths().mean().at(m - 1, n - 1);
                    if (ess_b < ess_a) std::swap(ess_a, ess_b);

                    // Errors.
                    value_type err_a = o->decision_errors().mean().at(0, 0);
                    value_type err_b = o->decision_errors().mean().at(m - 1, n - 1);
                    if (err_b < err_a) std::swap(err_a, err_b);

                    std::cout << "-- Rule " << rule_name
                        << " ESS = " << ess_a << "--" << ess_b << ","
                        << " P(error) = " << err_a << "--" << err_b << "." << std::endl;
                } // for (...)

                double elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / static_cast<double>(1'000);
                std::cout << "Simulation end." << std::endl;
                std::cout << "Elapsed time: " << elapsed_seconds << "s." << std::endl;
            } // if (...)
        } // execute(...)

        void estimate_limiting_distribution(engine_type& engine) noexcept
        {
            using limiting_distribution_type = hypotheses::asprt_limiting_distribution<engine_type, value_type>;

            std::size_t count_simulations = this->m_config.count_simulations();
            std::size_t count_observations = this->m_config.limiting_observations();
            std::size_t time_cutoff = this->m_config.limiting_cutoff_time();

            for (const run_type& r : this->m_config.runs())
            {
                std::cout << "Model " << r.model() << std::endl;

                limiting_distribution_type limit {this->m_config.mat_output_path(), r.model()};
                limit.touch(engine, count_simulations, count_observations, time_cutoff);
            } // for (...)
        } // estimate_limiting_distribution(...)

    public:
        explicit automator(const congif_type& config, const std::filesystem::path& config_path) noexcept
            : m_config(config), m_config_path(config_path), m_monte_carlo(config.count_simulations())
        {
        } // automator(...)

        void execute(engine_type& engine) noexcept
        {
            using matrix_printer_type = matrix_printer<value_type>;
            process_type proc {this->m_config.signal(), this->m_config.noise()};

            if (this->m_config.do_limiting_distribution())
            {
                this->estimate_limiting_distribution(engine);
                return;
            } // if (...)

            for (const run_type& r : this->m_config.runs())
            {
                sprt_factory_type factory {proc};

                // Set up rules.
                for (const init_info_type& init : r.inits())
                {
                    if (!this->m_config.has_rule_design(init.rule_id())) continue;

                    factory.initialize_visitor(init, r.model(), r.threshold_spacing(), r.threshold_count());
                    const design_variant_type& v = this->m_config.rule_design_by_id(init.rule_id());
                    std::visit(factory, v);
                } // for (...)

                // Now that the rules have been set up, treat them as observers.
                std::vector<observer_ptr_type> observer_pointers = factory.observer_pointers();

                // Statistics for standard operating characteristics.
                std::vector<oc_statistic_type> oc_statistics = {};
                oc_statistics.resize(observer_pointers.size()); // One default-initialized oc_array_t<moment_statistic_type> for each rule.

                writer_type w {this->m_config.mat_output_path(), this->m_config_path, r.model()};
                if (!w.good()) std::cout << "Creating writer failed. No filesystem output will be produced." << std::endl;

                std::cout << "Model " << r.model() << std::endl;
                std::cout << "Estimating operating characteristics in " << this->m_monte_carlo.count_simulations() << " Monte Carlo runs..." << std::endl;

                // First, build up the standard operating characteristics.
                if (!this->m_config.disable_oc_pass())
                {
                    oc_array_t<void> oc_list {};
                    for (operating_characteristic oc : oc_list)
                    {
                        // Determine change of measure.
                        change_of_measure_type mu_pair = change_of_measure_type::from_oc(oc, r.model());
                        // Run simulations.
                        this->execute(engine, r, mu_pair, observer_pointers, false); // Run the simulation, suppress display.

                        // Collect statistics to be stored later, once all OC simulations are completed.
                        
                        for (std::size_t k = 0; k < observer_pointers.size(); ++k)
                        {
                            const observer_ptr_type& o = observer_pointers[k];
                            oc_statistic_type& s = oc_statistics[k];
                            s[oc] = o->read_oc(oc);
                        } // for (...)
                    } // for (...)

                    // Present the statistics.
                    for (std::size_t k = 0; k < oc_statistics.size(); ++k)
                    {
                        const oc_array_t<moment_statistic_type>& oc_statistic_array = oc_statistics[k];
                        const moment_statistic_type& fa = oc_statistic_array[operating_characteristic::probability_of_false_alarm];
                        const moment_statistic_type& ms = oc_statistic_array[operating_characteristic::probability_of_missed_signal];

                        matrix_t<value_type> pfa = fa.mean();
                        matrix_t<value_type> pms = ms.mean();
                        matrix_t<value_type> vfa = fa.variance();
                        matrix_t<value_type> vms = ms.variance();
                        //vfa.transform([] (value_type x) { return std::sqrt(x); });
                        //vms.transform([] (value_type x) { return std::sqrt(x); });
                        std::string rule_name = observer_pointers[k]->to_path_string(2);
                        
                        std::cout << "Rule " << rule_name << ":" << std::endl;
                        matrix_printer_type::print_corners(std::cout, pfa, vfa, "    " + std::to_string(operating_characteristic::probability_of_false_alarm) + " = ");
                        std::cout << std::endl;
                        matrix_printer_type::print_corners(std::cout, pms, vms, "    " + std::to_string(operating_characteristic::probability_of_missed_signal) + " = ");
                        std::cout << std::endl;
                    } // for (...)

                    // Store the statistics to filesystem.
                    for (std::size_t k = 0; k < oc_statistics.size(); ++k)
                    {
                        const oc_array_t<moment_statistic_type>& oc_statistic_array = oc_statistics[k];
                        w.write_mat(this->m_config.count_simulations(), observer_pointers[k], oc_statistic_array);
                    } // for (...)
                } // if (...)

                // Second, then run the auxiliary simulations.
                if (!r.signal_strengths().empty() && !this->m_config.disable_gray_pass())
                {
                    std::cout << "Estimating other characteristics..." << std::endl;
                    for (const change_of_measure_type& mu_pair : r.signal_strengths())
                    {
                        // Run simulations.
                        this->execute(engine, r, mu_pair, observer_pointers, true); // Run the simulation, allow display.
                        // Store the statistics right away.
                        for (std::size_t k = 0; k < observer_pointers.size(); ++k)
                        {
                            w.write_mat(this->m_config.count_simulations(), observer_pointers[k], mu_pair);
                        } // for (...)
                    } // for (...)
                } // if (...)
            } // for (...)
        } // execute(...)
    }; // automator
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_AUTOMATOR_INCLUDED
