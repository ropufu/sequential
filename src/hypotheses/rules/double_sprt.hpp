
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/probability/standard_normal_distribution.hpp>
#include <ropufu/number_traits.hpp>

#include "double_sprt_design.hpp"
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
    template <typename t_engine_type, typename t_value_type>
    struct double_sprt;

    template <typename t_engine_type, typename t_value_type>
    struct double_sprt : public two_sprt<
        double_sprt<t_engine_type, t_value_type>,
        t_engine_type, t_value_type>
    {
        using type = double_sprt<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using base_type = two_sprt<type, engine_type, value_type>;
        friend base_type;

        using model_type = model<value_type>;
        using likelihood_type = likelihood<value_type>;
        using process_type = simple_process<engine_type, value_type>;
        using change_of_measure_type = change_of_measure<value_type>;
        using design_type = double_sprt_design<value_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;

    private:
        // ~~ Fundamental members ~~
        design_type m_design = {};
        matrix_t<value_type> m_mu_intermediate = {}; // Threshold used to measure distance from.

        // ~~ Members reset with each \c toc() ~~
        matrix_t<value_type> m_unscaled_distance_from_null = {}; // Latest (unscaled) LLR vs. null estimator.
        matrix_t<value_type> m_unscaled_distance_from_alt = {};  // Latest (unscaled) LLR vs. alt estimator.

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
            this->m_mu_intermediate = matrix_t<value_type>(m, n);
            this->m_unscaled_distance_from_null = matrix_t<value_type>(m, n);
            this->m_unscaled_distance_from_alt = matrix_t<value_type>(m, n);

            if (this->m_design.is_threshold_independent())
            {
                value_type mu_intermediate = model.mu_relative(this->m_design.relative_mu_intermediate());
                this->m_mu_intermediate.fill(mu_intermediate);
            } // if (...)
            else
            {
                value_type null_mu = model.mu_under_null();
                value_type alt_mu = model.smallest_mu_under_alt();
                value_type mu_diff = alt_mu - null_mu;

                if (this->m_design.huffman_correction())
                {
                    aftermath::probability::standard_normal_distribution<value_type> standard_normal {};
                    for (std::size_t i = 0; i < m; ++i)
                    {
                        for (std::size_t j = 0; j < n; ++j)
                        {
                            value_type x = 1 + std::sqrt(unscaled_null_thresholds[i] / unscaled_alt_thresholds[j]);
                            value_type r = standard_normal.quantile(1 / x);
                            value_type delta_mu_star = mu_diff / x;
							value_type mu_star = null_mu + delta_mu_star;
                            value_type mu_tilde = mu_star + r * delta_mu_star / std::sqrt(2 * unscaled_null_thresholds[i]);
                            if (mu_tilde > alt_mu) mu_tilde = alt_mu;
                            this->m_mu_intermediate(i, j) = mu_tilde;
                        } // for (...)
                    } // for (...)
                } // if (...)
                else
                {
                    for (std::size_t i = 0; i < m; ++i)
                        for (std::size_t j = 0; j < n; ++j)
                            this->m_mu_intermediate(i, j) = null_mu + mu_diff / (1 + std::sqrt(unscaled_null_thresholds[i] / unscaled_alt_thresholds[j]));
                } // else (...)
            } // else (...)
        } // on_initialized(...)

        /** @brief Auxiliary function to be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            this->m_unscaled_distance_from_null.fill(0);
            this->m_unscaled_distance_from_alt.fill(0);
        } // on_reset(...)

        /** @brief Auxiliary function to be executed right after the \c tic() call. */
        void on_tic(const process_type& proc, const likelihood_type& likelihood)
        {
            value_type null_mu = likelihood.model().mu_under_null();
            value_type alt_mu = likelihood.model().smallest_mu_under_alt();

            for (std::size_t i = 0; i < this->m_mu_intermediate.height(); ++i)
            {
                for (std::size_t j = 0; j < this->m_mu_intermediate.width(); ++j)
                {
                    this->m_unscaled_distance_from_null(i, j) = proc.unscaled_log_likelihood_between(this->m_mu_intermediate(i, j), null_mu);
                    this->m_unscaled_distance_from_alt(i, j) = proc.unscaled_log_likelihood_between(this->m_mu_intermediate(i, j), alt_mu);
                } // for (...)
            } // for (...)
        } // on_tic(...)

        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& /*proc*/, const likelihood_type& /*likelihood*/) { }

        bool do_decide_null(value_type threshold, std::size_t row_index, std::size_t column_index) const noexcept
        {
            return this->m_unscaled_distance_from_alt(row_index, column_index) > threshold;
        } // do_decide_null(...)

        bool do_decide_alt(value_type threshold, std::size_t row_index, std::size_t column_index) const noexcept
        {
            return this->m_unscaled_distance_from_null(row_index, column_index) > threshold;
        } // do_decide_alt(...)

    public:
        double_sprt() noexcept : base_type() { }

        /*implicit*/ double_sprt(const design_type& design) noexcept
            : base_type(design.id()), m_design(design)
        {
        } // double_sprt(...)

        const design_type& design() const noexcept { return this->m_design; }

        std::string to_path_string(std::size_t decimal_places = 3) const noexcept { return this->m_design.to_path_string(decimal_places); }

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct double_sprt
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_HPP_INCLUDED
