
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_HPP_INCLUDED

#include "not_an_error.hpp"
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
            struct generalized_sprt : public two_sprt<generalized_sprt<t_signal_type>, t_signal_type>
            {
                typedef generalized_sprt<t_signal_type> type;
                typedef two_sprt<generalized_sprt<t_signal_type>, t_signal_type> base_type;
                
                typedef t_signal_type signal_type;
                typedef model<signal_type> model_type;
                typedef process<signal_type> process_type;
                friend struct two_sprt<type, signal_type>;

            private:
                double m_unscaled_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator.
                double m_unscaled_distance_from_alt = 0; // Latest (unscaled) LLR vs. alt estimator.

                void post_process_thresholds(std::vector<double>& null_thresholds, std::vector<double>& alt_thresholds) noexcept
                {
                    double factor = this->model().log_likelihood_scale();
                    for (double& a : null_thresholds) a *= factor;
                    for (double& a : alt_thresholds) a *= factor;
                }

                void observe_unchecked(const process_type& proc) noexcept
                {
                    std::size_t count = proc.count();
                    std::size_t time_index = count - 1;

                    double null_mu = this->model().mu_under_null();
                    double alt_mu = this->model().mu_under_alt();
                    double mu_hat = proc.null_estimator_of_mu(time_index);
                    double mu_alt_hat = (mu_hat < alt_mu) ? alt_mu : mu_hat;
                    
                    this->m_unscaled_distance_from_null = proc.unscaled_log_likelihood_between(mu_hat, null_mu);
                    this->m_unscaled_distance_from_alt = proc.unscaled_log_likelihood_between(mu_hat, mu_alt_hat);
                }

                void reset_unchecked() noexcept
                {
                    this->m_unscaled_distance_from_null = 0;
                    this->m_unscaled_distance_from_alt = 0;
                }

            public:
                explicit generalized_sprt(const model_type& model) noexcept
                    : base_type(model)
                {
                }

                bool do_decide_null(double threshold) const noexcept { return this->m_unscaled_distance_from_alt > threshold; }
                bool do_decide_alt(double threshold) const noexcept { return this->m_unscaled_distance_from_null > threshold; }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_HPP_INCLUDED
