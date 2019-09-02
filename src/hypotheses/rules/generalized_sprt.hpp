
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_GENERALIZED_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_GENERALIZED_SPRT_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/number_traits.hpp>

#include "generalized_sprt_design.hpp"
#include "generalized_sprt_flavor.hpp"
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
    template <typename t_engine_type, typename t_value_type, generalized_sprt_flavor t_flavor = generalized_sprt_flavor::cutoff>
    struct generalized_sprt;

    template <typename t_engine_type, typename t_value_type, generalized_sprt_flavor t_flavor>
    struct generalized_sprt : public two_sprt<
        generalized_sprt<t_engine_type, t_value_type, t_flavor>,
        t_engine_type, t_value_type>
    {
        using type = generalized_sprt<t_engine_type, t_value_type, t_flavor>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using base_type = two_sprt<type, engine_type, value_type>;
        friend base_type;

        using model_type = model<value_type>;
        using likelihood_type = likelihood<value_type>;
        using process_type = simple_process<engine_type, value_type>;
        using change_of_measure_type = change_of_measure<value_type>;
        using design_type = generalized_sprt_design<value_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;

    private:
        // ~~ Fundamental members ~~
        design_type m_design = design_type(t_flavor);
        matrix_t<value_type> m_mu_cutoff = {}; // Threshold used to decide in favor of either of the hypotheses.
        
        // ~~ Members reset with each \c toc() ~~
        value_type m_unscaled_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator.
        value_type m_unscaled_distance_from_alt = 0;  // Latest (unscaled) LLR vs. alt estimator.
        matrix_t<bool> m_is_estimator_low = {}; // Indicator if the latest estimator of mu is below the threshold.
        matrix_t<bool> m_is_estimator_high = {}; // Indicator if the latest estimator of mu is above the threshold.

    protected:
        /** @brief Indicates if the choice of thresholds does not affect other design parameters. */
        bool is_design_threshold_independent() const noexcept { return this->m_design.is_threshold_independent(); }

        /** @brief Auxiliary function to be executed right after the \c initialize() call. */
        void on_initialized([[maybe_unused]] const model_type& model,
            [[maybe_unused]] const std::vector<value_type>& unscaled_null_thresholds,
            [[maybe_unused]] const std::vector<value_type>& unscaled_alt_thresholds) noexcept
        {
            if constexpr (t_flavor == generalized_sprt_flavor::cutoff)
            {
                std::size_t m = unscaled_null_thresholds.size(); // Height of the threshold matrix.
                std::size_t n = unscaled_alt_thresholds.size(); // Width of the threshold matrix.
                this->m_mu_cutoff = matrix_t<value_type>(m, n);
                this->m_is_estimator_low = matrix_t<bool>(m, n);
                this->m_is_estimator_high = matrix_t<bool>(m, n);

                if (this->m_design.is_threshold_independent())
                {
                    value_type mu_cutoff = model.mu_relative(this->m_design.relative_mu_cutoff());
                    this->m_mu_cutoff.fill(mu_cutoff);
                } // if (...)
                else
                {
                    value_type null_mu = model.mu_under_null();
                    value_type alt_mu = model.smallest_mu_under_alt();
                    value_type mu_diff = alt_mu - null_mu;

                    for (std::size_t i = 0; i < m; ++i)
                        for (std::size_t j = 0; j < n; ++j)
                            this->m_mu_cutoff(i, j) = null_mu + mu_diff / (1 + std::sqrt(unscaled_null_thresholds[i] / unscaled_alt_thresholds[j]));
                } // else (...)
            } // if constexpr(...)
        } // on_initialized(...)

        /** @brief Auxiliary function to be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            this->m_unscaled_distance_from_null = 0;
            this->m_unscaled_distance_from_alt = 0;
            this->m_is_estimator_low.fill(false);
            this->m_is_estimator_high.fill(false);
        } // on_reset(...)

        /** @brief Auxiliary function to be executed right after the \c tic() call. */
        void on_tic(const process_type& proc, const likelihood_type& likelihood)
        {
            value_type null_mu = likelihood.model().mu_under_null();
            value_type alt_mu = likelihood.model().smallest_mu_under_alt();
            value_type mu_null_hat = likelihood.null_estimator_of_mu().back();
            [[maybe_unused]] value_type mu_alt_hat = (mu_null_hat < alt_mu) ? alt_mu : mu_null_hat;
            
            this->m_unscaled_distance_from_null = proc.unscaled_log_likelihood_between(mu_null_hat, null_mu);
            if constexpr (t_flavor == generalized_sprt_flavor::general)
                this->m_unscaled_distance_from_alt = proc.unscaled_log_likelihood_between(mu_null_hat, mu_alt_hat);
            else
            {
                this->m_unscaled_distance_from_alt = proc.unscaled_log_likelihood_between(mu_null_hat, alt_mu);
                for (std::size_t i = 0; i < this->m_mu_cutoff.height(); ++i)
                {
                    for (std::size_t j = 0; j < this->m_mu_cutoff.width(); ++j)
                    {
                        this->m_is_estimator_low(i, j) = (mu_null_hat <= this->m_mu_cutoff(i, j));
                        this->m_is_estimator_high(i, j) = (mu_null_hat >= this->m_mu_cutoff(i, j));
                    } // for (...)
                } // for (...)
            } // if constexpr(...)
        } // on_tic(...)

        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& /*proc*/, const likelihood_type& /*likelihood*/) { }

        bool do_decide_null(value_type threshold, [[maybe_unused]] std::size_t row_index, [[maybe_unused]] std::size_t column_index) const noexcept
        {
            if constexpr (t_flavor == generalized_sprt_flavor::general)
                return this->m_unscaled_distance_from_alt > threshold;
            else
                return this->m_is_estimator_low(row_index, column_index) && this->m_unscaled_distance_from_alt > threshold;
        } // do_decide_null(...)

        bool do_decide_alt(value_type threshold, [[maybe_unused]] std::size_t row_index, [[maybe_unused]] std::size_t column_index) const noexcept
        {
            if constexpr (t_flavor == generalized_sprt_flavor::general)
                return this->m_unscaled_distance_from_null > threshold;
            else
                return this->m_is_estimator_high(row_index, column_index) && this->m_unscaled_distance_from_null > threshold;
        } // do_decide_alt(...)

    public:
        generalized_sprt() noexcept : base_type() { }

        /*implicit*/ generalized_sprt(const design_type& design)
            : base_type(design.id()), m_design(design)
        {
            if (design.flavor() != t_flavor) throw std::logic_error("Generalized SPRT flavor mismatch.");
        } // generalized_sprt(...)

        const design_type& design() const noexcept { return this->m_design; }

        std::string to_path_string(std::size_t decimal_places = 3) const noexcept { return this->m_design.to_path_string(decimal_places); }

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            return os << self.m_design;
        } // operator <<(...)
    }; // struct generalized_sprt
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_GENERALIZED_SPRT_HPP_INCLUDED
