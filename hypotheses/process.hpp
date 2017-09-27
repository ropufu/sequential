
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_PROCESS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_PROCESS_HPP_INCLUDED

#include <aftermath/probability.hpp>
#include <aftermath/random.hpp>

#include "model.hpp"
#include "time_window.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                template <typename t_distribution_type>
                struct sampler_switch
                {
                    typedef t_distribution_type type; // Built-in c++ distribution types are also samplers.
                };
                
                template <>
                struct sampler_switch<aftermath::probability::dist_normal>
                {
                    typedef aftermath::random::default_sampler_normal_t<std::default_random_engine>::type type;
                };
            }

            template <typename t_signal_type>
            struct process
            {
                typedef process<t_signal_type> type;
                typedef t_signal_type signal_type;
                typedef model<signal_type> model_type;
                typedef typename model_type::noise_distribution_type noise_distribution_type;
                typedef typename detail::sampler_switch<noise_distribution_type>::type noise_sampler_type;
                typedef std::default_random_engine engine_type;

            private:
                // ~~ Auxiliary members ~~
                engine_type m_engine; // Uniform PRNG.
                noise_sampler_type m_white_noise_sampler; // White noise sampler.

                // ~~ Observation generation members ~~
                std::size_t m_count = 0; // Number of observations.
                model_type m_model; // Hypotheses testing model descriptor.
                double m_actual_mu; // Actual signal "strength".
                time_window<double> m_ar_noise; // Brief history of AR noise used to generate observations.
                time_window<double> m_observed_process; // Brief history of observations to adjust for AR.
                std::vector<double> m_adjusted_process; // Full history of adjusted process.

                // ~~ Estimators of signal "strength" ~~
                double m_running_sum_ry = 0; // The running sum (adjusted signal) times (adjusted observation).
                double m_running_sum_rr = 0; // The running sum (adjusted signal) times (adjusted signal).
                std::vector<double> m_null_estimator_of_mu; // Estimator of signal "strength", constrained from below by the signal "strength" in the null hypothesis.

            public:
                /** Initializes a new time window of a given width. */
                explicit process(const model_type& model, double actual_mu) noexcept
                    : m_engine(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
                    m_white_noise_sampler(noise_distribution_type(0.0, model.noise_sigma())),
                    m_model(model),
                    m_actual_mu(actual_mu),
                    m_ar_noise(1 + model.ar_parameters().size()),
                    m_observed_process(1 + model.ar_parameters().size()),
                    m_adjusted_process(),
                    m_null_estimator_of_mu()
                {
                }

                void reset() noexcept
                {
                    this->m_count = 0;
                    this->m_ar_noise.reset();
                    this->m_observed_process.reset();
                    this->m_adjusted_process.clear();

                    this->m_running_sum_ry = 0;
                    this->m_running_sum_rr = 0;
                    this->m_null_estimator_of_mu.clear();
                }

                /** Hypotheses testing model descriptor. */
                const model_type& model() const noexcept { return this->m_model; }

                /** Indicates if any observations have been made. */
                bool empty() const noexcept { return this->m_count == 0; }

                /** The number of observations. */
                std::size_t count() const noexcept { return this->m_count; }

                /** The actual signal "strength". */
                double actual_mu() const noexcept { return this->m_actual_mu; }

                /** Adjusted process. */
                const std::vector<double>& adjusted_process() const noexcept { return this->m_adjusted_process; }
                /** Adjusted process. */
                double adjusted_process(std::size_t time_index) const noexcept { return this->m_adjusted_process[time_index]; }

                /** Estimator of signal "strength", constrained from below by the signal "strength" in the null hypothesis. */
                const std::vector<double>& null_estimator_of_mu() const noexcept { return this->m_null_estimator_of_mu; }
                /** Estimator of signal "strength", constrained from below by the signal "strength" in the null hypothesis. */
                double null_estimator_of_mu(std::size_t time_index) const noexcept { return this->m_null_estimator_of_mu[time_index]; }

                /** Takes another observation. */
                void tic()
                {
                    // ~~ Observations ~~
                    double w = this->m_white_noise_sampler(this->m_engine); // White noise.
                    double v = this->m_model.auto_regress(w, this->m_ar_noise, this->m_count); // AR noise.
                    this->m_ar_noise.observe(v); // Keep track of recent AR noise.

                    double x = this->m_actual_mu * this->m_model.signal(this->m_count) + v; // Observation.
                    this->m_observed_process.observe(x); // Keep track of recent observations.

                    double y = this->m_model.adjust(this->m_observed_process, this->m_count); // Adjust observations for AR.
                    this->m_adjusted_process.push_back(y); // Record the adjusted observation.

                    // ~~ Statistics: null estimator of signal "strength", mu. ~~
                    double r = this->m_model.adjusted_signal(this->m_count); // Current value of adjusted signal.
                    this->m_running_sum_ry += r * y; // Add current value of (adjusted signal) times (adjusted observation).
                    this->m_running_sum_rr += r * r; // Add current value of (adjusted signal) times (adjusted signal).
                    double mu_null_hat = this->m_running_sum_ry / this->m_running_sum_rr;
                    if (mu_null_hat < this->m_model.mu_under_null()) mu_null_hat = this->m_model.mu_under_null();
                    this->m_null_estimator_of_mu.push_back(mu_null_hat);

                    this->m_count++; // Increment time index.
                }

                double log_likelihood_scale() const noexcept { return this->m_model.log_likelihood_scale(); }

                /** Computes unscaled instantaneous log-likelihood ratio at \p time_index between two hypothetical values of signal "strength". */
                double unscaled_log_likelihood_at(std::size_t time_index, double theta, double eta) const noexcept
                {
                    if (theta == eta) return 0;
                    double shift = theta - eta;
                    double mean = (theta + eta) / 2;
                    
                    double r = this->m_model.adjusted_signal(time_index);
                    double y = this->m_adjusted_process[time_index];
                    return shift * r * (y - mean * r);
                }

                /** Computes unscaled log-likelihood ratio between two hypothetical values of signal "strength". */
                double unscaled_log_likelihood_between(double theta, double eta) const noexcept
                {
                    return this->unscaled_log_likelihood_between(theta, eta, this->m_count);
                }

                /** Computes unscaled log-likelihood ratio between two hypothetical values of signal "strength". */
                double unscaled_log_likelihood_between(double theta, double eta, std::size_t count) const noexcept
                {
                    if (theta == eta) return 0;
                    double value = 0;
                    double shift = theta - eta;
                    double mean = (theta + eta) / 2;
                    for (std::size_t i = 0; i < count; i++)
                    {
                        double r = this->m_model.adjusted_signal(i);
                        double y = this->m_adjusted_process[i];
                        value += shift * r * (y - mean * r);
                    }
                    return value;
                }

                /** @brief Computes adaptive log-likelihood ratio between two hypothetical values of signal "strength".
                 *  @remark \tparam t_theta_estimator_func has to implement operator (std::size_t) -> double..
                 *  @remark \tparam t_eta_estimator_func has to implement operator (std::size_t) -> double.
                 */
                template <typename t_theta_estimator_func, typename t_eta_estimator_func>
                double unscaled_adaptive_log_likelihood_between(const t_theta_estimator_func& theta, const t_eta_estimator_func& eta) const noexcept
                {
                    return this->unscaled_adaptive_log_likelihood_between(theta, eta, this->m_count);
                }

                /** @brief Computes adaptive log-likelihood ratio between two hypothetical values of signal "strength".
                 *  @remark \tparam t_theta_estimator_func has to implement operator (std::size_t) -> double..
                 *  @remark \tparam t_eta_estimator_func has to implement operator (std::size_t) -> double.
                 */
                template <typename t_theta_estimator_func, typename t_eta_estimator_func>
                double unscaled_adaptive_log_likelihood_between(const t_theta_estimator_func& theta, const t_eta_estimator_func& eta, std::size_t count) const noexcept
                {
                    double value = 0;
                    for (std::size_t i = 0; i < count; i++) value += unscaled_log_likelihood_at(i, theta(i), eta(i));
                    return value;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_PROCESS_HPP_INCLUDED
