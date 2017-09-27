
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MOMENT_STATISTIC_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MOMENT_STATISTIC_HPP_INCLUDED

#include <cstddef>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** Inaccurate but fast statistic builder suitable for some purposes. */
            template <typename t_data_type>
            struct moment_statistic
            {
                typedef moment_statistic<t_data_type> type;
                typedef t_data_type data_type;
                typedef t_data_type value_type;

            private:
                std::size_t m_count = 0;
                data_type m_zero;
                data_type m_sum;
                //data_type m_sum_of_squares;
                data_type m_shift; // Shift to use in the sum of squares, m.
                data_type m_sum_of_shifted_squares; // sum(x - m)^2 = (n - 1) var + n (mean - m)^2

            public:
                moment_statistic() noexcept
                    : m_zero(), m_sum(), m_shift(), m_sum_of_shifted_squares()
                {
                }

                explicit moment_statistic(const data_type& zero, const data_type& expected_mean) noexcept
                    : m_zero(zero), m_sum(zero), m_shift(expected_mean), m_sum_of_shifted_squares(zero)
                {
                }

                void clear() noexcept
                {
                    this->m_count = 0;
                    this->m_sum = this->m_zero;
                    this->m_sum_of_shifted_squares = this->m_zero;
                }

                void observe(const data_type& value) noexcept
                {
                    data_type x = value;
                    x -= this->m_shift;
                    x *= x;

                    this->m_sum += value;
                    this->m_sum_of_shifted_squares += x;
                    this->m_count++;
                }

                std::size_t count() const noexcept { return this->m_count; }
                const data_type& sum() const noexcept { return this->m_sum; }
                
                data_type sum_of_squares() const noexcept
                {
                    // sum(x)^2 = sum(x - m)^2 + 2 m sum(x) - n m^2.
                    data_type sum_of_squares = this->m_sum_of_shifted_squares;

                    data_type x = this->m_shift;
                    x *= this->m_sum;
                    x *= 2;
                    sum_of_squares += x;

                    data_type y = this->m_shift;
                    y *= y;
                    y *= this->m_count;
                    sum_of_squares -= y;

                    return sum_of_squares;
                }

                data_type mean() const noexcept
                {
                    data_type x = this->m_sum;
                    x /= this->m_count;
                    return x;
                }

                data_type variance() const noexcept
                {
                    if (this->m_count == 0) return this->m_zero;
                    // (n - 1) var = sum(x - m)^2 - n (mean - m)^2
                    data_type variance = this->m_sum_of_shifted_squares;

                    data_type x = this->m_sum;
                    x /= this->m_count;
                    x -= this->m_shift;
                    x *= x;
                    x *= this->m_count;

                    variance -= x;
                    variance /= (this->m_count - 1);
                    return variance;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MOMENT_STATISTIC_HPP_INCLUDED
