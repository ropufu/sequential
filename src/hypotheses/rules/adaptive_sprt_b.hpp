
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_B_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_B_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/on_error.hpp>

#include "../model.hpp"
#include "../process.hpp"
#include "../two_sprt.hpp"
#include "adaptive_sprt_b_design.hpp"

#include <cmath>     // std::sqrt
#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc
#include <vector>    // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_signal_type, typename t_noise_type>
    struct adaptive_sprt_b;

    template <typename t_process_type>
    using adaptive_sprt_b_t = adaptive_sprt_b<typename t_process_type::signal_type, typename t_process_type::noise_type>;

    template <typename t_signal_type, typename t_noise_type>
    struct adaptive_sprt_b : public two_sprt<
        adaptive_sprt_b<t_signal_type, t_noise_type>,
        t_signal_type, t_noise_type>
    {
        using type = adaptive_sprt_b<t_signal_type, t_noise_type>;
        using base_type = two_sprt<type, t_signal_type, t_noise_type>;
        friend base_type;

        using signal_type = typename base_type::signal_type;
        using noise_type = typename base_type::noise_type;
        using process_type = typename base_type::process_type;
        using value_type = typename base_type::value_type;
        using model_type = typename base_type::model_type;
        using likelihood_type = typename base_type::likelihood_type;
        using moment_statistic_type = typename base_type::moment_statistic_type;
        using design_type = adaptive_sprt_b_design<value_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;

    private:
        // ~~ Fundamental members ~~
        design_type m_design = {};
        matrix_t<value_type> m_mu_guess_for_null = {};
        matrix_t<value_type> m_mu_guess_for_alt = {};

        // ~~ Members reset with each \c toc() ~~
        value_type m_delayed_mu_null_estimator = 0; // Delayed (by one observation) null estimator for mu.
        value_type m_unscaled_offset_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator minus the initial (first observation-based) estimator.
        value_type m_unscaled_offset_distance_from_alt = 0;  // Latest (unscaled) LLR vs. alt estimator minus the initial (first observation-based) estimator.
        matrix_t<value_type> m_init_distance_from_null = {}; // Initial (unscaled) LLR vs. null estimator.
        matrix_t<value_type> m_init_distance_from_alt = {};  // Initial (unscaled) LLR vs. null estimator.

    protected:
        /** @brief Indicates if the choice of thresholds does not affect other design parameters. */
        bool is_design_threshold_independent() const noexcept { return this->m_design.is_threshold_independent(); }

        /** @brief Auxiliary function to be executed right after the \c initialize() call. */
        void on_initialized(
            const std::vector<value_type>& unscaled_null_thresholds,
            const std::vector<value_type>& unscaled_alt_thresholds) noexcept
        {
            std::size_t m = unscaled_null_thresholds.size(); // Height of the threshold matrix.
            std::size_t n = unscaled_alt_thresholds.size(); // Width of the threshold matrix.
            this->m_mu_guess_for_null = matrix_t<value_type>(m, n);
            this->m_mu_guess_for_alt = matrix_t<value_type>(m, n);
            this->m_init_distance_from_null = matrix_t<value_type>(m, n);
            this->m_init_distance_from_alt = matrix_t<value_type>(m, n);

            if (this->m_design.is_threshold_independent())
            {
                value_type mu_guess_for_null = this->likelihood().model().mu_relative(this->m_design.relative_mu_guess_for_null());
                value_type mu_guess_for_alt =  this->likelihood().model().mu_relative(this->m_design.relative_mu_guess_for_alt());
                this->m_mu_guess_for_null.fill(mu_guess_for_null);
                this->m_mu_guess_for_alt.fill(mu_guess_for_alt);
            } // if (...)
            else
            {
                value_type null_mu = this->likelihood().model().mu_under_null();
                value_type alt_mu = this->likelihood().model().smallest_mu_under_alt();
                value_type mu_diff = alt_mu - null_mu;

                for (std::size_t i = 0; i < m; ++i)
                {
                    for (std::size_t j = 0; j < n; ++j)
                    {
                        value_type mu_star = null_mu + mu_diff / (1 + std::sqrt(unscaled_null_thresholds[i] / unscaled_alt_thresholds[j]));
                        this->m_mu_guess_for_null(i, j) = mu_star;
                        this->m_mu_guess_for_alt(i, j) = mu_star;
                    } // for (...)
                } // for (...)
            } // else (...)
            
            this->on_reset_override();
        } // on_initialized(...)

        /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
        void on_reset_override() noexcept
        {
            this->m_delayed_mu_null_estimator = 0;
            this->m_unscaled_offset_distance_from_null = 0;
            this->m_unscaled_offset_distance_from_alt = 0;
            this->m_init_distance_from_null.fill(0);
            this->m_init_distance_from_alt.fill(0);
        } // on_reset_override(...)

        /** @brief Auxiliary function to be executed right after the \c on_tic() call. */
        void on_tic_override(const process_type& proc) noexcept
        {
            std::size_t time_index = proc.time();

            value_type null_mu = this->likelihood().model().mu_under_null();
            value_type alt_mu = this->likelihood().model().smallest_mu_under_alt();
            value_type mu_null_hat = this->likelihood().null_estimator_of_mu().back();
            //value_type mu_alt_hat = (mu_null_hat < alt_mu) ? alt_mu : mu_null_hat;
            
            if (time_index == 0)
            {
                for (std::size_t i = 0; i < this->m_mu_guess_for_null.height(); ++i)
                {
                    for (std::size_t j = 0; j < this->m_mu_guess_for_alt.width(); ++j)
                    {
                        this->m_init_distance_from_null(i, j) = proc.unscaled_log_likelihood_between(this->m_mu_guess_for_null(i, j), null_mu);
                        this->m_init_distance_from_alt(i, j) = proc.unscaled_log_likelihood_between(this->m_mu_guess_for_alt(i, j), alt_mu);
                    } // for (...)
                } // for (...)
            } // if (...)
            else
            {
                this->m_unscaled_offset_distance_from_null += proc.unscaled_log_likelihood_at(time_index, this->m_delayed_mu_null_estimator, null_mu);
                this->m_unscaled_offset_distance_from_alt += proc.unscaled_log_likelihood_at(time_index, this->m_delayed_mu_null_estimator, alt_mu);
            } // else (...)

            // Update the delayed signal "strength" estimators.
            this->m_delayed_mu_null_estimator = mu_null_hat;
        } // on_tic_override(...)

        /** @brief Auxiliary function to be executed right before the \c on_toc() call. */
        void on_toc_override(const process_type& /*proc*/) noexcept
        {
            this->on_reset_override();
        } // on_toc_override(...)

    public:
        adaptive_sprt_b() noexcept : base_type() { }

        /*implicit*/ adaptive_sprt_b(const design_type& design) noexcept
            : base_type(design.id()), m_design(design)
        {
        } // adaptive_sprt_b(...)

        const design_type& design() const noexcept { return this->m_design; }

        std::string to_path_string(std::size_t decimal_places = 3) const noexcept { return this->m_design.to_path_string(decimal_places); }

        bool do_decide_null(value_type threshold, std::size_t row_index, std::size_t column_index) const noexcept
        { 
            return (this->m_init_distance_from_alt(row_index, column_index) + this->m_unscaled_offset_distance_from_alt) > threshold;
        } // do_decide_null(...)

        bool do_decide_alt(value_type threshold, std::size_t row_index, std::size_t column_index) const noexcept
        {
            return (this->m_init_distance_from_null(row_index, column_index) + this->m_unscaled_offset_distance_from_null) > threshold;
        } // do_decide_alt(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            return os << self.m_design;
        } // operator <<(...)
    }; // struct adaptive_sprt_b
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_B_HPP_INCLUDED
