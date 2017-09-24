
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED

#include <cstddef>
#include <iostream>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** Represents a constant signal. */
            struct constant_signal
            {
                typedef constant_signal type;

            private:
                double m_level;
                double m_adjusted_stationary_level;
                std::vector<double> m_adjusted_transition_level;

            public:
                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self)
                {
                    os << "const = { " << self.m_level << " }";
                    return os;
                }

                /** Constant signal when no AR noise is present. */
                constant_signal(double level = 1) noexcept
                    : m_level(level), m_adjusted_stationary_level(level), m_adjusted_transition_level(0)
                {
                    this->m_adjusted_transition_level.shrink_to_fit();
                }

                /** Signal level. */
                double value(std::size_t time_index) const noexcept { return this->m_level; }

                /** Adjusted signal to compensate for AR. */
                double adjusted_value(std::size_t time_index) const noexcept
                {
                    std::size_t count = this->m_adjusted_transition_level.size(); // AR depth.
                    return time_index < count ? this->m_adjusted_transition_level[time_index] : this->m_adjusted_stationary_level;
                }

                /** Configure the AR de-coupling. */
                void set_auto_regression(const std::vector<double>& ar_parameters)
                {
                    double s = this->m_level;
                    std::size_t count = ar_parameters.size(); // AR depth.
                    
                    this->m_adjusted_transition_level.resize(count);
                    for (std::size_t i = 0; i < count; i++)
                    {
                        this->m_adjusted_transition_level[i] = s;
                        s -= ar_parameters[i] * this->m_level;
                    }
                    this->m_adjusted_transition_level.shrink_to_fit();
                    this->m_adjusted_stationary_level = s;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED
