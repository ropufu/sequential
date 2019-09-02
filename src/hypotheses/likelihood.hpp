
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_LIKELIHOOD_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_LIKELIHOOD_HPP_INCLUDED

#include "model.hpp"
#include "simple_process.hpp"

#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector

namespace ropufu::sequential::hypotheses
{
    /** @brief Process observer that keep track of likelihood statistics. */
    template <typename t_value_type>
    struct likelihood
    {
        using type = likelihood<t_value_type>;
        using value_type = t_value_type;
        using model_type = hypotheses::model<t_value_type>;

        template <typename t_engine_type>
        using process_t = simple_process<t_engine_type, value_type>;

        static constexpr std::size_t default_history_capacity = 100;

    private:
        model_type m_model = {};
        // ~~ Statistics ~~
        std::vector<value_type> m_estimator_of_mu = {}; // Unconstrained estimator of signal strength.
        std::vector<value_type> m_null_estimator_of_mu = {}; // Estimator of signal strength, constrained from below by the signal strength under the null hypothesis.

        void reserve() noexcept
        {
            this->m_estimator_of_mu.reserve(type::default_history_capacity);
            this->m_null_estimator_of_mu.reserve(type::default_history_capacity);
        } // reserve(...)

    public:
        likelihood() noexcept
        {
            this->reserve();
        } // likelihood(...)

        explicit likelihood(const model_type& model) noexcept
            : m_model(model)
        {
            this->reserve();
        } // likelihood(...)

        const model_type& model() const noexcept { return this->m_model; }

        /** @brief Resets the time to zero. */
        void reset() noexcept
        {
            this->m_estimator_of_mu.clear();
            this->m_null_estimator_of_mu.clear();
        } // reset(...)

        template <typename t_engine_type>
        void tic(const process_t<t_engine_type>& proc) noexcept
        {
            value_type mu_hat = proc.estimate_signal_strength();
            value_type mu_hat_null = mu_hat;
            if (mu_hat_null < this->m_model.mu_under_null()) mu_hat_null = this->m_model.mu_under_null();

            this->m_estimator_of_mu.push_back(mu_hat);
            this->m_null_estimator_of_mu.push_back(mu_hat_null);
        } // tic(...)

        /** Unconstrained estimator of signal strength. */
        const std::vector<value_type>& estimator_of_mu() const noexcept { return this->m_estimator_of_mu; }
        /** Unconstrained estimator of signal strength. */
        value_type estimator_of_mu(std::size_t time_index) const { return this->m_estimator_of_mu.at(time_index); }

        /** Estimator of signal strength, constrained from below by the signal strength under the null hypothesis. */
        const std::vector<value_type>& null_estimator_of_mu() const noexcept { return this->m_null_estimator_of_mu; }
        /** Estimator of signal strength, constrained from below by the signal strength under the null hypothesis. */
        value_type null_estimator_of_mu(std::size_t time_index) const { return this->m_null_estimator_of_mu.at(time_index); }
    }; // struct likelihood
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_LIKELIHOOD_HPP_INCLUDED
