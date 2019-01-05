
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_AUTOMATOR_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_AUTOMATOR_INCLUDED

#include <ropufu/algebra.hpp> // aftermath::algebra::matrix, aftermath::algebra::range
#include "../draft/algebra/interpolator.hpp"
#include "../draft/algebra/numbers.hpp"

#include "../hypotheses/de_auto_regress.hpp"
#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/process.hpp"
#include "../hypotheses/model.hpp"
#include "../hypotheses/rules.hpp"
#include "../hypotheses/monte_carlo.hpp"

#include "../hypotheses/moment_statistic.hpp"

#include "config.hpp"
#include "init_info.hpp"
#include "known_sprts.hpp"
#include "run.hpp"
#include "simulation_pair.hpp"
#include "writer.hpp"

#include <chrono>   // std::chrono::steady_clock, std::chrono::duration_cast
#include <cmath>    // std::sqrt
#include <cstddef>  // std::size_t
#include <iomanip>  // std::setw
#include <ios>      // std::left, std::right
#include <iostream> // std::cout, std::endl
#include <map>      // std::map
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string, std::to_string
#include <system_error> // std::error_code, std::make_error_code, std::errc
#include <utility>  // std::pair
#include <vector>   // std::vector

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        /** @brief Prints the corner values of simulation matrices.
         *  @param emat Matrix of expected values (sample means).
         *  @param emat Matrix of variances (sample variances).
         */
        template <typename t_data_type>
        void print_corners(
            const aftermath::algebra::matrix<t_data_type>& emat,
            const aftermath::algebra::matrix<t_data_type>& vmat,
            std::string&& prefix, std::size_t fixed_width = 15) noexcept
        {
            if (emat.size() == 0 || vmat.size() == 0)
            {
                std::cout << prefix << "empty" << std::endl;
                return;
            } // if (...)
            if (emat.height() != vmat.height() || emat.width() != vmat.width())
            {
                std::cout << prefix << "size mismatch" << std::endl;
                return;
            } // if (...)

            std::string blank(prefix.size(), ' ');
            std::size_t m = emat.height() - 1; // Cannot cause underflow because <emat> is not empty.
            std::size_t n = emat.width() - 1; // Cannot cause underflow because <emat> is not empty.

            //   a --- b      x --- y
            //   | ... |  pm  | ... |
            //   c --- d      z --- w
            t_data_type a = emat(0, n); t_data_type x = std::sqrt(vmat(0, n));
            t_data_type b = emat(m, n); t_data_type y = std::sqrt(vmat(m, n));
            t_data_type c = emat(0, 0); t_data_type z = std::sqrt(vmat(0, 0));
            t_data_type d = emat(m, 0); t_data_type w = std::sqrt(vmat(m, 0));

            std::cout << blank  << std::setw(fixed_width) << std::left << a << " --- " << std::setw(fixed_width) << std::right << b
                << "        "   << std::setw(fixed_width) << std::left << x << " --- " << std::setw(fixed_width) << std::right << y << std::endl;
            std::cout << prefix << std::setw(fixed_width) << std::left << " |" << " ... " << std::setw(fixed_width) << std::right << "| "
                << "   pm   "   << std::setw(fixed_width) << std::left << " |" << " ... " << std::setw(fixed_width) << std::right << "| " << std::endl;
            std::cout << blank  << std::setw(fixed_width) << std::left << c << " --- " << std::setw(fixed_width) << std::right << d
                << "        "   << std::setw(fixed_width) << std::left << z << " --- " << std::setw(fixed_width) << std::right << w << std::endl;
        } // print_corners(...)
    } // namespace detail

    /** @brief Class for reading and writing configurtation setting. */
    template <typename t_signal_type, typename t_noise_type>
    struct automator
    {
        using type = automator<t_signal_type, t_noise_type>;
        using de_auto_regress_type = hypotheses::detail::de_auto_regress<t_signal_type, t_noise_type>;

        using signal_type = t_signal_type;
        using noise_type = t_noise_type;
        using adjusted_signal_type = typename de_auto_regress_type::adjusted_signal_type;
        using adjusted_noise_type = typename de_auto_regress_type::adjusted_noise_type;
        using process_type = hypotheses::process<adjusted_signal_type, adjusted_noise_type>; // Adjusted process.
        using value_type = typename process_type::value_type;
        using model_type = hypotheses::model<value_type>;
        using rule_collection_type = hypotheses::known_sprts<adjusted_signal_type, adjusted_noise_type>;
        using monte_carlo_type = hypotheses::monte_carlo<adjusted_signal_type, adjusted_noise_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        using moment_statistic_type = hypotheses::moment_statistic<matrix_t<value_type>>;

        using congif_type = config<value_type>;

    private:
        adjusted_signal_type m_signal = {}; // Signal.
        adjusted_noise_type m_noise = {}; // Noise.
        rule_collection_type m_rules = {}; // Potential rules to run.
        std::string m_mat_output_path = {}; // Where to dump the statistic mat files.
        monte_carlo_type m_monte_carlo = {}; // Monte carlo.
        std::vector<run<value_type>> m_runs = {}; // MC simulations to perform.

        void build_runs(const congif_type& config) noexcept
        {
            std::vector<run<value_type>> keyframe_runs = config.runs(); // Copy runs from config.
            std::size_t count_in_between = config.interpolated_runs();
            this->m_runs = run<value_type>::interpolate(keyframe_runs, count_in_between);
        } // build_runs(...)

        bool try_execute(const model_type& model, const simulation_pair<value_type>& mu_pair,
            rule_collection_type& rules_to_run,
            const std::vector<init_info<value_type>>& rules_init,
            const hypothesis_pair<std::size_t>& threshold_count, aftermath::algebra::spacing threshold_spacing,
            bool is_verbal) noexcept
        {
            std::size_t k = rules_to_run.size();
            if (k != rules_init.size())
            {
                std::cout << "Rule init mismatch." << std::endl;
                return false;
            }
            if (k == 0) return true;

            if (is_verbal) std::cout
                << "Analyzed mu: " << mu_pair.analyzed_mu() << ", "
                << "simulated mu: " << mu_pair.simulated_mu() << std::endl;

            process_type proc(this->m_signal, this->m_noise, mu_pair.simulated_mu()); // Set up the process.

            std::chrono::steady_clock::time_point start {};
            std::chrono::steady_clock::time_point end {};

            std::error_code ec {};
            this->m_monte_carlo.run(proc, rules_to_run,
                [&] (std::error_code& ecx) {
                    if (is_verbal) std::cout << "Simulation start." << std::endl;
                    start = std::chrono::steady_clock::now();

                    for (const init_info<value_type>& init : rules_init)
                    {
                        if (is_verbal) 
                            std::cout << "Initializing rule #" << init.rule_id() << " with " << init << "." << std::endl;

                        std::vector<value_type> null_thresholds {};
                        std::vector<value_type> alt_thresholds {};
                        init.make_thresholds(threshold_count, threshold_spacing, null_thresholds, alt_thresholds);
                        rules_to_run.initialize(init,
                            model,
                            mu_pair.analyzed_mu(),
                            proc.log_likelihood_scale(), 
                            null_thresholds, alt_thresholds, ecx);
                        if (ecx.value() != 0) std::cout << "Initializing rule #" << init.rule_id() << "failed: " << ecx.message() << "." << std::endl;
                    } // for (...)
                },
                [&] () {
                    end = std::chrono::steady_clock::now();
                    rules_to_run.report([&] (std::size_t rule_id, const moment_statistic_type& run_lengths, const moment_statistic_type& errors)
                        {
                            // Report.
                            value_type temp {};

                            // ESS.
                            value_type ess_a = run_lengths.mean().front();
                            value_type ess_b = run_lengths.mean().back();
                            if (ess_b < ess_a) { temp = ess_b; ess_b = ess_a; ess_a = temp; }

                            // Errors.
                            value_type err_a = errors.mean().front();
                            value_type err_b = errors.mean().back();
                            if (err_b < err_a) { temp = err_b; err_b = err_a; err_a = temp; }

                            if (is_verbal) std::cout << "Rule #" << rule_id
                                << " ESS = " << ess_a << "--" << ess_b << ","
                                << " P(error) = " << err_a << "--" << err_b << "." << std::endl;
                        });

                    if (is_verbal) std::cout << "Simulation end." << std::endl;
                    value_type elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / static_cast<value_type>(1'000);

                    if (is_verbal) std::cout << "Elapsed time: " << elapsed_seconds << "s." << std::endl;
                }, ec); // run(...)
            return (ec.value() == 0);
        } // try_execute(...)

    public:
        automator() noexcept { }

        automator(const signal_type& signal, const noise_type& noise, const congif_type& config, std::error_code& ec) noexcept
            : m_signal(de_auto_regress_type::adjust_signal(signal, noise, ec)),
            m_noise(de_auto_regress_type::adjust_noise(signal, noise, ec)),
            m_rules(config.rules(), ec)
        {
            this->m_mat_output_path = config.mat_output_path();
            this->m_monte_carlo = monte_carlo_type(config.simulation_count());
            this->build_runs(config);
        } // automator(...)

        void execute(const congif_type& config) noexcept
        {
            for (const run<value_type>& r : this->m_runs)
            {
                std::error_code ec {};
                const model_type& model = r.model(); // Get the model.
                writer<value_type> w { this->m_mat_output_path, config, model, ec }; // Create .mat file writer.
                if (ec.value() != 0) std::cout << "Creating writer failed: " + ec.message() + "." << std::endl;
                if (ec.value() != 0) { std::cout << "Aborting." << std::endl; return; }

                // Match up rules to run.
                rule_collection_type rules_to_run = this->m_rules.filter(r.init_rules());

                // First, build up the operating characteristics.
                std::cout << "Model " << model << std::endl;
                std::cout << "Estimating operating characteristics in " << this->m_monte_carlo.count_simulations() << " Monte Carlo runs..." << std::endl;
                oc_array_t<void> oc_list {};
                for (operating_characteristic oc : oc_list)
                {
                    simulation_pair<value_type> mu_pair(oc, model, ec);
                    if (ec.value() != 0) std::cout << "Constructing simulation pair failed: " + ec.message() + "." << std::endl;
                    if (ec.value() != 0) { std::cout << "Aborting." << std::endl; return; }

                    bool is_okay = this->try_execute(model, mu_pair, rules_to_run, r.init_rules(), r.threshold_count(), r.threshold_spacing(), false); // Run the simulation, suppress display.
                    if (!is_okay) std::cout << "Something went wrong in " << __FUNCTION__ << " on line " << __LINE__ << "." << std::endl;
                    if (!is_okay) { std::cout << "Aborting." << std::endl; return; }

                    // Collect statistics to be stored later, once all OC simulations are completed.
                    rules_to_run.record_oc(oc, mu_pair, ec);
                    if (ec.value() != 0) std::cout << "Reading operating characteristic failed: " + ec.message() + "." << std::endl;
                    if (ec.value() != 0) { std::cout << "Aborting." << std::endl; return; }
                } // for (...)

                // Present the statistics.
                for (const oc_array_t<moment_statistic_type>& oc_statistic_array : rules_to_run.oc_statistics())
                {
                    const moment_statistic_type& fa = oc_statistic_array[operating_characteristic::probability_of_false_alarm];
                    const moment_statistic_type& ms = oc_statistic_array[operating_characteristic::probability_of_missed_signal];

                    matrix_t<value_type> pfa = fa.mean();
                    matrix_t<value_type> pms = ms.mean();
                    matrix_t<value_type> vfa = fa.variance();
                    matrix_t<value_type> vms = ms.variance();
                    //vfa.transform([] (value_type x) { return std::sqrt(x); });
                    //vms.transform([] (value_type x) { return std::sqrt(x); });
                    std::string rule_name = "??";
                    
                    std::cout << "Rule " << rule_name << ":" << std::endl;
                    detail::print_corners(pfa, vfa, "    " + std::to_string(operating_characteristic::probability_of_false_alarm) + " = ");
                    std::cout << std::endl;
                    detail::print_corners(pms, vms, "    " + std::to_string(operating_characteristic::probability_of_missed_signal) + " = ");
                    std::cout << std::endl;
                } // for (...)

                rules_to_run.dump_oc(w, model, ec); // Write to .mat file.
                if (ec.value() != 0) std::cout << "Writing operating characteristics failed: " + ec.message() + "." << std::endl;
                ec.clear();

                // Second, then run the auxiliary simulations.
                if (!r.simulation_pairs().empty())
                {
                    std::cout << "Estimating other characteristics..." << std::endl;
                    for (const simulation_pair<value_type>& mu_pair : r.simulation_pairs())
                    {
                        bool is_okay = this->try_execute(model, mu_pair, rules_to_run, r.init_rules(), r.threshold_count(), r.threshold_spacing(), true); // Run the simulation, allow display.
                        if (!is_okay) std::cout << "Something went wrong in " << __FUNCTION__ << " on line " << __LINE__ << "." << std::endl;
                        if (!is_okay) { std::cout << "Aborting." << std::endl; return; }

                        // Store the statistics right away.
                        rules_to_run.dump(w, model, mu_pair, ec); // Write to .mat file.
                        if (ec.value() != 0) std::cout << "Writing operating characteristics failed: " + ec.message() + "." << std::endl;
                        ec.clear();
                    } // for (...)
                } // if (...)
            } // for (...)
        } // execute(...)
    }; // automator
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_AUTOMATOR_INCLUDED
