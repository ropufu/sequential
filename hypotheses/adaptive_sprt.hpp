
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_HPP_INCLUDED

#include <aftermath/not_an_error.hpp>

#include "process.hpp"
#include "two_sprt.hpp"

#include <cstddef>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            template <typename t_signal_type>
            struct adaptive_sprt : public two_sprt<adaptive_sprt<t_signal_type>, t_signal_type>
            {
                using type = adaptive_sprt<t_signal_type>;
                using base_type = two_sprt<adaptive_sprt<t_signal_type>, t_signal_type>;
                
                using signal_type = t_signal_type;
                using model_type = hypotheses::model<signal_type>;
                using process_type = process<signal_type>;
                friend struct two_sprt<type, signal_type>;

            private:
                double m_guess_mu_for_null;
                double m_guess_mu_for_alt;
                double m_mu_estimator_for_llr_null; // Latest signal "strength" estimator to be used in LLR vs. null estimator.
                double m_mu_estimator_for_llr_alt; // Latest signal "strength" estimator to be used in LLR vs. alt estimator.
                double m_unscaled_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator.
                double m_unscaled_distance_from_null_with_alt_init = 0; // Latest (unscaled) LLR vs. null estimator with an alt initial guess for signal "strength".
                double m_unscaled_distance_from_alt = 0; // Latest (unscaled) LLR vs. alt estimator.

                void observe_unchecked(const process_type& proc) noexcept
                {
                    std::size_t count = proc.count();
                    std::size_t time_index = count - 1;

                    double null_mu = this->model().mu_under_null();
                    double alt_mu = this->model().mu_under_alt();
                    double mu_hat = proc.null_estimator_of_mu(time_index);
                    double mu_alt_hat = (mu_hat < alt_mu) ? alt_mu : mu_hat;
                    
                    // Latest (unscaled) LLR vs. alt estimator.
                    this->m_unscaled_distance_from_null += proc.unscaled_log_likelihood_at(time_index, this->m_mu_estimator_for_llr_null, null_mu);
                    // Latest (unscaled) LLR vs. alt estimator.
                    this->m_unscaled_distance_from_null_with_alt_init += proc.unscaled_log_likelihood_at(time_index, this->m_mu_estimator_for_llr_alt, null_mu);
                    this->m_unscaled_distance_from_alt = 
                        this->m_unscaled_distance_from_null_with_alt_init - 
                        proc.unscaled_log_likelihood_between(mu_alt_hat, null_mu);

                    // Update the delayed signal "strength" estimators.
                    this->m_mu_estimator_for_llr_null = mu_hat;
                    this->m_mu_estimator_for_llr_alt = mu_hat;
                }

                void reset_unchecked() noexcept
                {
                    this->m_mu_estimator_for_llr_null = this->m_guess_mu_for_null;
                    this->m_mu_estimator_for_llr_alt = this->m_guess_mu_for_alt;
                    this->m_unscaled_distance_from_null = 0;
                    this->m_unscaled_distance_from_null_with_alt_init = 0;
                    this->m_unscaled_distance_from_alt = 0;
                }

            public:
                explicit adaptive_sprt(const model_type& model, double guess_mu_for_null, double guess_mu_for_alt) noexcept
                    : base_type(model),
                    m_guess_mu_for_null(guess_mu_for_null), 
                    m_guess_mu_for_alt(guess_mu_for_alt),
                    m_mu_estimator_for_llr_null(guess_mu_for_null), 
                    m_mu_estimator_for_llr_alt(guess_mu_for_alt)
                {
                }

                bool do_decide_null(double threshold) const noexcept { return this->m_unscaled_distance_from_alt > threshold; }
                bool do_decide_alt(double threshold) const noexcept { return this->m_unscaled_distance_from_null > threshold; }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_HPP_INCLUDED
