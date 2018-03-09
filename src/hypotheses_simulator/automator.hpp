
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_AUTOMATOR_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_AUTOMATOR_INCLUDED

#include <aftermath/algebra.hpp>      // aftermath::algebra::matrix
#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include "../hypotheses/de_auto_regress.hpp"
#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/process.hpp"
#include "../hypotheses/model.hpp"
#include "../hypotheses/rules.hpp"
#include "../hypotheses/monte_carlo.hpp"

#include "../hypotheses/moment_statistic.hpp"
#include "../hypotheses/modules/interpolator.hpp"
#include "../hypotheses/modules/numbers.hpp"

#include "config.hpp"
#include "run.hpp"
#include "init_info.hpp"
#include "simulation_pair.hpp"
#include "writer.hpp"

#include <chrono>   // std::chrono::steady_clock, std::chrono::duration_cast
#include <cmath>    // std::sqrt
#include <cstddef>  // std::size_t
#include <iomanip>  // std::setw
#include <ios>      // std::left, std::right
#include <iostream> // std::cout, std::endl
#include <map>      // std::map
#include <string>   // std::string, std::to_string
#include <utility>  // std::pair
#include <vector>   // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
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
                    t_data_type a = emat.unchecked_at(0, n); t_data_type x = std::sqrt(vmat.unchecked_at(0, n));
                    t_data_type b = emat.unchecked_at(m, n); t_data_type y = std::sqrt(vmat.unchecked_at(m, n));
                    t_data_type c = emat.unchecked_at(0, 0); t_data_type z = std::sqrt(vmat.unchecked_at(0, 0));
                    t_data_type d = emat.unchecked_at(m, 0); t_data_type w = std::sqrt(vmat.unchecked_at(m, 0));

                    std::cout << blank  << std::setw(fixed_width) << std::left << a << " --- " << std::setw(fixed_width) << std::right << b
                        << "        "   << std::setw(fixed_width) << std::left << x << " --- " << std::setw(fixed_width) << std::right << y << std::endl;
                    std::cout << prefix << std::setw(fixed_width) << std::left << " |" << " ... " << std::setw(fixed_width) << std::right << "| "
                        << "   pm   "   << std::setw(fixed_width) << std::left << " |" << " ... " << std::setw(fixed_width) << std::right << "| " << std::endl;
                    std::cout << blank  << std::setw(fixed_width) << std::left << c << " --- " << std::setw(fixed_width) << std::right << d
                        << "        "   << std::setw(fixed_width) << std::left << z << " --- " << std::setw(fixed_width) << std::right << w << std::endl;
                } // print_corners(...)
            } // namespace detail

            /** @brief Class for reading and writing configurtation setting. */
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check = true>
            struct automator
            {
                using type = automator<t_signal_type, t_noise_type, t_sync_check>;
                using de_auto_regress_type = hypotheses::detail::de_auto_regress<t_signal_type, t_noise_type>;

                using signal_type = t_signal_type;
                using noise_type = t_noise_type;
                using adjusted_signal_type = typename de_auto_regress_type::adjusted_signal_type;
                using adjusted_noise_type = typename de_auto_regress_type::adjusted_noise_type;
                using process_type = hypotheses::process<adjusted_signal_type, adjusted_noise_type>; // Adjusted process.
                using value_type = typename process_type::value_type;
                using model_type = hypotheses::model<value_type>;
                using rule_type = hypotheses::xsprt<adjusted_signal_type, adjusted_noise_type, t_sync_check>;
                using monte_carlo_type = hypotheses::monte_carlo<adjusted_signal_type, adjusted_noise_type>;

                template <typename t_data_type>
                using matrix_t = aftermath::algebra::matrix<t_data_type>;
                using statistic_type = hypotheses::moment_statistic<matrix_t<value_type>>;

                using congif_type = config<value_type>;

            private:
                std::string m_mat_output_path = { }; // Where to dump the statistic mat files.
                adjusted_signal_type m_signal = { }; // Signal.
                adjusted_noise_type m_noise = { }; // Noise.
                monte_carlo_type m_monte_carlo = { }; // Monte carlo.
                std::map<std::size_t, rule_type> m_rules = { }; // Potential rules to run.
                std::vector<run<value_type>> m_runs = { }; // MC simulations to perform.

                void build_rules(const congif_type& config) noexcept
                {
                    this->m_rules.clear();
                    for (const typename congif_type::rule_type& x : config.rules()) this->m_rules.emplace(x.id(), x);
                } // build_rules(...)

                void build_runs(const congif_type& config) noexcept
                {
                    std::vector<run<value_type>> keyframe_runs = config.runs(); // Copy runs from config.
                    std::size_t count_in_between = config.interpolated_runs();
                    this->m_runs = run<value_type>::interpolate(keyframe_runs, count_in_between);
                } // build_runs(...)

                void execute(const model_type& model, const simulation_pair<value_type>& mu_pair,
                    std::vector<rule_type>& rules_to_run,
                    std::vector<init_info<value_type>>& rules_init,
                    std::size_t threshold_count, const std::string& threshold_spacing,
                    bool is_verbal) noexcept
                {
                    std::size_t k = rules_to_run.size();
                    if (k != rules_init.size())
                    {
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "Rule init mismatch.", __FUNCTION__, __LINE__);
                        return;
                    }
                    if (k == 0) return;

                    if (is_verbal) std::cout
                        << "Analyzed mu: " << mu_pair.analyzed_mu() << ", "
                        << "simulated mu: " << mu_pair.simulated_mu() << std::endl;

                    process_type proc(this->m_signal, this->m_noise, mu_pair.simulated_mu()); // Set up the process.

                    std::chrono::steady_clock::time_point start { };
                    std::chrono::steady_clock::time_point end { };

                    this->m_monte_carlo.run(proc, rules_to_run,
                        [&] () {
                            if (is_verbal) std::cout << "Simulation start." << std::endl;
                            start = std::chrono::steady_clock::now();
                            for (std::size_t i = 0; i < k; ++i)
                            {
                                rule_type& rule = rules_to_run[i];
                                init_info<value_type>& init = rules_init[i];
                                if (is_verbal) std::cout << "Initializing rule #" << rule.id() << " with " << init << "." << std::endl;

                                std::vector<value_type> null_thresholds { };
                                std::vector<value_type> alt_thresholds { };
                                init.make_thresholds(threshold_count, threshold_spacing, null_thresholds, alt_thresholds);
                                rule.initialize(model, mu_pair.analyzed_mu(), init.anticipated_run_length(), proc, null_thresholds, alt_thresholds);
                            } // for (...)
                        },
                        [&] () {
                            end = std::chrono::steady_clock::now();
                            for (std::size_t i = 0; i < k; ++i)
                            {
                                rule_type& rule = rules_to_run[i];

                                // Report.
                                value_type temp { };

                                // ESS.
                                value_type ess_a = rule.run_lengths().mean().front();
                                value_type ess_b = rule.run_lengths().mean().back();
                                if (ess_b < ess_a) { temp = ess_b; ess_b = ess_a; ess_a = temp; }

                                // Errors.
                                value_type err_a = rule.errors().mean().front();
                                value_type err_b = rule.errors().mean().back();
                                if (err_b < err_a) { temp = err_b; err_b = err_a; err_a = temp; }

                                if (is_verbal) std::cout << "Rule #" << rule.id()
                                    << " ESS = " << ess_a << "--" << ess_b << ","
                                    << " P(error) = " << err_a << "--" << err_b << "." << std::endl;
                            } // for (...)
                            if (is_verbal) std::cout << "Simulation end." << std::endl;
                            value_type elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / static_cast<value_type>(1'000);

                            if (is_verbal) std::cout << "Elapsed time: " << elapsed_seconds << "s." << std::endl;
                        }); // run(...)
                } // execute(...)

            public:
                automator() noexcept { }

                automator(const signal_type& signal, const noise_type& noise) noexcept
                    : m_signal(de_auto_regress_type::adjust_signal(signal, noise)), m_noise(de_auto_regress_type::adjust_noise(signal, noise))
                {
                    const congif_type& config = congif_type::instance();

                    this->m_mat_output_path = config.mat_output_path();
                    this->m_monte_carlo = monte_carlo_type(config.simulation_count());
                    this->build_rules(config);
                    this->build_runs(config);
                } // automator(...)

                void execute() noexcept
                {
                    for (const run<value_type>& r : this->m_runs)
                    {
                        const model_type& model = r.model(); // Get the model.
                        writer<value_type> w(this->m_mat_output_path, model); // Create .mat file writer.

                        // Match up rules to run.
                        std::vector<rule_type> rules_to_run { };
                        std::vector<init_info<value_type>> rules_init { };
                        rules_to_run.reserve(this->m_rules.size());
                        rules_init.reserve(this->m_rules.size());
                        for (const std::pair<std::size_t, init_info<value_type>>& item : r.init_rules())
                        {
                            std::size_t rule_id = item.first;
                            auto match_iterator = this->m_rules.find(rule_id);
                            if (match_iterator != this->m_rules.end())
                            {
                                rules_to_run.push_back(match_iterator->second);
                                rules_init.push_back(item.second);
                            } // if (...)
                        } // for (...)

                        // First, build up the operating characteristics.
                        std::cout << "Model " << model << std::endl;
                        std::cout << "Estimating operating characteristics in " << this->m_monte_carlo.count_simulations() << " Monte Carlo runs..." << std::endl;
                        oc_array<void> oc_list { };
                        std::vector<oc_array<statistic_type>> oc_statistics(rules_to_run.size()); // One default-initialized <oc_array> for each rule.
                        for (operating_characteristic oc : oc_list)
                        {
                            simulation_pair<value_type> mu_pair(oc, model);
                            this->execute(model, mu_pair, rules_to_run, rules_init, r.threshold_count(), r.threshold_spacing(), false); // Run the simulation, suppress display.

                            // Collect statistics to be stored later, once all OC simulations are completed.
                            for (std::size_t k = 0; k < rules_to_run.size(); ++k)
                            {
                                const rule_type& rule = rules_to_run[k];
                                oc_array<statistic_type>& oc_statistic_array = oc_statistics[k];
                                oc_statistic_array[oc] = mu_pair.read_oc(oc, rule);
                            } // for (...)
                        } // for (...)

                        // Present the statistics.
                        for (std::size_t k = 0; k < rules_to_run.size(); ++k)
                        {
                            const rule_type& rule = rules_to_run[k];
                            const oc_array<statistic_type>& oc_statistic_array = oc_statistics[k];

                            const statistic_type& fa = oc_statistic_array[operating_characteristic::probability_of_false_alarm];
                            const statistic_type& ms = oc_statistic_array[operating_characteristic::probability_of_missed_signal];

                            matrix_t<value_type> pfa = fa.mean();
                            matrix_t<value_type> pms = ms.mean();
                            matrix_t<value_type> vfa = fa.variance();
                            matrix_t<value_type> vms = ms.variance();
                            //vfa.transform([] (value_type x) { return std::sqrt(x); });
                            //vms.transform([] (value_type x) { return std::sqrt(x); });
                            
                            std::cout << "Rule " << rule << ":" << std::endl;
                            detail::print_corners(pfa, vfa, "    " + std::to_string(operating_characteristic::probability_of_false_alarm) + " = ");
                            std::cout << std::endl;
                            detail::print_corners(pms, vms, "    " + std::to_string(operating_characteristic::probability_of_missed_signal) + " = ");
                            std::cout << std::endl;

                            w.write_mat(model, rule, oc_statistic_array); // Write to .mat file.
                        } // for (...)

                        // Second, then run the auxiliary simulations.
                        if (!r.simulation_pairs().empty())
                        {
                            std::cout << "Estimating other characteristics..." << std::endl;
                            for (const simulation_pair<value_type>& mu_pair : r.simulation_pairs())
                            {
                                this->execute(model, mu_pair, rules_to_run, rules_init, r.threshold_count(), r.threshold_spacing(), true); // Run the simulation, allow display.
                                // Store the statistics right away.
                                for (std::size_t k = 0; k < rules_to_run.size(); ++k)
                                {
                                    const rule_type& rule = rules_to_run[k];
                                    w.write_mat(model, rule, mu_pair); // Write to .mat file.
                                } // for (...)
                            } // for (...)
                        } // if (...)
                    } // for (...)
                } // execute(...)
            }; // automator
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_AUTOMATOR_INCLUDED
