
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_PROCESS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_PROCESS_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include "timed.hpp"
#include "signal_base.hpp"
#include "noise_base.hpp"

#include "type_traits.hpp"

#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <random>   // std::normal_distribution, std::default_random_engine
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc
#include <type_traits> // std::is_same_v, std::is_base_of_v
#include <vector>   // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_signal_type, typename t_noise_type>
    struct process;
    template <typename t_signal_type, typename t_noise_type>
    void to_json(nlohmann::json& j, const process<t_signal_type, t_noise_type>& x) noexcept;
    template <typename t_signal_type, typename t_noise_type>
    void from_json(const nlohmann::json& j, process<t_signal_type, t_noise_type>& x);

    template <typename t_signal_type, typename t_noise_type>
    struct process : public timed<process<t_signal_type, t_noise_type>>
    {
        using type = process<t_signal_type, t_noise_type>;
        using signal_type = t_signal_type;
        using noise_type = t_noise_type;
        using value_type = typename t_noise_type::value_type;

        using base_type = timed<type>;
        friend base_type;

        static constexpr std::size_t default_history_capacity = 100;

        // ~~ Json names ~~
        static constexpr char jstr_signal[] = "signal";
        static constexpr char jstr_noise[] = "noise";
        static constexpr char jstr_actual_mu[] = "actual mu";

    private:
        // ~~ Signal ~~
        signal_type m_signal = {}; // Signal.
        noise_type m_noise = {};   // Parameters in the AR noise.
        value_type m_actual_mu = 0; // Actual signal "strength".

        // ~~ Observation generated members ~~
        std::vector<value_type> m_history = {}; // Full history of process.

        // ~~ Estimators of signal "strength" ~~
        value_type m_running_sum_ry = 0; // The running sum (signal) x (observation).
        value_type m_running_sum_rr = 0; // The running sum (signal) x (signal).
        std::vector<value_type> m_running_sum_ry_history = {};
        std::vector<value_type> m_running_sum_rr_history = {};

        static constexpr void traits_check() noexcept
        {
            // Concepts check.
            static_assert(is_signal_v<signal_type>, "t_signal_type has to be a signal.");
            static_assert(is_noise_v<noise_type>, "t_noise_type has to be a noise.");
            // Other checks.
            static_assert(std::is_same_v<
                typename signal_type::value_type,
                typename noise_type::value_type>, "t_signal_type and t_noise_type must have the same value_type typedef.");
            
        } // traits_check(...)
        
    protected:
        void coerce() noexcept
        {
            this->m_history.reserve(type::default_history_capacity);
            this->m_running_sum_ry_history.reserve(type::default_history_capacity);
            this->m_running_sum_rr_history.reserve(type::default_history_capacity);
        } // coerce(...)

        /** @brief To be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            this->m_history.clear();
            this->m_running_sum_ry_history.clear();
            this->m_running_sum_rr_history.clear();
            this->m_noise.reset();

            this->m_running_sum_ry = 0;
            this->m_running_sum_rr = 0;
        } // on_reset(...)

        /** @brief To be executed right after the \c tic() call. */
        void on_tic() noexcept
        {
            std::size_t time_index = this->time();
            this->m_noise.tic();
            
            // ~~ Observations ~~
            value_type r = this->m_signal.at(time_index); // Signal.
            value_type y = this->m_actual_mu * r + this->m_noise.current_value(); // Observation.
            this->m_history.push_back(y); // Record the adjusted observation.

            // ~~ Statistics: null estimator of signal "strength", mu. ~~
            this->m_running_sum_ry += r * y; // Add current value of (signal) x (observation).
            this->m_running_sum_rr += r * r; // Add current value of (signal) x (signal).
            this->m_running_sum_ry_history.push_back(this->m_running_sum_ry);
            this->m_running_sum_rr_history.push_back(this->m_running_sum_rr);
        } // on_tic(...)

    public:
        process() noexcept { this->coerce(); }

        /** Initializes a new time window of a given width. */
        explicit process(const signal_type& signal, const noise_type& noise, value_type actual_mu) noexcept
            : m_signal(signal), m_noise(noise), m_actual_mu(actual_mu)
        {
            this->coerce();
        } // process(...)
        
        process(const nlohmann::json& j, std::error_code& ec)
        {
            // Parse json entries.
            aftermath::noexcept_json::required(j, type::jstr_signal, this->m_signal, ec);
            aftermath::noexcept_json::required(j, type::jstr_noise, this->m_noise, ec);
            aftermath::noexcept_json::optional(j, type::jstr_actual_mu, this->m_actual_mu, ec);

            this->coerce();
        } // process(...)

        /** Signal. */
        const signal_type& signal() const noexcept { return this->m_signal; }

        /** Noise. */
        const noise_type& noise() const noexcept { return this->m_noise; }

        /** The actual signal "strength". */
        value_type actual_mu() const noexcept { return this->m_actual_mu; }

        /** The running estimate of signal "strength". */
        value_type estimate_mu() const noexcept { return this->m_running_sum_ry / this->m_running_sum_rr; }

        /** Adjusted process. */
        const std::vector<value_type>& history() const noexcept { return this->m_history; }
        /** Adjusted process. */
        value_type history(std::size_t time_index) const noexcept { return this->m_history[time_index]; }

        /** Scaling factor for LLR. */
        value_type log_likelihood_scale() const noexcept { return this->m_noise.noise_variance(); }

        /** Computes unscaled instantaneous log-likelihood ratio at \p time_index between two hypothetical values of signal "strength". */
        value_type unscaled_log_likelihood_at(std::size_t time_index, value_type theta, value_type eta) const noexcept
        {
            if (theta == eta) return 0;
            value_type shift = theta - eta;
            value_type mean = (theta + eta) / 2;
            
            value_type r = this->m_signal.at(time_index);
            value_type y = this->m_history[time_index];
            return shift * r * (y - mean * r);
        } // unscaled_log_likelihood_at(...)

        /** Computes unscaled log-likelihood ratio between two hypothetical values of signal "strength". */
        value_type unscaled_log_likelihood_between(value_type theta, value_type eta) const noexcept
        {
            return this->unscaled_log_likelihood_between(theta, eta, this->time());
        } // unscaled_log_likelihood_between(...)

        /** Computes unscaled log-likelihood ratio between two hypothetical values of signal "strength". */
        value_type unscaled_log_likelihood_between(value_type theta, value_type eta, std::size_t time_index) const noexcept
        {
            if (theta == eta) return 0;
            value_type value = 0;
            value_type shift = theta - eta;
            value_type mean = (theta + eta) / 2;
            // for (std::size_t i = 0; i <= time_index; ++i)
            // {
            //     value_type r = this->m_signal.at(i);
            //     value_type y = this->m_history[i];
            //     value += shift * r * (y - mean * r);
            // } // for (...)
            value = shift * (this->m_running_sum_ry_history[time_index] - mean * this->m_running_sum_rr_history[time_index]);
            return value;
        } // unscaled_log_likelihood_between(...)

        /** @brief Computes adaptive log-likelihood ratio between two hypothetical values of signal "strength".
         *  @remark \tparam t_theta_estimator_func has to implement operator (std::size_t) -> value_type.
         *  @remark \tparam t_eta_estimator_func has to implement operator (std::size_t) -> value_type.
         */
        template <typename t_theta_estimator_func, typename t_eta_estimator_func>
        value_type unscaled_adaptive_log_likelihood_between(const t_theta_estimator_func& theta, const t_eta_estimator_func& eta) const noexcept
        {
            return this->unscaled_adaptive_log_likelihood_between(theta, eta, this->time());
        } // unscaled_adaptive_log_likelihood_between(...)

        /** @brief Computes adaptive log-likelihood ratio between two hypothetical values of signal "strength".
         *  @remark \tparam t_theta_estimator_func has to implement operator (std::size_t) -> value_type.
         *  @remark \tparam t_eta_estimator_func has to implement operator (std::size_t) -> value_type.
         */
        template <typename t_theta_estimator_func, typename t_eta_estimator_func>
        value_type unscaled_adaptive_log_likelihood_between(const t_theta_estimator_func& theta, const t_eta_estimator_func& eta, std::size_t time_index) const noexcept
        {
            value_type value = 0;
            for (std::size_t i = 0; i <= time_index; ++i) value += this->unscaled_log_likelihood_at(i, theta(i), eta(i));
            return value;
        } // unscaled_adaptive_log_likelihood_between(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct process

    // ~~ Json name definitions ~~
    template <typename t_signal_type, typename t_noise_type> constexpr char process<t_signal_type, t_noise_type>::jstr_signal[];
    template <typename t_signal_type, typename t_noise_type> constexpr char process<t_signal_type, t_noise_type>::jstr_noise[];
    template <typename t_signal_type, typename t_noise_type> constexpr char process<t_signal_type, t_noise_type>::jstr_actual_mu[];
    
    template <typename t_signal_type, typename t_noise_type>
    void to_json(nlohmann::json& j, const process<t_signal_type, t_noise_type>& x) noexcept
    {
        using type = process<t_signal_type, t_noise_type>;

        j = nlohmann::json{
            {type::jstr_signal, x.signal()},
            {type::jstr_noise, x.noise()},
            {type::jstr_actual_mu, x.actual_mu()}
        };
    } // to_json(...)

    template <typename t_signal_type, typename t_noise_type>
    void from_json(const nlohmann::json& j, process<t_signal_type, t_noise_type>& x)
    {
        using type = process<t_signal_type, t_noise_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_PROCESS_HPP_INCLUDED
