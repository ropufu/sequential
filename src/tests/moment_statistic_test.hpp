
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
            
            template <typename t_scalar_type>
            using matrix_t = aftermath::algebra::matrix<t_scalar_type>;

            template <typename t_observation_type, typename t_statistic_type, std::size_t t_order>
            static bool test_scalar() noexcept
            {
                using tested_type = hypotheses::moment_statistic<t_observation_type, t_statistic_type, t_order>;
                t_statistic_type zero {};

                for (t_statistic_type anticipated_mean = 0; anticipated_mean < 15; ++anticipated_mean)
                {
                    tested_type stat { zero, anticipated_mean };

                    std::size_t n = 100;
                    std::vector<t_observation_type> history {};
                    t_statistic_type sum = 0;
                    
                    history.reserve(n);

                    for (std::size_t time = 0; time < n; ++time)
                    {
                        t_observation_type x = static_cast<t_observation_type>(time + time * time + (time % 15));
                        stat.observe(x);
                        history.push_back(x);
                        sum += static_cast<t_statistic_type>(x);
                    } // for (...)

                    t_statistic_type mean = sum / static_cast<t_statistic_type>(n);
                    t_statistic_type variance = 0;
                    for (const t_observation_type& x : history)
                    {
                        t_statistic_type y = static_cast<t_statistic_type>(x);
                        variance += ((y - mean) * (y - mean));
                    }
                    variance /= static_cast<t_statistic_type>(n - 1);

                    t_statistic_type e1 = abs((stat.mean() - mean) / mean);
                    t_statistic_type e2 = abs((stat.variance() - variance) / variance);
                    
                    if (e1 > 0.00001) return false;
                    if (e2 > 0.0001) return false;
                } // for (...)
                return true;
            } // test_scalar(...)

            template <typename t_observation_type, typename t_statistic_type, std::size_t t_order, std::size_t t_height, std::size_t t_width>
            static bool test_matrix() noexcept
            {
                using tested_type = hypotheses::moment_statistic<matrix_t<t_observation_type>, matrix_t<t_statistic_type>, t_order>;
                matrix_t<t_statistic_type> zero { t_height, t_width };

                for (t_statistic_type am = 0; am < 15; ++am)
                {
                    matrix_t<t_statistic_type> anticipated_mean { t_height, t_width };
                    anticipated_mean.fill(am);

                    tested_type stat { zero, anticipated_mean };

                    std::size_t n = 100;
                    std::vector<matrix_t<t_observation_type>> history {};
                    matrix_t<t_statistic_type> sum { t_height, t_width };
                    
                    history.reserve(n);

                    for (std::size_t time = 0; time < n; ++time)
                    {
                        t_observation_type sc = static_cast<t_observation_type>(time + time * time + (time % 15));
                        matrix_t<t_observation_type> x { t_height, t_width };
                        x.fill(sc);
                        stat.observe(x);
                        history.push_back(x);
                        
                        matrix_t<t_statistic_type> y { x };
                        sum += y;
                    } // for (...)

                    matrix_t<t_statistic_type> mean = sum / static_cast<t_statistic_type>(n);
                    matrix_t<t_statistic_type> variance { t_height, t_width };
                    for (const matrix_t<t_observation_type>& x : history)
                    {
                        matrix_t<t_statistic_type> y = static_cast<matrix_t<t_statistic_type>>(x);
                        y -= mean;
                        y *= y;
                        variance += y;
                    } // for (...)
                    variance /= static_cast<t_statistic_type>(n - 1);

                    matrix_t<t_statistic_type> e1 = stat.mean() - mean;
                    e1 /= mean;
                    matrix_t<t_statistic_type> e2 = stat.variance() - variance;
                    e2 /= variance;

                    for (t_statistic_type& x : e1) if (x < 0) x = (-x);
                    for (t_statistic_type& x : e2) if (x < 0) x = (-x);

                    for (t_statistic_type& x : e1) if (x > 0.00001) return false;
                    for (t_statistic_type& x : e2) if (x > 0.0001) return false;
                } // for (...)
                return true;
            } // test_scalar(...)
        }; // struct moment_statistic_test
    } // namespace hypotheses_test
} // namespace ropufu::sequential

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TEST_MOMENT_STATISTIC_TEST_HPP_INCLUDED
