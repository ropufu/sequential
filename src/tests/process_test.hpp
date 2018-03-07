
#ifndef ROPUFU_SEQUENTIAL_TESTS_PROCESS_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_PROCESS_TEST_HPP_INCLUDED

#include "../hypotheses/de_auto_regress.hpp"
#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/process.hpp"
#include "generator.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses_test
        {
            struct process_test
            {
                using type = process_test;
                using value_type = double;

                template <typename t_process_type>
                static bool test_tic(t_process_type& proc, std::size_t max_time = 100)
                {
                    using signal_type = typename t_process_type::signal_type;
                    using noise_type = typename t_process_type::noise_type;

                    for (std::size_t time = 0; time < max_time; ++time) proc.tic();
                    if (proc.count() != max_time) return false;
                    proc.reset();

                    auto adjusted_proc = adjust_process(proc);

                    for (std::size_t time = 0; time < max_time; ++time) adjusted_proc.tic();
                    if (adjusted_proc.count() != max_time) return false;
                    adjusted_proc.reset();

                    return true;
                }

                static bool test_constant_white() noexcept
                {
                    using signal_type = hypotheses::constant_signal<value_type>;
                    using noise_type = hypotheses::white_noise<value_type>;
                    using tested_type = hypotheses::process<signal_type, noise_type>;

                    value_type sigma_one = 1;
                    value_type sigma_two = 2;

                    signal_type s((sigma_one + sigma_two) / 2);
                    noise_type n1(sigma_one);
                    noise_type n2(sigma_two);

                    tested_type x(s, n1, 0.1);
                    tested_type y(s, n2, 0.5);
                    tested_type z { };

                    if (!type::test_tic(x)) return false;
                    if (!type::test_tic(y)) return false;
                    if (!type::test_tic(z)) return false;
                    return true;
                } // test_constant_white(...)

                template <std::size_t t_ar_size>
                static bool test_constant_ar() noexcept
                {
                    using signal_type = hypotheses::constant_signal<value_type>;
                    using noise_type = hypotheses::auto_regressive_noise<value_type, t_ar_size>;
                    using tested_type = hypotheses::process<signal_type, noise_type>;
                    using noise_generator_type = generator<value_type, t_ar_size>;
                    
                    value_type sigma_one = 1;
                    value_type sigma_two = 2;

                    signal_type s((sigma_one + sigma_two) / 2);
                    noise_type n1(sigma_one, noise_generator_type::make_lin_array());
                    noise_type n2(sigma_two, noise_generator_type::make_quad_array());

                    tested_type x(s, n1, 0.1);
                    tested_type y(s, n2, 0.5);
                    tested_type z { };

                    if (!type::test_tic(x)) return false;
                    if (!type::test_tic(y)) return false;
                    if (!type::test_tic(z)) return false;
                    return true;
                } // test_constant_ar(...)

                template <std::size_t t_transition_size>
                static bool test_transit_white() noexcept
                {
                    using signal_type = hypotheses::transitionary_signal<value_type, t_transition_size>;
                    using noise_type = hypotheses::white_noise<value_type>;
                    using tested_type = hypotheses::process<signal_type, noise_type>;
                    using signal_generator_type = generator<value_type, t_transition_size>;
                    
                    value_type level_one = 1;
                    value_type level_two = 2;

                    signal_type s1(level_one, signal_generator_type::make_lin_array());
                    signal_type s2(level_two, signal_generator_type::make_quad_array());
                    noise_type n(0.4);

                    tested_type x(s1, n, 0.1);
                    tested_type y(s2, n, 0.5);
                    tested_type z { };

                    if (!type::test_tic(x)) return false;
                    if (!type::test_tic(y)) return false;
                    if (!type::test_tic(z)) return false;
                    return true;
                } // test_transit_white(...)

                template <std::size_t t_ar_size, std::size_t t_transition_size>
                static bool test_transit_ar() noexcept
                {
                    using signal_type = hypotheses::transitionary_signal<value_type, t_transition_size>;
                    using noise_type = hypotheses::auto_regressive_noise<value_type, t_ar_size>;
                    using tested_type = hypotheses::process<signal_type, noise_type>;
                    using signal_generator_type = generator<value_type, t_transition_size>;
                    using noise_generator_type = generator<value_type, t_ar_size>;
                    
                    value_type level_one = 1;
                    value_type level_two = 3;
                    value_type sigma_one = 2;
                    value_type sigma_two = 4;

                    signal_type s1(level_one, signal_generator_type::make_lin_array());
                    signal_type s2(level_two, signal_generator_type::make_quad_array());
                    noise_type n1(sigma_one, noise_generator_type::make_lin_array());
                    noise_type n2(sigma_two, noise_generator_type::make_quad_array());

                    tested_type u(s1, n1, 0.1);
                    tested_type v(s2, n1, 0.5);
                    tested_type x(s1, n2, 0.1);
                    tested_type y(s2, n2, 0.5);
                    tested_type z { };

                    if (!type::test_tic(u)) return false;
                    if (!type::test_tic(v)) return false;
                    if (!type::test_tic(x)) return false;
                    if (!type::test_tic(y)) return false;
                    if (!type::test_tic(z)) return false;
                    return true;
                } // test_transit_ar(...)

                static bool print() noexcept
                {
                    auto x = generator<value_type, 3>::constant_white();
                    auto y = generator<value_type, 0>::constant_ar();
                    auto z = generator<value_type, 5>::constant_ar();
                    auto v = generator<value_type, 4>::transit_white();
                    auto w = generator<value_type, 2>::transit_ar();

                    test_ostream(x, json_round_trip(x));
                    test_ostream(y, json_round_trip(y));
                    test_ostream(z, json_round_trip(z));
                    test_ostream(v, json_round_trip(v));
                    test_ostream(w, json_round_trip(w));
                    return true;
                } // print(...)
            }; // struct process_test
        } // namespace hypotheses_test
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_TESTS_PROCESS_TEST_HPP_INCLUDED
