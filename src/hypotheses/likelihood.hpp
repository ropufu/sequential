
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_LIKELIHOOD_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_LIKELIHOOD_HPP_INCLUDED

#include <ropufu/probability.hpp>
#include <ropufu/on_error.hpp> // aftermath::detail::on_error

#include "model.hpp"
#include "process.hpp"
#include "observer.hpp"

#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Process observer that keep track of likelihood statistics. */
            template <typename t_signal_type, typename t_noise_type>
            struct likelihood : public observer<likelihood<t_signal_type, t_noise_type>, t_signal_type, t_noise_type>
            {
                using type = likelihood<t_signal_type, t_noise_type>;
                using base_type = observer<type, t_signal_type, t_noise_type>;
                friend base_type;

                using signal_type = typename base_type::signal_type;
                using noise_type = typename base_type::noise_type;
                using process_type = typename base_type::process_type;
                using value_type = typename base_type::value_type;
                using model_type = hypotheses::model<value_type>;

            private:
                model_type m_model = {};
                // ~~ Statistics ~~
                std::vector<value_type> m_estimator_of_mu = {}; // Unconstrained estimator of signal "strength".
                std::vector<value_type> m_null_estimator_of_mu = {}; // Estimator of signal "strength", constrained from below by the signal "strength" in the null hypothesis.

            protected:
                /** @brief Auxiliary function to be executed right before the \c reset() call. */
                void on_reset() noexcept
                {
                    this->m_estimator_of_mu.clear();
                    this->m_null_estimator_of_mu.clear();
                } // on_reset(...)

                /** @brief Auxiliary function to be executed right after the \c tic() call. */
                void on_tic(const process_type& proc, std::error_code& /*ec*/) noexcept
                {
                    value_type mu_hat = proc.estimate_mu();
                    value_type mu_hat_null = (mu_hat < this->m_model.mu_under_null()) ? this->m_model.mu_under_null() : mu_hat;

                    this->m_estimator_of_mu.push_back(mu_hat);
                    this->m_null_estimator_of_mu.push_back(mu_hat_null);
                } // on_tic(...)
                
                /** @brief Auxiliary function to be executed right before the \c toc() call. */
                void on_toc(const process_type& /*proc*/, std::error_code& /*ec*/) noexcept
                {
                    this->on_reset();
                } // on_toc(...)

            public:
                likelihood() noexcept { }

                explicit likelihood(const model_type& model) noexcept : m_model(model) { }

                const model_type& model() const noexcept { return this->m_model; }

                /** @brief Indicates whether the observer is still active. */
                bool is_listening() const noexcept { return true; }

                /** Unconstrained estimator of signal "strength". */
                const std::vector<value_type>& estimator_of_mu() const noexcept { return this->m_estimator_of_mu; }
                /** Unconstrained estimator of signal "strength". */
                value_type estimator_of_mu(std::size_t time_index) const noexcept { return this->m_estimator_of_mu[time_index]; }

                /** Estimator of signal "strength", constrained from below by the signal "strength" in the null hypothesis. */
                const std::vector<value_type>& null_estimator_of_mu() const noexcept { return this->m_null_estimator_of_mu; }
                /** Estimator of signal "strength", constrained from below by the signal "strength" in the null hypothesis. */
                value_type null_estimator_of_mu(std::size_t time_index) const noexcept { return this->m_null_estimator_of_mu[time_index]; }
            }; // struct likelihood
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_LIKELIHOOD_HPP_INCLUDED
