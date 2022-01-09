
#ifndef ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_XSPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_XSPRT_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/probability/normal_distribution.hpp>
#include <ropufu/random/normal_sampler_512.hpp>
#include <ropufu/sequential/iid_process.hpp>
#include <ropufu/sequential/parallel_stopping_time.hpp>
#include <ropufu/sequential/statistic.hpp>
#include <ropufu/simple_vector.hpp>

#include "model.hpp"

#include <cmath>       // std::exp
#include <concepts>    // std::floating_point, std::same_as
#include <ranges>      // std::ranges::...
#include <stdexcept>   // std::logic_error
#include <utility>     // std::make_pair

namespace ropufu::sequential::gaussian_mean_hypotheses
{
    template <typename t_type>
    struct xsprt_pair
    {
        t_type adaptive_sprt;
        t_type generalized_sprt;
    }; // struct xsprt_pair

    template <std::floating_point t_value_type>
    struct xsprt_output
    {
        template <typename t_data_type>
        using matrix_pair_t = xsprt_pair<ropufu::aftermath::algebra::matrix<t_data_type>>;

        t_value_type anticipated_sample_size;

        /** Number of observations prior to stopping. */
        matrix_pair_t<std::size_t> when_stopped;

        /** Indicator of erroneous decision associated with current simulation. */
        matrix_pair_t<t_value_type> direct_error_indicator;

        /** Estimator of erroneous decision associated with the change of measure. */
        matrix_pair_t<t_value_type> importance_error_indicator;

        std::size_t height() const noexcept { return this->when_stopped.adaptive_sprt.height(); }
        std::size_t width() const noexcept { return this->when_stopped.adaptive_sprt.width(); }
    }; // struct xsprt_output

    /** Shared statistics between ASPRT and GSPRT. */
    template <std::floating_point t_value_type>
    struct xsprt_state
    {
        using value_type = t_value_type;

        value_type running_sum_of_signal_times_observation = 0;
        value_type running_sum_of_signal_squared = 0;
        value_type running_sum_for_adaptive_log_likelihood = 0;
        value_type adaptive_log_likelihood_init_null = 0;
        value_type adaptive_log_likelihood_init_alternative = 0;
        value_type delayed_signal_strength_estimator = 0;

        value_type log_likelihood_ratio_between(value_type a, value_type b) const noexcept
        {
            value_type delta = a - b;
            value_type mean = (a + b) / 2;
            return delta * (
                this->running_sum_of_signal_times_observation -
                mean * this->running_sum_of_signal_squared);
        } // log_likelihood_ratio_between(...)
    }; // struct xsprt_state

