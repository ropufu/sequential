
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_STOPPING_TIME_OBSERVER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_STOPPING_TIME_OBSERVER_HPP_INCLUDED

#include <aftermath/algebra.hpp>
#include <aftermath/not_an_error.hpp>
#include <aftermath/template_math.hpp>

#include "moment_statistic.hpp"
#include "process.hpp"

#include <cmath>
#include <cstddef>
#include <iostream> // For std::cout.
#include <string>
#include <utility>  // For std::forward.

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** A wrapper around stopping time to keep track of its operating characteristics. */
            template <typename t_stopping_time_type>
            struct stopping_time_observer
            {
                using type = stopping_time_observer<t_stopping_time_type>;
                using stopping_type = t_stopping_time_type;
                using signal_type = typename stopping_type::signal_type;
                using model_type = typename stopping_type::model_type;
                using process_type = typename stopping_type::process_type;

                template <typename t_data_type>
                using matrix_t = aftermath::algebra::matrix<t_data_type>;
                
                using statistic_type = moment_statistic<matrix_t<double>>;

                /** Scaling factor for displaying error probabilities. */
                static constexpr std::size_t error_factor = 100'000;
                /** Suffix for displaying error probabilities. */
                static constexpr std::size_t error_suffix = aftermath::log_base_n<error_factor, 10>::value;
            
            private:
                std::string m_type_name = ""; // Type for storage purposes.
                std::string m_name = ""; // Name for display purposes.
                double m_analyzed_mu = 0; // The signal "strength" conrresponding to what measure we want to analyze.
                double m_expected_run_length = 0; // An auxiliary quantity to improve accuracy of statistics.
                stopping_type m_stopping_time; // Stopping time to be analyzed.
                statistic_type m_errors; // Track erroneous decisions made by the stopping time.
                statistic_type m_run_lengths; // Track run lengths of the stopping time.
            
            public:
                /** @brief Create a stopping time observer.
                 *  @remark Forwards the arguments to the stopping time constructor.
                 */
                template <typename... t_args>
                stopping_time_observer(t_args&&... args) noexcept
                    : m_stopping_time(std::forward<t_args>(args)...),
                    m_errors(),
                    m_run_lengths()
                {
                }

                /** @brief Create a stopping time observer.
                 *  @remark Also defines default conversion.
                 */
                stopping_time_observer(stopping_type&& stopping_time) noexcept
                    : m_stopping_time(stopping_time),
                    m_errors(),
                    m_run_lengths()
                {
                }

                /** @brief Create a stopping time observer.
                 *  @remark Also defines default conversion.
                 */
                stopping_time_observer(const stopping_type& stopping_time) noexcept
                    : m_stopping_time(stopping_time),
                    m_errors(),
                    m_run_lengths()
                {
                }

                /** Name as it will appear when sending to a mat file. */
                const std::string& type_name() const noexcept { return this->m_type_name; }
                /** Name as it will appear when sending to a mat file. */
                void set_type_name(const std::string& value) noexcept { this->m_type_name = value; }

                /** Name as it will appear when sending to an output stream. */
                const std::string& name() const noexcept { return this->m_name; }
                /** Name as it will appear when sending to an output stream. */
                void set_name(const std::string& value) noexcept { this->m_name = value; }

                /** Set the signal "strength" conrresponding to what measure we want to analyze and an auxiliary quantity to improve accuracy of statistics. */
                void look_for(double analyzed_mu, double expected_run_length) noexcept 
                { 
                    this->m_analyzed_mu = analyzed_mu;
                    this->m_expected_run_length = expected_run_length;
                }

                /** Signal "strength" conrresponding to what measure we want to analyze. */
                double analyzed_mu() const noexcept { return this->m_analyzed_mu; }
                /** An auxiliary quantity to improve accuracy of statistics. */
                double expected_run_length() const noexcept { return this->m_expected_run_length; }
            
                /** Stopping time to be analyzed. */
                const stopping_type& stopping_time() const noexcept { return this->m_stopping_time; }
                /** Track erroneous decisions made by the stopping time. */
                const statistic_type& errors() const noexcept { return this->m_errors; }
                /** Track run lengths of the stopping time. */
                const statistic_type& run_lengths() const noexcept { return this->m_run_lengths; }

                /** Clears all the observations and resets the underlying stopping time. */
                void clear() noexcept
                {
                    this->m_errors.clear();
                    this->m_run_lengths.clear();

                    this->m_stopping_time.reset();
                }
            
                /** @brief Set the thresholds for the underlying stopping type.
                 *  @remark Forwards the arguments to the stopping time.
                 */
                template <typename... t_args>
                void set_thresholds(t_args&&... args) noexcept 
                { 
                    this->m_stopping_time.set_thresholds(std::forward<t_args>(args)...);
                    if (!aftermath::quiet_error::instance().good()) return;

                    // Now that the stopping time has verified the threshold structure, go on and resize the empirical measures accordingly.
                    std::size_t m = this->m_stopping_time.run_lengths().height();
                    std::size_t n = this->m_stopping_time.run_lengths().width();
                    matrix_t<double> zero(m, n);
                    matrix_t<double> expected_run_length(m, n, this->m_expected_run_length);

                    this->m_errors = statistic_type(zero, zero); // Align cross indicators to match threshold size.
                    this->m_run_lengths = statistic_type(zero, expected_run_length); // Align observation counts to match threshold size.
                }
            
                /** Indicates if the underlying stopping time is still running. */
                bool is_running() const noexcept { return this->m_stopping_time.is_running(); }
            
                /** @brief Collect another obseration from the \p proc.
                 *  @remark Forwards the argument to the stopping time.
                 */
                void tic(const process_type& proc) noexcept { this->m_stopping_time.observe(proc); }
            
                /** Builds operating characteristics of the stopped rule and resets it for the next simulation. */
                void toc(const process_type& proc) noexcept
                {
                    const matrix_t<std::size_t>& run_lengths = this->m_stopping_time.run_lengths();
                    const matrix_t<bool>& have_crossed_null = this->m_stopping_time.have_crossed_null();
                    const matrix_t<bool>& have_crossed_alt = this->m_stopping_time.have_crossed_alt();

                    const model_type& model = proc.model();
                    bool is_null_true = model.is_null(this->m_analyzed_mu);
                    bool is_alt_true = model.is_alt(this->m_analyzed_mu);

                    // Count the thresholds.
                    std::size_t m = run_lengths.height();
                    std::size_t n = run_lengths.width();

                    matrix_t<double> corrected_run_lengths(m, n);
                    matrix_t<double> corrected_errors(m, n);
                    for (std::size_t i = 0; i < m; i++)
                    {
                        for (std::size_t j = 0; j < n; j++)
                        {
                            bool has_crossed_null = have_crossed_null.at(i, j);
                            bool has_crossed_alt = have_crossed_alt.at(i, j);
                            // Check if the stopping time has stopped!
                            if (!has_crossed_null && !has_crossed_alt)
                            {
                                aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, "The procedure has not stopped!", __FUNCTION__, __LINE__);
                                this->m_stopping_time.reset(); // If not, reset the rule anyway.
                                return;
                            }
                    
                            std::size_t run_length = run_lengths.at(i, j);
                            std::size_t error = 0;
                            if (has_crossed_null && has_crossed_alt) error = 1;
                            if (has_crossed_null && is_alt_true) error = 1;
                            if (has_crossed_alt && is_null_true) error = 1;
                            //std::size_t error_relaxed = has_crossed_null ? (is_null_true ? 0 : 1) : (is_null_true ? 1 : 0);
                    
                            double correction = std::exp(proc.unscaled_log_likelihood_between(proc.actual_mu(), this->m_analyzed_mu, run_length) / proc.log_likelihood_scale());
                            double t = run_length / correction;
                            double e = error / correction;
                            corrected_run_lengths.at(i, j) = t;
                            corrected_errors.at(i, j) = e;
                        }
                    }
                    this->m_run_lengths.observe(corrected_run_lengths);
                    this->m_errors.observe(corrected_errors);

                    this->m_stopping_time.reset();
                }

                friend std::ostream& operator <<(std::ostream& os, const type& self)
                {
                    auto ess = self.m_run_lengths.mean();
                    auto perror = self.m_errors.mean();
                    os <<
                        self.m_name << " {" <<
                        " ESS: " << ess.front() << "---" << ess.back() <<
                        " pm " << std::sqrt(self.m_run_lengths.variance().back()) << ","
                        " P[error]: (" << type::error_factor * perror.front() << "---" << type::error_factor * perror.back() <<
                        " pm " << type::error_factor * std::sqrt(self.m_errors.variance().back()) <<
                        ") e-" << type::error_suffix <<
                        " }";
                    return os;
                }

            private:
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_STOPPING_TIME_OBSERVER_HPP_INCLUDED
