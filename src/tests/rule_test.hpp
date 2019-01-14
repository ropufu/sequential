
#ifndef ROPUFU_SEQUENTIAL_TESTS_RULE_TEST_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_RULE_TEST_HPP_INCLUDED

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/process.hpp"
#include "../hypotheses/model.hpp"
#include "../hypotheses/rules.hpp"
#include "../hypotheses_simulator/known_sprts.hpp"
#include "generator.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <system_error> // std::error_code, std::errc
#include <vector>

namespace ropufu::sequential
{
    namespace hypotheses_test
    {
        struct rule_test
        {
            using type = rule_test;
            using value_type = double;

            template <typename t_process_type, typename t_rule_type>
            static bool test_tic(t_process_type& proc, t_rule_type& rule) noexcept
            {
                /*using signal_type = typename t_process_type::signal_type;*/
                /*using noise_type = typename t_process_type::noise_type;*/
                std::error_code ec {};

                while (rule.is_listening())
                {
                    proc.tic();
                    rule.tic(proc, ec);
                }
                rule.toc(proc, ec);
                proc.reset();

                return true;
            } // test_tic(...)

            template <typename t_process_type>
            static bool test_asprt_with(t_process_type&& proc) noexcept
            {
                using tested_type = hypotheses::adaptive_sprt_a<typename t_process_type::signal_type, typename t_process_type::noise_type>;
                using design_type = typename tested_type::design_type;
                //using xsprt_type = hypotheses::known_sprts_t<t_process_type>;
                std::error_code ec {};

                hypotheses::model<value_type> model {};
                value_type analyzed_mu = 1;
                value_type anticipated_run_length = 1;
                value_type guess_mu = model.mu_relative(static_cast<value_type>(0.5));
                tested_type rule { design_type{0, guess_mu, guess_mu, ec} };

                std::vector<value_type> null_thresholds = { 1, 2, 3 };
                std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                rule.initialize(model, analyzed_mu, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds, ec);

                //xsprt_type x_rule = rule;
                if (!type::test_tic(proc, rule)) return false;
                //if (!type::test_tic(proc, x_rule)) return false;
                return (ec.value() == 0);
            } // test_asprt_with(...)

            template <typename t_process_type>
            static bool test_bsprt_with(t_process_type&& proc) noexcept
            {
                using tested_type = hypotheses::adaptive_sprt_b<typename t_process_type::signal_type, typename t_process_type::noise_type>;
                using design_type = typename tested_type::design_type;
                //using xsprt_type = hypotheses::known_sprts_t<t_process_type>;
                std::error_code ec {};

                hypotheses::model<value_type> model {};
                value_type analyzed_mu = 1;
                value_type anticipated_run_length = 1;
                value_type guess_mu = model.mu_relative(static_cast<value_type>(0.5));
                tested_type rule { design_type{1, guess_mu, guess_mu, ec} };

                std::vector<value_type> null_thresholds = { 1, 2, 3 };
                std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                rule.initialize(model, analyzed_mu, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds, ec);

                //xsprt_type x_rule = rule;
                if (!type::test_tic(proc, rule)) return false;
                //if (!type::test_tic(proc, x_rule)) return false;
                return (ec.value() == 0);
            } // test_bsprt_with(...)

            template <typename t_process_type>
            static bool test_gsprt_with(t_process_type&& proc) noexcept
            {
                using tested_type = hypotheses::generalized_sprt_a<typename t_process_type::signal_type, typename t_process_type::noise_type>;
                using design_type = typename tested_type::design_type;
                //using xsprt_type = hypotheses::known_sprts_t<t_process_type>;
                std::error_code ec {};

                hypotheses::model<value_type> model {};
                value_type analyzed_mu = 1;
                value_type anticipated_run_length = 1;
                //value_type guess_mu = model.mu_relative(static_cast<value_type>(0.5));
                tested_type rule { design_type{2, ec} };

                std::vector<value_type> null_thresholds = { 1, 2, 3 };
                std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                rule.initialize(model, analyzed_mu, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds, ec);

                //xsprt_type x_rule = rule;
                if (!type::test_tic(proc, rule)) return false;
                //if (!type::test_tic(proc, x_rule)) return false;
                return (ec.value() == 0);
            } // test_gsprt_with(...)

            template <typename t_process_type>
            static bool test_hsprt_with(t_process_type&& proc) noexcept
            {
                using tested_type = hypotheses::generalized_sprt_b<typename t_process_type::signal_type, typename t_process_type::noise_type>;
                using design_type = typename tested_type::design_type;
                //using xsprt_type = hypotheses::known_sprts_t<t_process_type>;
                std::error_code ec {};

                hypotheses::model<value_type> model {};
                value_type analyzed_mu = 1;
                value_type anticipated_run_length = 1;
                value_type guess_mu = model.mu_relative(static_cast<value_type>(0.5));
                tested_type rule { design_type{3, guess_mu, ec} };

                std::vector<value_type> null_thresholds = { 1, 2, 3 };
                std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                rule.initialize(model, analyzed_mu, anticipated_run_length, proc.log_likelihood_scale(), null_thresholds, alt_thresholds, ec);

                //xsprt_type x_rule = rule;
                if (!type::test_tic(proc, rule)) return false;
                //if (!type::test_tic(proc, x_rule)) return false;
                return (ec.value() == 0);
            } // test_hsprt_with(...)
            
