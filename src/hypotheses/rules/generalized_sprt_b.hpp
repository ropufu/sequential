
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_B_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_B_HPP_INCLUDED

#include <ropufu/on_error.hpp>

#include "../model.hpp"
#include "../process.hpp"
#include "../two_sprt.hpp"
#include "generalized_sprt_b_design.hpp"

#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc
#include <vector>    // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_signal_type, typename t_noise_type>
    struct generalized_sprt_b;

    template <typename t_process_type>
    using generalized_sprt_b_t = generalized_sprt_b<typename t_process_type::signal_type, typename t_process_type::noise_type>;

    template <typename t_signal_type, typename t_noise_type>
    struct generalized_sprt_b : public two_sprt<
        generalized_sprt_b<t_signal_type, t_noise_type>,
        t_signal_type, t_noise_type>
    {
        using type = generalized_sprt_b<t_signal_type, t_noise_type>;
        using base_type = two_sprt<type, t_signal_type, t_noise_type>;
        friend base_type;

        using signal_type = typename base_type::signal_type;
        using noise_type = typename base_type::noise_type;
        using process_type = typename base_type::process_type;
        using value_type = typename base_type::value_type;
        using model_type = typename base_type::model_type;
        using likelihood_type = typename base_type::likelihood_type;
        using moment_statistic_type = typename base_type::moment_statistic_type;
        using design_type = generalized_sprt_b_design<value_type>;

    private:
        // ~~ Fundamental members ~~
        design_type m_design = {};
        value_type m_mu_cutoff = 0; // Threshold used to decide in favor of either of the hypotheses.
        
        // ~~ Members reset with each \c toc() ~~
        value_type m_unscaled_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator.
        value_type m_unscaled_distance_from_alt = 0;  // Latest (unscaled) LLR vs. alt estimator.
        bool m_is_estimator_low = false; // Indicator if the latest estimator of mu is below the threshold.
        bool m_is_estimator_high = false; // Indicator if the latest estimator of mu is above the threshold.

    protected:
        /** @brief Indicates if the choice of thresholds does not affect other design parameters. */
        bool is_design_threshold_independent() const noexcept { return this->m_design.is_threshold_independent(); }

        /** @brief Auxiliary function to be executed right after the \c initialize() call. */
        void on_initialized() noexcept
        {
            this->m_mu_cutoff = this->likelihood().model().mu_relative(this->m_design.relative_mu_cutoff());
            this->on_reset_override();
        } // on_initialized(...)

        /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
        void on_reset_override() noexcept
        {
            this->m_unscaled_distance_from_null = 0;
            this->m_unscaled_distance_from_alt = 0;
            this->m_is_estimator_low = false;
            this->m_is_estimator_high = false;
        } // on_reset_override(...)

        /** @brief Auxiliary function to be executed right after the \c on_tic() call. */
        void on_tic_override(const process_type& proc) noexcept
        {
            value_type null_mu = this->likelihood().model().mu_under_null();
            value_type alt_mu = this->likelihood().model().smallest_mu_under_alt();
            value_type mu_null_hat = this->likelihood().null_estimator_of_mu().back();
            //value_type mu_alt_hat = (mu_null_hat < alt_mu) ? alt_mu : mu_null_hat;
            
            this->m_unscaled_distance_from_null = proc.unscaled_log_likelihood_between(mu_null_hat, null_mu);
            this->m_unscaled_distance_from_alt = proc.unscaled_log_likelihood_between(mu_null_hat, alt_mu);
            this->m_is_estimator_low = (mu_null_hat <= this->m_mu_cutoff);
            this->m_is_estimator_high = (mu_null_hat >= this->m_mu_cutoff);
        } // on_tic_override(...)

        /** @brief Auxiliary function to be executed right before the \c on_toc() call. */
        void on_toc_override(const process_type& /*proc*/) noexcept
        {
            this->on_reset_override();
        } // on_toc_override(...)

    public:
        generalized_sprt_b() noexcept : base_type() { }

        /*implicit*/ generalized_sprt_b(const design_type& design) noexcept
            : base_type(design.id()), m_design(design)
        {
        } // generalized_sprt_b(...)

        const design_type& design() const noexcept { return this->m_design; }

        std::string to_path_string(std::size_t decimal_places = 3) const noexcept { return this->m_design.to_path_string(decimal_places); }

        bool do_decide_null(value_type threshold, std::size_t /*row_index*/, std::size_t /*column_index*/) const noexcept { return this->m_is_estimator_low && this->m_unscaled_distance_from_alt > threshold; }
        bool do_decide_alt(value_type threshold, std::size_t /*row_index*/, std::size_t /*column_index*/) const noexcept { return this->m_is_estimator_high && this->m_unscaled_distance_from_null > threshold; }

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            return os << self.m_design;
        } // operator <<(...)
    }; // struct generalized_sprt_b
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_B_HPP_INCLUDED
