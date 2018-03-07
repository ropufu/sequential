
#ifndef ROPUFU_SEQUENTIAL_TESTS_SIGNAL_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_SIGNAL_TEST_HPP_INCLUDED

#include "../hypotheses/signals.hpp"
#include "generator.hpp"

#include <array>
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
            struct signal_test
            {
                using type = signal_test;
                using value_type = double;

                static bool test_constant() noexcept
                {
                    using tested_type = hypotheses::constant_signal<value_type>;
                    
                    value_type level_one = 3;
                    value_type level_two = -6;

                    tested_type x(level_one);
                    tested_type y(level_two);

                    for (std::size_t time = 0; time < 10; ++time)
                    {
                        if (x.at(time) != level_one) return false;
                        if (y.at(time) != level_two) return false;
                    }
                    return true;
                } // test_constant_no_ar(...)

                template <std::size_t t_transition_size>
                static bool test_transitionary() noexcept
                {
                    using tested_type = hypotheses::transitionary_signal<value_type, t_transition_size>;
                    using generator_type = generator<value_type, t_transition_size>;

                    value_type level_one = 3;
                    value_type level_two = -6;

                    std::array<value_type, t_transition_size> transition = generator_type::make_lin_array();

                    tested_type x(level_one, transition);
                    tested_type y(level_two, transition);

                    for (std::size_t time = 0; time < t_transition_size; ++time)
                    {
                        if (x.at(time) != transition[time]) return false;
                        if (y.at(time) != transition[time]) return false;
                    }
                    for (std::size_t time = t_transition_size; time < t_transition_size + 10; ++time)
                    {
                        if (x.at(time) != level_one) return false;
                        if (y.at(time) != level_two) return false;
                    }
                    return true;
                } // test_transitionary(...)

                static bool print() noexcept
                {
                    hypotheses::constant_signal<value_type> x(3);
                    hypotheses::transitionary_signal<value_type, 0> y(2, generator<value_type, 0>::make_lin_array());
                    hypotheses::transitionary_signal<value_type, 4> z(1, generator<value_type, 4>::make_lin_array());
                    hypotheses::transitionary_signal<value_type, 2> w(5, generator<value_type, 2>::make_quad_array());

                    test_ostream(x, json_round_trip(x));
                    test_ostream(y, json_round_trip(y));
                    test_ostream(z, json_round_trip(z));
                    test_ostream(w, json_round_trip(w));
                    return true;
                } // print(...)
            }; // struct signal_test
        } // namespace hypotheses_test
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_TESTS_SIGNAL_TEST_HPP_INCLUDED
