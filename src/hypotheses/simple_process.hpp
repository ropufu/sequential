
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMPLE_PROCESS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMPLE_PROCESS_HPP_INCLUDED

#include "signals/constant_signal.hpp"
#include "noises/white_noise.hpp"

#include <cstddef>     // std::size_t
#include <stdexcept>   // std::logic_error
#include <string>      // std::string
#include <type_traits> // std::is_same_v
#include <vector>      // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_engine_type, typename t_value_type>
    struct simple_process
    {
        using type = simple_process<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using signal_type = constant_signal<value_type>;
        using noise_type = white_noise<engine_type, value_type>;

        static constexpr std::size_t default_history_capacity = 100;

    private:
        signal_type m_signal = {}; // Signal.
        noise_type m_noise = {}; // Noise.
        value_type m_signal_strength = 0; // Signal scale (multiplicative).

        // ~~ Timer ~~
        std::size_t m_count = 0; // Number of observations taken.
        std::size_t m_time = 0; // Current time index.

        // ~~ Running statistics ~~
        value_type m_running_sum_ry = 0; // The running sum (signal) x (observation).
        value_type m_running_sum_rr = 0; // The running sum (signal) x (signal).

        // ~~ Global statistics ~~
        std::vector<value_type> m_history = {}; // Full history of process.
        std::vector<value_type> m_running_sum_ry_history = {};
        std::vector<value_type> m_running_sum_rr_history = {};

        void reserve() noexcept
        {
            this->m_history.reserve(type::default_history_capacity);
            this->m_running_sum_ry_history.reserve(type::default_history_capacity);
            this->m_running_sum_rr_history.reserve(type::default_history_capacity);
        } // reserve(...)

    public:
        simple_process() noexcept
        {
            this->reserve();
        } // simple_process(...)

        /** Initializes a new time window of a given width. */
        simple_process(const signal_type& signal, const noise_type& noise, value_type signal_strength = 0) noexcept
            : m_signal(signal), m_noise(noise), m_signal_strength(signal_strength) 
        {
            this->reserve();
        } // simple_process(...)

        /** Indicates if any observations have been made. */
        bool empty() const noexcept { return this->m_count == 0; }

        /** @brief The number of tics up to this moment. */
        std::size_t count() const noexcept { return this->m_count; }

        /** @brief Current time index (zero-based). */
        std::size_t time() const noexcept { return this->m_time; }

        const signal_type& signal() const noexcept { return this->m_signal; }

        const noise_type& noise() const noexcept { return this->m_noise; }

        /** Signal scale (multiplicative). */
        value_type signal_strength() const noexcept { return this->m_signal_strength; }
        /** Signal scale (multiplicative). */
        void set_signal_strength(value_type value)
        {
            if (this->m_count != 0) throw std::logic_error("Signal strength cannot be changed once the process has started.");
            this->m_signal_strength = value;
        } // set_signal_strength(...)

        /** @brief Resets the time to zero. */
        void reset() noexcept
        {
            this->m_history.clear();
            this->m_running_sum_ry_history.clear();
            this->m_running_sum_rr_history.clear();
            this->m_noise.reset();

            this->m_running_sum_ry = 0;
            this->m_running_sum_rr = 0;
            
            this->m_count = 0;
            this->m_time = 0;
        } // reset(...)

        /** @brief Advances the time index by one unit. */
        void tic(engine_type& uniform_engine) noexcept
        {
            ++this->m_count;
            this->m_time = this->m_count - 1;
            this->m_noise.tic(uniform_engine);
            
            // ~~ Observations ~~
            value_type r = this->m_signal.at(this->m_time); // Signal.
            value_type y = this->m_signal_strength * r + this->m_noise.current_value(); // Observation.
            this->m_history.push_back(y); // Record the observation.

            // ~~ Running statistics ~~
            this->m_running_sum_ry += r * y; // Add current value of (signal) x (observation).
            this->m_running_sum_rr += r * r; // Add current value of (signal) x (signal).
            this->m_running_sum_ry_history.push_back(this->m_running_sum_ry);
            this->m_running_sum_rr_history.push_back(this->m_running_sum_rr);
        } // tic(...)

        /** The running estimate of signal strength. */
        value_type estimate_signal_strength() const noexcept { return this->m_running_sum_ry / this->m_running_sum_rr; }

        const std::vector<value_type>& history() const noexcept { return this->m_history; }
        value_type history(std::size_t time_index) const { return this->m_history.at(time_index); }

        /** Scaling factor for LLR. */
        value_type log_likelihood_scale() const noexcept { return this->m_noise.variance(); }

        /** Computes instantaneous log-likelihood ratio at \p time_index between two hypothetical values of signal strength. */
        value_type unscaled_log_likelihood_at(std::size_t time_index, value_type theta, value_type eta) const
        {
            if (theta == eta) return 0;
            value_type shift = theta - eta;
            value_type mean = (theta + eta) / 2;
            
            value_type r = this->m_signal.at(time_index);
            value_type y = this->m_history.at(time_index);
            return shift * r * (y - mean * r);
        } // unscaled_log_likelihood_at(...)

        /** Computes log-likelihood ratio between two hypothetical values of signal strength. */
        value_type unscaled_log_likelihood_between(value_type theta, value_type eta) const
        {
            return this->unscaled_log_likelihood_between(theta, eta, this->m_time);
        } // unscaled_log_likelihood_between(...)

        /** Computes log-likelihood ratio between two hypothetical values of signal strength. */
        value_type unscaled_log_likelihood_between(value_type theta, value_type eta, std::size_t time_index) const
        {
            if (theta == eta) return 0;
            value_type shift = theta - eta;
            value_type mean = (theta + eta) / 2;

            return shift * (this->m_running_sum_ry_history.at(time_index) - mean * this->m_running_sum_rr_history.at(time_index));
        } // unscaled_log_likelihood_between(...)

        /** @brief Computes adaptive log-likelihood ratio between two hypothetical values of signal strength.
         *  @remark \tparam t_theta_estimator_type has to implement operator ()(std::size_t) -> value_type.
         *  @remark \tparam t_eta_estimator_type has to implement operator ()(std::size_t) -> value_type.
         */
        template <typename t_theta_estimator_type, typename t_eta_estimator_type>
        value_type unscaled_adaptive_log_likelihood_between(const t_theta_estimator_type& theta, const t_eta_estimator_type& eta) const
        {
            return this->unscaled_adaptive_log_likelihood_between(theta, eta, this->m_time);
        } // unscaled_adaptive_log_likelihood_between(...)

        /** @brief Computes adaptive log-likelihood ratio between two hypothetical values of signal strength.
         *  @remark \tparam t_theta_estimator_type has to implement operator ()(std::size_t) -> value_type.
         *  @remark \tparam t_eta_estimator_type has to implement operator ()(std::size_t) -> value_type.
         */
        template <typename t_theta_estimator_type, typename t_eta_estimator_type>
        value_type unscaled_adaptive_log_likelihood_between(const t_theta_estimator_type& theta, const t_eta_estimator_type& eta, std::size_t time_index) const
        {
            value_type value = 0;
            for (std::size_t i = 0; i <= time_index; ++i) value += this->log_likelihood_at(i, theta(i), eta(i));
            return value;
        } // unscaled_adaptive_log_likelihood_between(...)
    }; // struct simple_process
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMPLE_PROCESS_HPP_INCLUDED
