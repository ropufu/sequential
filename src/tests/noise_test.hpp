
#ifndef ROPUFU_SEQUENTIAL_TESTS_NOISE_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_NOISE_TEST_HPP_INCLUDED

#include "../hypotheses/noises.hpp"
#include "generator.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <system_error> // std::error_code, std::errc
#include <vector>

namespace ropufu::sequential
{
    namespace hypotheses_test
    {
        struct noise_test
        {
            using type = noise_test;
            using value_type = double;

            static bool test_white() noexcept
            {
                using tested_type = hypotheses::white_noise<value_type>;
                std::error_code ec {};

                value_type sigma_one = 1;
                value_type sigma_two = 2;

                tested_type x(sigma_one, ec);
                tested_type y(sigma_two, ec);
                tested_type z {};

                std::size_t max_time = 100;
                for (std::size_t time = 0; time < max_time; ++time)
                {
                    x.tic();
                    y.tic();
                    z.tic();
                } // for (...)
                x.reset();
                y.reset();
                z.reset();
                return (ec.value() == 0);
            } // test_no_ar(...)

            template <std::size_t t_ar_size>
            static bool test_ar() noexcept
            {
                using tested_type = hypotheses::auto_regressive_noise<value_type, t_ar_size>;
                using generator_type = generator<value_type, t_ar_size>;
                std::error_code ec {};

                value_type sigma_one = 1;
                value_type sigma_two = 2;

                tested_type x(sigma_one, generator_type::make_lin_array(), ec);
                tested_type y(sigma_two, generator_type::make_quad_array(), ec);
                tested_type z {};

                std::size_t max_time = 100;
                for (std::size_t time = 0; time < max_time; ++time)
                {
                    x.tic();
                    y.tic();
                    z.tic();
                } // for (...)
                //if (x.count() != max_time) return false;
                //if (y.count() != max_time) return false;
                //if (z.count() != max_time) return false;
                return (ec.value() == 0);
            } // test_no_ar(...)

            static bool print() noexcept
            {
                std::error_code ec {};

                hypotheses::white_noise<value_type> x(3, ec);
                hypotheses::auto_regressive_noise<value_type, 0> y(2, generator<value_type, 0>::make_lin_array(), ec);
                hypotheses::auto_regressive_noise<value_type, 4> z(1, generator<value_type, 4>::make_lin_array(), ec);
                hypotheses::auto_regressive_noise<value_type, 2> w(5, generator<value_type, 2>::make_quad_array(), ec);

                test_ostream(x, json_round_trip(x));
                test_ostream(y, json_round_trip(y));
                test_ostream(z, json_round_trip(z));
                test_ostream(w, json_round_trip(w));

                return (ec.value() == 0);
            } // print(...)
        }; // struct noise_test
    } // namespace hypotheses_test
} // namespace ropufu::sequential

#endif // ROPUFU_SEQUENTIAL_TESTS_NOISE_TEST_HPP_INCLUDED
