
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED

#include <aftermath/probability.hpp>

#include <cstddef>
#include <iostream>
#include <random>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** Describes the hypotheses testing setup. */
            template <typename t_signal_type>
            struct model
            {
                using type = model;
                using signal_type = t_signal_type;
                using noise_distribution_type = ropufu::aftermath::probability::dist_normal; // For aftermath engine.
                // using noise_distribution_type = std::normal_distribution<double>; // For built-in c++ engine.

            private:
                signal_type m_signal; // Signal.
                double m_null_mu; // Signal "strength" under the null hypothesis.
                double m_alt_mu; // Minimal signal "strength" under the alternative hypothesis.
                double m_noise_sigma; // Standard deviation of noise.
                double m_noise_variance; // Variance deviation of noise.
                std::vector<double> m_ar_parameters; // Parameters in the AR noise.

            public:
                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self)
                {
                    // Output AR parameters.
                    bool is_first = true;
                    os << "AR parameters : [";
                    for (double rho : self.m_ar_parameters)
                    {
                        if (!is_first) os << ", ";
                        os << rho;
                        is_first = false;
                    }
                    os << "]";
                    // Output other settings.
                    os << ", mu : " << self.m_null_mu << " vs. " << self.m_alt_mu;
                    os << ", noise sigma : " << self.m_noise_sigma << ".";
                    return os;
                }

                std::ostringstream& mat_prefix(std::ostringstream& os) const noexcept
                {
                    os << (this->m_ar_parameters.empty() ? "no_ar" : "ar");
                    for (double rho : this->m_ar_parameters) os << "_" << rho;
                    os << "_mu_null_" << this->m_null_mu << "_alt_" << this->m_alt_mu;
                    return os;
                }

                /** Constant signal when no AR noise is present. */
                model(const signal_type& signal, double null_mu, double alt_mu, double noise_sigma, const std::vector<double>& ar_parameters) noexcept
                    : m_signal(signal), 
                    m_null_mu(null_mu), m_alt_mu(alt_mu), 
                    m_noise_sigma(noise_sigma), m_noise_variance(noise_sigma * noise_sigma), 
                    m_ar_parameters(ar_parameters)
                {
                    this->m_signal.set_auto_regression(this->m_ar_parameters);
                }
                
                /** Signal "strength" under the null hypothesis. */
                double mu_under_null() const noexcept { return this->m_null_mu; }
                /** Minimal signal "strength" under the alternative hypothesis. */
                double mu_under_alt() const noexcept { return this->m_alt_mu; }
                /** Determines if the provided signal "strength" falls into the null category. */
                bool is_null(double theta) const noexcept { return theta == this->m_null_mu; }
                /** Determines if the provided signal "strength" falls into the alternative category. */
                bool is_alt(double theta) const noexcept { return theta >= this->m_alt_mu; }

                /** Standard deviation of noise. */
                double noise_sigma() const noexcept { return this->m_noise_sigma; }
                /** Variance of noise. */
                double noise_variance() const noexcept { return this->m_noise_variance; }

                /** Scaling factor for LLR. */
                double log_likelihood_scale() const noexcept { return this->m_noise_variance; }

                /** AR parameters for noise. */
                const std::vector<double>& ar_parameters() const noexcept { return this->m_ar_parameters; }

                /** Deterministic signal. */
                double signal(std::size_t time_index) const noexcept
                {
                    return this->m_signal.value(time_index);
                    //return 1.0;
                }

                /** Adjust the sequence to compensate for AR noise. */
                double adjusted_signal(std::size_t time_index) const noexcept
                {
                    return this->m_signal.adjusted_value(time_index);
                }
                
                /** @brief Adjust the sequence to compensate for AR noise.
                 *  @remark \tparam t_sequence_type has to implement operator [std::size_t] -> double.
                 */
                template <typename t_sequence_type>
                double adjust(const t_sequence_type& sequence, std::size_t time_index) const
                {
                    const std::vector<double>& rho = this->m_ar_parameters;
                    double s = sequence[time_index];
                    
                    //
                    //  ...-------- AR elements --->|                 
                    //       |<---- t elements ---->|                 
                    // ------|----|----|--...--|----|----|-----> time 
                    //       0    1    2  ... t-2  t-1   t            
                    //
                    std::size_t count = rho.size(); // How many elements should we take from the past.
                    if (count > time_index) count = time_index; // Take at most <time_index> observations from the past.

                    for (std::size_t i = 0; i < count; i++) s -= rho[i] * sequence[time_index - 1 - i];
                    return s;
                }

                /** @brief Build AR process from an i.i.d. sequence.
                 *  @param latest The latest observation from the i.i.d. sequence.
                 *  @param sequence The AR sequence.
                 *  @remark \tparam t_sequence_type has to implement operator [std::size_t] -> double.
                 */
                template <typename t_sequence_type>
                double auto_regress(double latest, const t_sequence_type& sequence, std::size_t time_index) const
                {
                    const std::vector<double>& rho = this->m_ar_parameters;
                    double s = latest;
                    
                    //
                    //  ...-------- AR elements --->|                 
                    //       |<---- t elements ---->|                 
                    // ------|----|----|--...--|----|----|-----> time 
                    //       0    1    2  ... t-2  t-1   t            
                    //
                    std::size_t count = rho.size(); // How many elements should we take from the past.
                    if (count > time_index) count = time_index; // Take at most <time_index> observations from the past.

                    for (std::size_t i = 0; i < count; i++) s += rho[i] * sequence[time_index - 1 - i];
                    return s;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
