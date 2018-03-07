
#ifndef ROPUFU_SEQUENTIAL_TESTS_RULE_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_RULE_TEST_HPP_INCLUDED

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/process.hpp"
#include "../hypotheses/model.hpp"
#include "../hypotheses/rules.hpp"
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
            struct rule_test
            {
                using type = rule_test;
                using value_type = double;

                template <typename t_process_type, typename t_rule_type>
                static bool test_tic(t_process_type& proc, t_rule_type& rule)
                {
                    using signal_type = typename t_process_type::signal_type;
                    using noise_type = typename t_process_type::noise_type;

                    while (rule.is_listening())
                    {
                        proc.tic();
                        rule.tic(proc);
                    }
                    rule.toc(proc);
                    proc.reset();

                    return true;
                } // test_tic(...)

                template <typename t_process_type>
                static bool test_asprt(t_process_type&& proc)
                {
                    using tested_type = hypotheses::adaptive_sprt<typename t_process_type::signal_type, typename t_process_type::noise_type, true>;
                    using xsprt_type = hypotheses::xsprt_t<t_process_type, true>;

                    hypotheses::model<value_type> model { };
                    value_type analyzed_mu = 1;
                    value_type anticipated_run_length = 1;
                    value_type guess_mu = model.mu_relative(static_cast<value_type>(0.5));
                    tested_type rule(0, guess_mu, guess_mu);

                    std::vector<value_type> null_thresholds = { 1, 2, 3 };
                    std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                    rule.initialize(model, analyzed_mu, anticipated_run_length, proc, null_thresholds, alt_thresholds);

                    xsprt_type x_rule = rule;
                    if (!type::test_tic(proc, rule)) return false;
                    if (!type::test_tic(proc, x_rule)) return false;
                    return true;
                } // test_asprt(...)

                template <typename t_process_type>
                static bool test_bsprt(t_process_type&& proc)
                {
                    using tested_type = hypotheses::adaptive_sprt_star<typename t_process_type::signal_type, typename t_process_type::noise_type, true>;
                    using xsprt_type = hypotheses::xsprt_t<t_process_type, true>;

                    hypotheses::model<value_type> model { };
                    value_type analyzed_mu = 1;
                    value_type anticipated_run_length = 1;
                    value_type guess_mu = model.mu_relative(static_cast<value_type>(0.5));
                    tested_type rule(1, guess_mu, guess_mu);

                    std::vector<value_type> null_thresholds = { 1, 2, 3 };
                    std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                    rule.initialize(model, analyzed_mu, anticipated_run_length, proc, null_thresholds, alt_thresholds);

                    xsprt_type x_rule = rule;
                    if (!type::test_tic(proc, rule)) return false;
                    if (!type::test_tic(proc, x_rule)) return false;
                    return true;
                } // test_bsprt(...)

                template <typename t_process_type>
                static bool test_gsprt(t_process_type&& proc)
                {
                    using tested_type = hypotheses::generalized_sprt<typename t_process_type::signal_type, typename t_process_type::noise_type, true>;
                    using xsprt_type = hypotheses::xsprt_t<t_process_type, true>;

                    hypotheses::model<value_type> model { };
                    value_type analyzed_mu = 1;
                    value_type anticipated_run_length = 1;
                    //value_type guess_mu = model.mu_relative(static_cast<value_type>(0.5));
                    tested_type rule(2);

                    std::vector<value_type> null_thresholds = { 1, 2, 3 };
                    std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                    rule.initialize(model, analyzed_mu, anticipated_run_length, proc, null_thresholds, alt_thresholds);

                    xsprt_type x_rule = rule;
                    if (!type::test_tic(proc, rule)) return false;
                    if (!type::test_tic(proc, x_rule)) return false;
                    return true;
                } // test_gsprt(...)

                template <typename t_process_type>
                static bool test_hsprt(t_process_type&& proc)
                {
                    using tested_type = hypotheses::generalized_sprt_star<typename t_process_type::signal_type, typename t_process_type::noise_type, true>;
                    using xsprt_type = hypotheses::xsprt_t<t_process_type, true>;

                    hypotheses::model<value_type> model { };
                    value_type analyzed_mu = 1;
                    value_type anticipated_run_length = 1;
                    value_type guess_mu = model.mu_relative(static_cast<value_type>(0.5));
                    tested_type rule(3, guess_mu);

                    std::vector<value_type> null_thresholds = { 1, 2, 3 };
                    std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                    rule.initialize(model, analyzed_mu, anticipated_run_length, proc, null_thresholds, alt_thresholds);

                    xsprt_type x_rule = rule;
                    if (!type::test_tic(proc, rule)) return false;
                    if (!type::test_tic(proc, x_rule)) return false;
                    return true;
                } // test_hsprt(...)
                
                static bool test_asprt()
                {
                    if (!type::test_asprt(generator<value_type, 5>::constant_white())) return false;
                    if (!type::test_asprt(generator<value_type, 5>::constant_ar()))    return false;
                    if (!type::test_asprt(generator<value_type, 5>::transit_white()))  return false;
                    if (!type::test_asprt(generator<value_type, 5>::transit_ar()))     return false;
                    return true;
                }
                
                static bool test_bsprt()
                {
                    if (!type::test_bsprt(generator<value_type, 5>::constant_white())) return false;
                    if (!type::test_bsprt(generator<value_type, 5>::constant_ar()))    return false;
                    if (!type::test_bsprt(generator<value_type, 5>::transit_white()))  return false;
                    if (!type::test_bsprt(generator<value_type, 5>::transit_ar()))     return false;
                    return true;
                }
                
                static bool test_gsprt()
                {
                    if (!type::test_gsprt(generator<value_type, 5>::constant_white())) return false;
                    if (!type::test_gsprt(generator<value_type, 5>::constant_ar()))    return false;
                    if (!type::test_gsprt(generator<value_type, 5>::transit_white()))  return false;
                    if (!type::test_gsprt(generator<value_type, 5>::transit_ar()))     return false;
                    return true;
                }
                
                static bool test_hsprt()
                {
                    if (!type::test_hsprt(generator<value_type, 5>::constant_white())) return false;
                    if (!type::test_hsprt(generator<value_type, 5>::constant_ar()))    return false;
                    if (!type::test_hsprt(generator<value_type, 5>::transit_white()))  return false;
                    if (!type::test_hsprt(generator<value_type, 5>::transit_ar()))     return false;
                    return true;
                }

                static bool print() noexcept
                {
                    auto a = generator<value_type, 3>::constant_white();
                    auto b = generator<value_type, 0>::constant_ar();
                    auto c = generator<value_type, 5>::constant_ar();
                    auto d = generator<value_type, 4>::transit_white();
                    auto e = generator<value_type, 2>::transit_ar();

                    using tested_type_a = hypotheses::adaptive_sprt_t<decltype(a)>;
                    using tested_type_b = hypotheses::adaptive_sprt_star_t<decltype(b)>;
                    using tested_type_c = hypotheses::adaptive_sprt_star_t<decltype(c)>;
                    using tested_type_d = hypotheses::generalized_sprt_t<decltype(d)>;
                    using tested_type_e = hypotheses::generalized_sprt_star_t<decltype(e)>;
                    
                    using tested_type_xa = hypotheses::xsprt_t<decltype(a)>;
                    using tested_type_xb = hypotheses::xsprt_t<decltype(b)>;
                    using tested_type_xc = hypotheses::xsprt_t<decltype(c)>;
                    using tested_type_xd = hypotheses::xsprt_t<decltype(d)>;
                    using tested_type_xe = hypotheses::xsprt_t<decltype(e)>;

                    value_type theta = static_cast<value_type>(0.5);
                    tested_type_a x { 0, theta, theta };
                    tested_type_b y { 1, theta, theta };
                    tested_type_c z { 2, theta, theta };
                    tested_type_d v { 3 };
                    tested_type_e w { 4, theta };

                    tested_type_xa maybe_x { };
                    tested_type_xb maybe_y { };
                    tested_type_xc maybe_z { };
                    tested_type_xd maybe_v { };
                    tested_type_xe maybe_w { };

                    generator<value_type, 1>::reset_rule(x, a);
                    generator<value_type, 1>::reset_rule(y, b);
                    generator<value_type, 1>::reset_rule(z, c);
                    generator<value_type, 1>::reset_rule(v, d);
                    generator<value_type, 1>::reset_rule(w, e);

                    maybe_x = x;
                    maybe_y = y;
                    maybe_z = z;
                    maybe_v = v;
                    maybe_w = w;

                    test_ostream(x, json_round_trip(x));
                    test_ostream(y, json_round_trip(y));
                    test_ostream(z, json_round_trip(z));
                    test_ostream(v, json_round_trip(v));
                    test_ostream(w, json_round_trip(w));

                    test_ostream(maybe_x, json_round_trip(maybe_x));
                    test_ostream(maybe_y, json_round_trip(maybe_y));
                    test_ostream(maybe_z, json_round_trip(maybe_z));
                    test_ostream(maybe_v, json_round_trip(maybe_v));
                    test_ostream(maybe_w, json_round_trip(maybe_w));
                    return true;
                } // print(...)
            }; // struct rule_test
        } // namespace hypotheses_test
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_TESTS_RULE_TEST_HPP_INCLUDED
