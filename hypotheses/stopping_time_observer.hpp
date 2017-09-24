
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_STOPPING_TIME_OBSERVER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_STOPPING_TIME_OBSERVER_HPP_INCLUDED

#include <aftermath/probability/empirical_measure.hpp>
#include <aftermath/template_math.hpp>

#include "not_an_error.hpp"
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
                typedef stopping_time_observer<t_stopping_time_type> type;
                typedef t_stopping_time_type stopping_type;
                typedef typename stopping_type::signal_type signal_type;
                typedef typename stopping_type::model_type model_type;
                typedef typename stopping_type::process_type process_type;
                typedef aftermath::probability::empirical_measure<double, std::size_t, double> empirical_measure_type;

                /** Scaling factor for displaying error probabilities. */
                static constexpr std::size_t error_factor = 100'000;
                /** Suffix for displaying error probabilities. */
                static constexpr std::size_t error_suffix = aftermath::log_base_n<error_factor, 10>::value;
            
            private:
                std::string m_name = ""; // Name for display purposes.
                double m_desired_mu = 0; // The signal "strength" conrresponding to what measure we want to analyze.
                stopping_type m_stopping_time; // Stopping time to be analyzed.
                std::vector<empirical_measure_type> m_errors; // Track erroneous decisions made by the stopping time.
                std::vector<empirical_measure_type> m_run_lengths; // Track run lengths of the stopping time.
            
            public:
                /** @brief Create a stopping time observer.
                 *  @remark Forwards the arguments to the stopping time constructor.
                 */
                template <typename... t_args>
                stopping_time_observer(t_args&&... args) noexcept
                    : m_stopping_time(std::forward<t_args>(args)...),
                    m_errors(0),
                    m_run_lengths(0)
                {
                }

                /** @brief Create a stopping time observer.
                 *  @remark Also defines default conversion.
                 */
                stopping_time_observer(stopping_type&& stopping_time) noexcept
                    : m_stopping_time(stopping_time),
                    m_errors(0),
                    m_run_lengths(0)
                {
                }

                /** @brief Create a stopping time observer.
                 *  @remark Also defines default conversion.
                 */
                stopping_time_observer(const stopping_type& stopping_time) noexcept
                    : m_stopping_time(stopping_time),
                    m_errors(0),
                    m_run_lengths(0)
                {
                }

                /** Name as it will appear when sending to an output stream. */
                const std::string& name() const noexcept { return this->m_name; }
                /** Name as it will appear when sending to an output stream. */
                void set_name(const std::string& value) noexcept { this->m_name = value; }

                /** The signal "strength" conrresponding to what measure we want to analyze. */
                void look_for(double desired_mu) noexcept { this->m_desired_mu = desired_mu; }
            
                /** Stopping time to be analyzed. */
                const stopping_type& stopping_time() const noexcept { return this->m_stopping_time; }
                /** Track run lengths of the stopping time. */
                const std::vector<empirical_measure_type>& run_lengths() const noexcept { return this->m_run_lengths; }
                /** Track erroneous decisions made by the stopping time. */
                const std::vector<empirical_measure_type>& errors() const noexcept { return this->m_errors; }

                /** Clears all the observations and resets the underlying stopping time. */
                void clear() noexcept
                {
                    for (empirical_measure_type& x : this->m_run_lengths) x.clear();
                    for (empirical_measure_type& x : this->m_errors) x.clear();

                    this->m_stopping_time.reset();
                }
            
                /** @brief Set the thresholds for the underlying stopping type.
                 *  @remark Forwards the arguments to the stopping time.
                 */
                template <typename... t_args>
                quiet_return<void> set_thresholds(t_args&&... args) noexcept 
                { 
                    quiet_return<void> result = this->m_stopping_time.set_thresholds(std::forward<t_args>(args)...); 
                    if (result.error() != not_an_error::all_good) return result;
                    // Now that the stopping time has verified the threshold structure, go on and resize the empirical measures accordingly.
                    std::size_t count = this->m_stopping_time.run_lengths().size();

                    this->m_run_lengths.resize(count); // Align observation counts to match threshold size.
                    this->m_errors.resize(count); // Align cross indicators to match threshold size.
                    return not_an_error::all_good;
                }
            
                /** Indicates if the underlying stopping time is still running. */
                bool is_running() const noexcept { return this->m_stopping_time.is_running(); }
            
                /** @brief Collect another obseration from the \p proc.
                 *  @remark Forwards the argument to the stopping time.
                 */
                quiet_return<void> tic(const process_type& proc) noexcept { return this->m_stopping_time.observe(proc); }
            
                /** Builds operating characteristics of the stopped rule and resets it for the next simulation. */
                quiet_return<void> toc(const process_type& proc) noexcept
                {
                    const std::vector<std::size_t>& run_lengths = this->m_stopping_time.run_lengths();
                    const std::vector<bool>& have_crossed_null = this->m_stopping_time.have_crossed_null();
                    const std::vector<bool>& have_crossed_alt = this->m_stopping_time.have_crossed_alt();

                    // Count the thresholds.
                    std::size_t count = run_lengths.size();
                    for (std::size_t i = 0; i < count; i++)
                    {
                        bool has_crossed_null = have_crossed_null[i];
                        bool has_crossed_alt = have_crossed_alt[i];
                        // Check if the stopping time has stopped!
                        if (!has_crossed_null && !has_crossed_alt)
                        {
                            this->m_stopping_time.reset(); // If not, reset the rule anyway...
                            return not_an_error::logic_error; // ...and signal a quiet error.
                        }

                        bool is_null_true = proc.model().is_null(this->m_desired_mu);
                        bool is_alt_true = proc.model().is_alt(this->m_desired_mu);
                
                        std::size_t run_length = run_lengths[i];
                        std::size_t error = 0;
                        if (has_crossed_null && has_crossed_alt) error = 1;
                        if (has_crossed_null && is_alt_true) error = 1;
                        if (has_crossed_alt && is_null_true) error = 1;
                        //std::size_t error_relaxed = has_crossed_null ? (is_null_true ? 0 : 1) : (is_null_true ? 1 : 0);
                
                        double correction = std::exp(proc.unscaled_log_likelihood_between(proc.actual_mu(), this->m_desired_mu, run_length) / proc.log_likelihood_scale());
                        double t = run_length / correction;
                        double e = error / correction;
                        this->m_run_lengths[i].observe(t);
                        this->m_errors[i].observe(e);
                    }
                    this->m_stopping_time.reset();
                    return not_an_error::all_good;
                }

                friend std::ostream& operator <<(std::ostream& os, const type& self)
                {
                    os <<
                        self.m_name << " {" <<
                        " ESS: " << self.m_run_lengths.back().mean() <<
                        " pm " << self.m_run_lengths.back().compute_standard_deviation() << ","
                        " P[error]: (" << type::error_factor * self.m_errors.back().mean() <<
                        " pm " << type::error_factor * self.m_errors.back().compute_standard_deviation() <<
                        ") e-" << type::error_suffix <<
                        " }";
                    return os;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_STOPPING_TIME_OBSERVER_HPP_INCLUDED
