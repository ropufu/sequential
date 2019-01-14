
#ifndef ROPUFU_SEQUENTIAL_TESTS_MONTE_CARLO_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_MONTE_CARLO_TEST_HPP_INCLUDED

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/process.hpp"
#include "../hypotheses/model.hpp"
#include "../hypotheses/rules.hpp"
#include "../hypotheses/monte_carlo.hpp"
#include "../hypotheses_simulator/init_info.hpp"
#include "../hypotheses_simulator/known_sprts.hpp"
#include "generator.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <system_error> // std::error_code, std::errc
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
                    using asprt_type = hypotheses::adaptive_sprt_a_t<t_process_type>;
                    using bsprt_type = hypotheses::adaptive_sprt_b_t<t_process_type>;
                    using csprt_type = hypotheses::adaptive_sprt_b_t<t_process_type>;
                    using gsprt_type = hypotheses::generalized_sprt_a_t<t_process_type>;
                    using hsprt_type = hypotheses::generalized_sprt_b_t<t_process_type>;
                    
                    using xsprt_type = hypotheses::known_sprts_t<t_process_type>;
                    using tested_type = hypotheses::monte_carlo_t<t_process_type>;
                    using init_type = hypotheses::init_info<value_type>;
                    std::error_code ec {};

                    // Create rules.
                    value_type theta = static_cast<value_type>(0.5);
                    
                    asprt_type x { typename asprt_type::design_type{0, theta, theta, ec} };
                    bsprt_type y { typename bsprt_type::design_type{1, theta, theta, ec} };
                    csprt_type z { typename csprt_type::design_type{2, theta, theta, ec} };
                    gsprt_type v { typename gsprt_type::design_type{3, ec} };
                    hsprt_type w { typename hsprt_type::design_type{4, theta, ec} };

                    std::vector<init_type> init_rules {};
                    init_rules.emplace_back(0);
                    init_rules.emplace_back(1);
                    init_rules.emplace_back(2);
                    init_rules.emplace_back(3);
                    init_rules.emplace_back(4);

                    // List rules.
                    xsprt_type rules {};
                    rules.insert(x.design());
                    rules.insert(y.design());
                    rules.insert(z.design());
                    rules.insert(v.design());
                    rules.insert(w.design());

                    // Output stream.
                    std::ostringstream ss {};

                    // Monte Carlo.
                    tested_type mc(1'000);
                    mc.run(proc, rules,
                        [&] (std::error_code& ecx) {
                            ss << "Simulation start." << std::endl;
                            generator<value_type, 1>::init_rules(rules, init_rules, proc, ecx);
                            if (ecx.value() != 0) ec = ecx;
                        },
                        [&] () {
                            ss << "Simulation stop." << std::endl;
                        }, ec);
                    
                    return (ec.value() == 0);
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