    /** Calculates two stopping times: adaptive SPRT, and generalized SPRT. */
    template <std::floating_point t_value_type>
    struct xsprt
        : public ropufu::aftermath::sequential::statistic<t_value_type, void>
    {
        using type = xsprt<t_value_type>;
        using value_type = t_value_type;

        template <typename t_data_type>
        using matrix_t = ropufu::aftermath::algebra::matrix<t_data_type>;
        template <typename t_data_type>
        using matrix_pair_t = xsprt_pair<ropufu::aftermath::algebra::matrix<t_data_type>>;
        
        using state_type = xsprt_state<value_type>;
        using output_type = xsprt_output<value_type>;

        using model_type = ropufu::sequential::gaussian_mean_hypotheses::model<value_type>;
        using stopping_time_type = ropufu::aftermath::sequential::parallel_stopping_time<value_type, value_type>;
        using thresholds_type = typename stopping_time_type::thresholds_type;

    private:
        model_type m_model = {};
        std::size_t m_count_observations = 0;
        state_type m_state = {};
        stopping_time_type m_adaptive_sprt = {};
        stopping_time_type m_generalized_sprt = {};
        value_type m_simulated_signal_strength = 0;
        value_type m_change_of_measure_signal_strength = 0;
        value_type m_anticipated_sample_size = 0;

        char truth(value_type signal_strength) const noexcept
        {
            if (signal_strength == 0) return stopping_time_type::decide_vertical;
            if (signal_strength >= this->m_model.weakest_signal_strength()) return stopping_time_type::decide_horizontal;
            return 0;
        } // truth(...)

        matrix_t<value_type> direct_error_indicator(const stopping_time_type& t) const noexcept
        {
            const matrix_t<char>& which = t.which();
            const std::size_t m = which.height();
            const std::size_t n = which.width();

            char correct_decision = this->truth(this->m_simulated_signal_strength);

            return matrix_t<value_type>::generate(m, n, [correct_decision, &which] (std::size_t i, std::size_t j) {
                return (which(i, j) == correct_decision) ? 0 : 1;
            });
        } // direct_error_indicator(...)

        matrix_t<value_type> importance_error_indicator(const stopping_time_type& t) const noexcept
        {
            const matrix_t<char>& which = t.which();
            const std::size_t m = which.height();
            const std::size_t n = which.width();

            matrix_t<value_type> change_of_measure = t.stopped_statistic();
            char correct_decision = this->truth(this->m_change_of_measure_signal_strength);

            return matrix_t<value_type>::generate(m, n, [correct_decision, &which, &change_of_measure] (std::size_t i, std::size_t j) {
                return (which(i, j) == correct_decision) ? 0 : std::exp(-change_of_measure(i, j));
            });
        } // importance_error_indicator(...)

    public:
        xsprt() noexcept = default;

        explicit xsprt(model_type model,
            const thresholds_type& asprt_thresholds, const thresholds_type& gsprt_thresholds,
            value_type simulated_signal_strength, value_type change_of_measure_signal_strength,
            value_type anticipated_sample_size) noexcept
            : m_model(model),
            m_adaptive_sprt(asprt_thresholds.first, asprt_thresholds.second),
            m_generalized_sprt(gsprt_thresholds.first, gsprt_thresholds.second),
            m_simulated_signal_strength(simulated_signal_strength), m_change_of_measure_signal_strength(change_of_measure_signal_strength),
            m_anticipated_sample_size(anticipated_sample_size)
        {
        } // xsprt(...)

        const model_type& model() const noexcept { return this->m_model; }

        value_type simulated_signal_strength() const noexcept { return this->m_simulated_signal_strength; }
        
        value_type change_of_measure_signal_strength() const noexcept { return this->m_change_of_measure_signal_strength; }

        value_type anticipated_sample_size() const noexcept { return this->m_anticipated_sample_size; }

        bool is_running() const noexcept
        {
            return this->m_adaptive_sprt.is_running() || this->m_generalized_sprt.is_running();
        } // is_running(...)

        void reset() noexcept override
        {
            this->m_count_observations = 0;
            this->m_state = {};
            this->m_adaptive_sprt.reset();
            this->m_generalized_sprt.reset();
        } // reset(...)

        void observe(const value_type& value) noexcept override
        {
            ++this->m_count_observations;
            const std::size_t time = this->m_count_observations;
            const xsprt_state<value_type>& state = this->m_state;

            // ================================================================
            // Update auxiliary statistics shared by ASPRT and GSPRT (state).
            // ================================================================
            value_type s = this->m_model.signal_at(time);
            this->m_state.running_sum_of_signal_times_observation += s * value;
            this->m_state.running_sum_of_signal_squared += s * s;

            value_type uncostrained_signal_strength_estimator = this->m_state.running_sum_of_signal_times_observation / this->m_state.running_sum_of_signal_squared;
            if (uncostrained_signal_strength_estimator < 0) uncostrained_signal_strength_estimator = 0;

            value_type alternative_signal_strength_estimator =
                (uncostrained_signal_strength_estimator < this->m_model.weakest_signal_strength()) ?
                this->m_model.weakest_signal_strength() :
                uncostrained_signal_strength_estimator;

            if (time == 1) [[unlikely]]
            {
                value_type y = alternative_signal_strength_estimator * s;
                this->m_state.adaptive_log_likelihood_init_null = 0;
                this->m_state.adaptive_log_likelihood_init_alternative = y * (value - y / 2);
            } // if (...)
            else [[likely]]
            {
                value_type y = this->m_state.delayed_signal_strength_estimator * s;
                this->m_state.running_sum_for_adaptive_log_likelihood += y * (value - y / 2);
            } // else (...)
            
            // ================================================================
            // Set up importance sampling.
            // ================================================================
            value_type change_of_measure = state.log_likelihood_ratio_between(
                this->m_simulated_signal_strength,
                this->m_change_of_measure_signal_strength);
            this->m_adaptive_sprt.if_stopped(change_of_measure);
            this->m_generalized_sprt.if_stopped(change_of_measure);

            // ================================================================
            // Calculate the ASPRT statistic.
            // ================================================================
            value_type adaptive_log_likelihood_null = state.adaptive_log_likelihood_init_null +
                state.running_sum_for_adaptive_log_likelihood;
            value_type adaptive_log_likelihood_alternative = state.adaptive_log_likelihood_init_alternative +
                state.running_sum_for_adaptive_log_likelihood +
                state.log_likelihood_ratio_between(0, alternative_signal_strength_estimator);
            this->m_adaptive_sprt.observe(std::make_pair(adaptive_log_likelihood_alternative, adaptive_log_likelihood_null));
            
            // ================================================================
            // Calculate the GSPRT statistic.
            // ================================================================
            value_type generalized_log_likelihood_null = state.log_likelihood_ratio_between(uncostrained_signal_strength_estimator, 0);
            value_type generalized_log_likelihood_alternative = state.log_likelihood_ratio_between(uncostrained_signal_strength_estimator, alternative_signal_strength_estimator);
            this->m_generalized_sprt.observe(std::make_pair(generalized_log_likelihood_alternative, generalized_log_likelihood_null));

            // ================================================================
            // Update delayed statistics.
            // ================================================================
            this->m_state.delayed_signal_strength_estimator = uncostrained_signal_strength_estimator;
        } // observe(...)

        output_type output() const noexcept
        {
            return {
                this->m_anticipated_sample_size,
                matrix_pair_t<std::size_t>(this->m_adaptive_sprt.when(), this->m_generalized_sprt.when()),
                matrix_pair_t<value_type>(
                    this->direct_error_indicator(this->m_adaptive_sprt),
                    this->direct_error_indicator(this->m_generalized_sprt)),
                matrix_pair_t<value_type>(
                    this->importance_error_indicator(this->m_adaptive_sprt),
                    this->importance_error_indicator(this->m_generalized_sprt))
            };
        } // output(...)
    }; // struct xsprt
} // namespace ropufu::sequential::gaussian_mean_hypotheses

#endif // ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_XSPRT_HPP_INCLUDED
