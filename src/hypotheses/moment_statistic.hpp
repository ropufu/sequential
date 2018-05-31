
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MOMENT_STATISTIC_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MOMENT_STATISTIC_HPP_INCLUDED

#include <cstddef>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** Not the most accurate but fast statistic builder suitable for some purposes. */
            template <typename t_matrix_type>
            struct moment_statistic
            {
                using type = moment_statistic<t_matrix_type>;
                using matrix_type = t_matrix_type;
                using value_type = typename t_matrix_type::value_type;

            private:
                std::size_t m_count = 0;
                matrix_type m_zero = { };
                matrix_type m_sum = { };
                //matrix_type m_sum_of_squares;
                matrix_type m_shift = { }; // Shift to use in the sum of squares, m.
                matrix_type m_sum_of_shifted_squares = { }; // sum(x - m)^2 = (n - 1) var + n (mean - m)^2

            public:
                moment_statistic() noexcept { }

                explicit moment_statistic(const matrix_type& zero, const matrix_type& anticipated_mean) noexcept
                    : m_zero(zero), m_sum(zero), m_shift(anticipated_mean), m_sum_of_shifted_squares(zero)
                {
                } // moment_statistic(...)

                void clear() noexcept
                {
                    this->m_count = 0;
                    this->m_sum = this->m_zero;
                    this->m_sum_of_shifted_squares = this->m_zero;
                }

                void observe(const matrix_type& value) noexcept
                {
                    matrix_type x = value;
                    x -= this->m_shift;
                    x *= x;

                    this->m_sum += value;
                    this->m_sum_of_shifted_squares += x;
                    this->m_count++;
                }

                std::size_t count() const noexcept { return this->m_count; }
                const matrix_type& sum() const noexcept { return this->m_sum; }
                
                matrix_type sum_of_squares() const noexcept
                {
                    // sum(x)^2 = sum(x - m)^2 + 2 m sum(x) - n m^2.
                    matrix_type sum_of_squares = this->m_sum_of_shifted_squares;

                    matrix_type x = this->m_shift;
                    x *= this->m_sum;
                    x.transform([&](value_type& e) { e *= 2; });
                    sum_of_squares += x;

                    matrix_type y = this->m_shift;
                    y *= y;
                    y.transform([&](value_type& e) { e *= this->m_count; });
                    sum_of_squares -= y;

                    return sum_of_squares;
                }

                matrix_type mean() const noexcept
                {
                    matrix_type x = this->m_sum;
                    x.transform([&](value_type& e) { e /= this->m_count; });
                    return x;
                }

                matrix_type variance() const noexcept
                {
                    if (this->m_count == 0) return this->m_zero;
                    // (n - 1) var = sum(x - m)^2 - n (mean - m)^2
                    matrix_type variance = this->m_sum_of_shifted_squares;

                    matrix_type x = this->m_sum;
                    x.transform([&](value_type& e) { e /= this->m_count; });
                    x -= this->m_shift;
                    x *= x;
                    x.transform([&](value_type& e) { e *= this->m_count; });

                    variance -= x;
                    variance.transform([&](value_type& e) { e /= (this->m_count - 1); });
                    return variance;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MOMENT_STATISTIC_HPP_INCLUDED
