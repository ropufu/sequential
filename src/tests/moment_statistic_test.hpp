
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TEST_MOMENT_STATISTIC_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TEST_MOMENT_STATISTIC_TEST_HPP_INCLUDED

#include <ropufu/algebra.hpp>  // aftermath::algebra::matrix

#include "../hypotheses/moment_statistic.hpp"

#include <cstddef>      // std::size_t
#include <system_error> // std::error_code, std::errc
#include <vector>       // std::vector

namespace ropufu::sequential
{
    namespace hypotheses_test
    {
        template <typename t_scalar_type>
        inline t_scalar_type abs(t_scalar_type value) { return value < 0 ? (-value) : value; }

        struct moment_statistic_test
        {
            using type = moment_statistic_test;
            using scalar_type = double;
            using matrix_type = aftermath::algebra::matrix<scalar_type>;

            template <std::size_t t_order>
            static bool test_scalar() noexcept
            {
                using tested_type = hypotheses::moment_statistic<scalar_type, t_order>;
                scalar_type zero {};

                for (scalar_type anticipated_mean = 0; anticipated_mean < 15; ++anticipated_mean)
                {
                    tested_type stat { zero, anticipated_mean };

                    std::size_t n = 100;
                    std::vector<scalar_type> history {};
                    scalar_type sum = 0;
                    
                    history.reserve(n);

                    for (std::size_t time = 0; time < n; ++time)
                    {
                        scalar_type x = static_cast<scalar_type>(time + time * time + (time % 15));
                        stat.observe(x);

                        history.push_back(x);
                        sum += x;
                    } // for (...)

                    scalar_type mean = sum / static_cast<scalar_type>(n);
                    scalar_type variance = 0;
                    for (const scalar_type& x : history) variance += (x - mean) * (x - mean);
                    variance /= static_cast<scalar_type>(n - 1);

                    scalar_type e1 = abs(stat.mean() - mean);
                    scalar_type e2 = abs(stat.variance() - variance);

                    if (e1 > 0.001) return false;
                    if (e2 > 0.01) return false;
                } // for (...)
                return true;
            } // test_scalar(...)

            template <std::size_t t_order, std::size_t t_height, std::size_t t_width>
            static bool test_matrix() noexcept
            {
                using tested_type = hypotheses::moment_statistic<matrix_type, t_order>;
                matrix_type zero { t_height, t_width };

                for (scalar_type am = 0; am < 15; ++am)
                {
                    matrix_type anticipated_mean { t_height, t_width };
                    anticipated_mean.fill(am);

                    tested_type stat { zero, anticipated_mean };

                    std::size_t n = 100;
                    std::vector<matrix_type> history {};
                    matrix_type sum { t_height, t_width };
                    
                    history.reserve(n);

                    for (std::size_t time = 0; time < n; ++time)
                    {
                        scalar_type sc = static_cast<scalar_type>(time + time * time + (time % 15));
                        matrix_type x { t_height, t_width };
                        x.fill(sc);

                        stat.observe(x);

                        history.push_back(x);
                        sum += x;
                    } // for (...)

                    matrix_type mean = sum / static_cast<scalar_type>(n);
                    matrix_type variance { t_height, t_width };
                    for (const matrix_type& x : history) variance += (x - mean) * (x - mean);
                    variance /= static_cast<scalar_type>(n - 1);

                    matrix_type e1 = stat.mean() - mean;
                    matrix_type e2 = stat.variance() - variance;

                    for (scalar_type& x : e1) if (x < 0) x = (-x);
                    for (scalar_type& x : e2) if (x < 0) x = (-x);

                    for (scalar_type& x : e1) if (x > 0.001) return false;
                    for (scalar_type& x : e2) if (x > 0.01) return false;
                } // for (...)
                return true;
            } // test_scalar(...)
        }; // struct moment_statistic_test
    } // namespace hypotheses_test
} // namespace ropufu::sequential

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TEST_MOMENT_STATISTIC_TEST_HPP_INCLUDED