            static bool test_asprt() noexcept
            {
                if (!type::test_asprt_with(generator<value_type, 5>::constant_white())) return false;
                if (!type::test_asprt_with(generator<value_type, 5>::constant_ar()))    return false;
                if (!type::test_asprt_with(generator<value_type, 5>::transit_white()))  return false;
                if (!type::test_asprt_with(generator<value_type, 5>::transit_ar()))     return false;
                return true;
            }
            
            static bool test_bsprt() noexcept
            {
                if (!type::test_bsprt_with(generator<value_type, 5>::constant_white())) return false;
                if (!type::test_bsprt_with(generator<value_type, 5>::constant_ar()))    return false;
                if (!type::test_bsprt_with(generator<value_type, 5>::transit_white()))  return false;
                if (!type::test_bsprt_with(generator<value_type, 5>::transit_ar()))     return false;
                return true;
            }
            
            static bool test_gsprt() noexcept
            {
                if (!type::test_gsprt_with(generator<value_type, 5>::constant_white())) return false;
                if (!type::test_gsprt_with(generator<value_type, 5>::constant_ar()))    return false;
                if (!type::test_gsprt_with(generator<value_type, 5>::transit_white()))  return false;
                if (!type::test_gsprt_with(generator<value_type, 5>::transit_ar()))     return false;
                return true;
            }
            
            static bool test_hsprt() noexcept
            {
                if (!type::test_hsprt_with(generator<value_type, 5>::constant_white())) return false;
                if (!type::test_hsprt_with(generator<value_type, 5>::constant_ar()))    return false;
                if (!type::test_hsprt_with(generator<value_type, 5>::transit_white()))  return false;
                if (!type::test_hsprt_with(generator<value_type, 5>::transit_ar()))     return false;
                return true;
            }

            static bool print() noexcept
            {
                auto a = generator<value_type, 3>::constant_white();
                auto b = generator<value_type, 0>::constant_ar();
                auto c = generator<value_type, 5>::constant_ar();
                auto d = generator<value_type, 4>::transit_white();
                auto e = generator<value_type, 2>::transit_ar();

                using tested_type_a = hypotheses::adaptive_sprt_a_t<decltype(a)>;
                using tested_type_b = hypotheses::adaptive_sprt_b_t<decltype(b)>;
                using tested_type_c = hypotheses::adaptive_sprt_b_t<decltype(c)>;
                using tested_type_d = hypotheses::generalized_sprt_a_t<decltype(d)>;
                using tested_type_e = hypotheses::generalized_sprt_b_t<decltype(e)>;
                
                //using tested_type_xa = hypotheses::known_sprts_t<decltype(a)>;
                //using tested_type_xb = hypotheses::known_sprts_t<decltype(b)>;
                //using tested_type_xc = hypotheses::known_sprts_t<decltype(c)>;
                //using tested_type_xd = hypotheses::known_sprts_t<decltype(d)>;
                //using tested_type_xe = hypotheses::known_sprts_t<decltype(e)>;
                std::error_code ec {};

                value_type theta = static_cast<value_type>(0.5);
                tested_type_a x { typename tested_type_a::design_type{0, theta, theta, ec} };
                tested_type_b y { typename tested_type_b::design_type{1, theta, theta, ec} };
                tested_type_c z { typename tested_type_c::design_type{2, theta, theta, ec} };
                tested_type_d v { typename tested_type_d::design_type{3, ec} };
                tested_type_e w { typename tested_type_e::design_type{4, theta, ec} };

                // tested_type_xa maybe_x {};
                // tested_type_xb maybe_y {};
                // tested_type_xc maybe_z {};
                // tested_type_xd maybe_v {};
                // tested_type_xe maybe_w {};

                generator<value_type, 1>::init_rule(x, a, ec);
                generator<value_type, 1>::init_rule(y, b, ec);
                generator<value_type, 1>::init_rule(z, c, ec);
                generator<value_type, 1>::init_rule(v, d, ec);
                generator<value_type, 1>::init_rule(w, e, ec);

                // maybe_x = x;
                // maybe_y = y;
                // maybe_z = z;
                // maybe_v = v;
                // maybe_w = w;

                test_ostream(x.design(), json_round_trip(x.design()));
                test_ostream(y.design(), json_round_trip(y.design()));
                test_ostream(z.design(), json_round_trip(z.design()));
                test_ostream(v.design(), json_round_trip(v.design()));
                test_ostream(w.design(), json_round_trip(w.design()));

                // test_ostream(maybe_x, json_round_trip(maybe_x));
                // test_ostream(maybe_y, json_round_trip(maybe_y));
                // test_ostream(maybe_z, json_round_trip(maybe_z));
                // test_ostream(maybe_v, json_round_trip(maybe_v));
                // test_ostream(maybe_w, json_round_trip(maybe_w));
                return (ec.value() == 0);
            } // print(...)
        }; // struct rule_test
    } // namespace hypotheses_test
} // namespace ropufu::sequential

#endif // ROPUFU_SEQUENTIAL_TESTS_RULE_TEST_HPP_INCLUDED
