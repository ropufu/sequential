
#ifndef ROPUFU_SEQUENTIAL_TESTS_MONTE_CARLO_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_MONTE_CARLO_TEST_HPP_INCLUDED

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/process.hpp"
#include "../hypotheses/model.hpp"
#include "../hypotheses/rules.hpp"
#include "../hypotheses/monte_carlo.hpp"
#include "generator.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses_test
        {
            struct monte_carlo_test
            {
                using type = monte_carlo_test;
                using value_type = double;
                
                template <typename t_process_type>
                static bool test_with_process(t_process_type& proc) noexcept
                {
                    using asprt_type = hypotheses::adaptive_sprt_t<t_process_type>;
                    using bsprt_type = hypotheses::adaptive_sprt_star_t<t_process_type>;
                    using csprt_type = hypotheses::adaptive_sprt_star_t<t_process_type>;
                    using gsprt_type = hypotheses::generalized_sprt_t<t_process_type>;
                    using hsprt_type = hypotheses::generalized_sprt_star_t<t_process_type>;
                    
                    using xsprt_type = hypotheses::xsprt_t<t_process_type>;
                    using tested_type = hypotheses::monte_carlo_t<t_process_type>;

                    // Create rules.
                    value_type theta = static_cast<value_type>(0.5);
                    
                    asprt_type x { 0, theta, theta };
                    bsprt_type y { 1, theta, theta };
                    csprt_type z { 2, theta, theta };
                    gsprt_type v { 3 };
                    hsprt_type w { 4, theta };

                    // List rules.
                    std::vector<xsprt_type> rules = { x, y, z, v, w };

                    // Output stream.
                    std::ostringstream ss { };

                    // Monte Carlo.
                    tested_type mc(1'000);
                    mc.run(proc, rules,
                        [&] () {
                            ss << "Simulation start." << std::endl;
                            for (auto& r : rules) generator<value_type, 1>::reset_rule(r, proc);
                        },
                        [&] () {
                            ss << "Simulation stop." << std::endl;
                        });
                    
                    return true;
                }

                static bool test_all() noexcept
                {
                    auto a = generator<value_type, 3>::constant_white();
                    auto b = generator<value_type, 0>::constant_ar();
                    auto c = generator<value_type, 5>::constant_ar();
                    auto d = generator<value_type, 4>::transit_white();
                    auto e = generator<value_type, 2>::transit_ar();

                    if (!type::test_with_process(a)) return false;
                    if (!type::test_with_process(b)) return false;
                    if (!type::test_with_process(c)) return false;
                    if (!type::test_with_process(d)) return false;
                    if (!type::test_with_process(e)) return false;
                    return true;
                } // print(...)
            }; // struct monte_carlo_test
        } // namespace hypotheses_test
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_TESTS_MONTE_CARLO_TEST_HPP_INCLUDED
