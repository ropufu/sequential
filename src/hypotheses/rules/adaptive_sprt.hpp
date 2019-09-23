
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/number_traits.hpp>

#include "adaptive_sprt_design.hpp"
#include "adaptive_sprt_flavor.hpp"
#include "two_sprt.hpp"
#include "../likelihood.hpp"
#include "../model.hpp"
#include "../simple_process.hpp"
#include "../change_of_measure.hpp"

#include <cmath>     // std::sqrt
#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <stdexcept> // std::logic_error
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc
#include <vector>    // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_engine_type, typename t_value_type, adaptive_sprt_flavor t_flavor = adaptive_sprt_flavor::simple>
    struct adaptive_sprt;

    template <typename t_engine_type, typename t_value_type, adaptive_sprt_flavor t_flavor>
    struct adaptive_sprt : public two_sprt<
        adaptive_sprt<t_engine_type, t_value_type, t_flavor>,
        t_engine_type, t_value_type>
    {
        using type = adaptive_sprt<t_engine_type, t_value_type, t_flavor>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using base_type = two_sprt<type, engine_type, value_type>;
        friend base_type;

        using model_type = model<value_type>;
        using likelihood_type = likelihood<value_type>;
        using process_type = simple_process<engine_type, value_type>;
        using change_of_measure_type = change_of_measure<value_type>;
        using design_type = adaptive_sprt_design<value_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        
    private:
        // ~~ Fundamental members ~~
        design_type m_design = design_type(t_flavor);
        matrix_t<value_type> m_mu_guess_for_null = {};
        matrix_t<value_type> m_mu_guess_for_alt = {};

        // ~~ Members reset with each \c toc() ~~
        value_type m_delayed_mu_null_estimator = 0; // Delayed (by one observation) null estimator for mu.
        value_type m_unscaled_offset_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator.
        value_type m_unscaled_offset_distance_from_alt = 0;  // Latest (unscaled) LLR vs. alt estimator.
        matrix_t<value_type> m_init_distance_from_null = {}; // Initial (unscaled) LLR vs. null estimator.
        matrix_t<value_type> m_init_distance_from_alt = {};  // Initial (unscaled) LLR vs. null estimator.

    protected:
        /** @brief Indicates if the choice of thresholds does not affect other design parameters. */
        bool is_design_threshold_independent() const noexcept { return this->m_design.is_threshold_independent(); }

        /** @brief Auxiliary function to be executed right after the \c initialize() call. */
        void on_initialized(const model_type& model,
            const std::vector<value_type>& unscaled_null_thresholds,
            const std::vector<value_type>& unscaled_alt_thresholds) noexcept
        {
            std::size_t m = unscaled_null_thresholds.size(); // Height of the threshold matrix.
            std::size_t n = unscaled_alt_thresholds.size(); // Width of the threshold matrix.
            this->m_mu_guess_for_null = matrix_t<value_type>(m, n);
            this->m_mu_guess_for_alt = matrix_t<value_type>(m, n);
            this->m_init_distance_from_null = matrix_t<value_type>(m, n);
            this->m_init_distance_from_alt = matrix_t<value_type>(m, n);
            // this->m_init_distance_from_null.fill(0);
            // this->m_init_distance_from_alt.fill(0);

            if (this->m_design.is_threshold_independent())
            {
                value_type mu_guess_for_null = model.mu_relative(this->m_design.relative_mu_null_init());
                value_type mu_guess_for_alt =  model.mu_relative(this->m_design.relative_mu_alt_init());
                this->m_mu_guess_for_null.fill(mu_guess_for_null);
                this->m_mu_guess_for_alt.fill(mu_guess_for_alt);
            } // if (...)
            else
            {
                value_type null_mu = model.mu_under_null();
                value_type alt_mu = model.smallest_mu_under_alt();
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
        } // on_initialized(...)

        /** @brief Auxiliary function to be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            this->m_delayed_mu_null_estimator = 0;
            this->m_unscaled_offset_distance_from_null = 0;
            this->m_unscaled_offset_distance_from_alt = 0;
            this->m_init_distance_from_null.fill(0);
            this->m_init_distance_from_alt.fill(0);
        } // on_reset(...)

        /** @brief Auxiliary function to be executed right after the \c tic() call. */
        void on_tic(const process_type& proc, const likelihood_type& likelihood)
        {
            std::size_t time_index = proc.time();

            value_type null_mu = likelihood.model().mu_under_null();
            value_type alt_mu = likelihood.model().smallest_mu_under_alt();
            value_type mu_hat = likelihood.estimator_of_mu().back();
            [[maybe_unused]] value_type mu_null_hat = (mu_hat < null_mu) ? null_mu : mu_hat;
            [[maybe_unused]] value_type mu_alt_hat = (mu_hat < alt_mu) ? alt_mu : mu_hat;
            
            if (time_index == 0)
            {
                for (std::size_t i = 0; i < this->m_mu_guess_for_null.height(); ++i)
                {
                    for (std::size_t j = 0; j < this->m_mu_guess_for_alt.width(); ++j)
                    {
                        this->m_init_distance_from_null(i, j) = proc.unscaled_log_likelihood_between(this->m_mu_guess_for_null(i, j), null_mu);
                        if constexpr (t_flavor == adaptive_sprt_flavor::simple || t_flavor == adaptive_sprt_flavor::unconstrained)
                            this->m_init_distance_from_alt(i, j) =
                                proc.unscaled_log_likelihood_between(this->m_mu_guess_for_alt(i, j), alt_mu);
                        if constexpr (t_flavor == adaptive_sprt_flavor::general)
                            this->m_init_distance_from_alt(i, j) =
                                proc.unscaled_log_likelihood_between(this->m_mu_guess_for_alt(i, j), null_mu) -
                                proc.unscaled_log_likelihood_between(mu_alt_hat, null_mu);
                    } // for (...)
                } // for (...)
            } // if (...)
            else
            {
                this->m_unscaled_offset_distance_from_null += proc.unscaled_log_likelihood_at(time_index, this->m_delayed_mu_null_estimator, null_mu);
                if constexpr (t_flavor == adaptive_sprt_flavor::simple || t_flavor == adaptive_sprt_flavor::unconstrained)
                    this->m_unscaled_offset_distance_from_alt += proc.unscaled_log_likelihood_at(time_index, this->m_delayed_mu_null_estimator, alt_mu);
                if constexpr (t_flavor == adaptive_sprt_flavor::general)
                    this->m_unscaled_offset_distance_from_alt =
                        this->m_unscaled_offset_distance_from_null - 
                        proc.unscaled_log_likelihood_between(mu_alt_hat, null_mu);
            } // else (...)

            // Update the delayed signal "strength" estimators.
            if constexpr (t_flavor == adaptive_sprt_flavor::unconstrained) this->m_delayed_mu_null_estimator = mu_hat;
            else this->m_delayed_mu_null_estimator = mu_null_hat;
        } // on_tic_override(...)

        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& /*proc*/, const likelihood_type& /*likelihood*/) { }

        bool do_decide_null(value_type threshold, std::size_t row_index, std::size_t column_index) const noexcept
        { 
            return (this->m_init_distance_from_alt(row_index, column_index) + this->m_unscaled_offset_distance_from_alt) > threshold;
        } // do_decide_null(...)

        bool do_decide_alt(value_type threshold, std::size_t row_index, std::size_t column_index) const noexcept
        {
            return (this->m_init_distance_from_null(row_index, column_index) + this->m_unscaled_offset_distance_from_null) > threshold;
        } // do_decide_alt(...)

    public:
        adaptive_sprt() noexcept : base_type() { }

        /*implicit*/ adaptive_sprt(const design_type& design)
            : base_type(design.id()), m_design(design)
        {
            if (design.flavor() != t_flavor) throw std::logic_error("Adaptive SPRT flavor mismatch.");
        } // adaptive_sprt(...)

        const design_type& design() const noexcept { return this->m_design; }

        std::string to_path_string(std::size_t decimal_places = 3) const noexcept { return this->m_design.to_path_string(decimal_places); }
        
        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            return os << self.m_design;
        } // operator <<(...)
    }; // struct adaptive_sprt
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_HPP_INCLUDED
