
#ifndef ROPUFU_SEQUENTIAL_TESTS_GENERATOR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_TESTS_GENERATOR_HPP_INCLUDED

#include <nlohmann/json.hpp>

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/process.hpp"
#include "../hypotheses/rules.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream> // std::ostringstream
#include <string>  // std::string
#include <system_error> // std::error_code, std::errc
#include <utility> // std::forward
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses_test
        {
            template <typename t_type>
            t_type json_round_trip(const t_type& x) noexcept
            {
                nlohmann::json j = x;
                t_type y = j;
                return y;
            } // json_round_trip(...)

            template <typename t_type>
            static bool test_ostream(const t_type& x, const t_type& y) noexcept
            {
                std::ostringstream ssx {};
                std::ostringstream ssy {};

                ssx << x;
                ssy << y;
                return ssx.str() == ssy.str();
            }

            template <typename t_value_type, std::size_t t_size>
            struct generator
            {
                using type = generator<t_value_type, t_size>;
                using value_type = t_value_type;

            private:
                static std::size_t s_count;

            public:
                static std::array<value_type, t_size> make_lin_array(value_type seed = 1) noexcept
                {
                    std::array<value_type, t_size> result {};
                    for (std::size_t i = 0; i < t_size; ++i) result[i] = (i + 1) * seed / 100;
                    return result;
                } // make_array(...)

                static std::array<value_type, t_size> make_quad_array(value_type seed = 1) noexcept
                {
                    std::array<value_type, t_size> result {};
                    for (std::size_t i = 0; i < t_size; ++i)
                    {
                        value_type a = (i + 1) * (i + 2) * seed / 500;
                        value_type b = (i + 3) * seed / 100;
                        result[i] = a - b;
                    }
                    return result;
                } // make_array(...)

                static auto constant_white(value_type mu = 1, value_type level = 1, value_type sigma = 1) noexcept -> hypotheses::process<
                    hypotheses::constant_signal<value_type>,
                    hypotheses::white_noise<value_type>>
                {
                    using signal_type = hypotheses::constant_signal<value_type>;
                    using noise_type = hypotheses::white_noise<value_type>;
                    using process_type = hypotheses::process<signal_type, noise_type>;
                    std::error_code ec {};

                    signal_type s(level, ec);
                    noise_type n(sigma, ec);
                    return process_type(s, n, mu);
                } // constant_white(...)

                static auto constant_ar(value_type mu = 1, value_type level = 1, value_type sigma = 1) noexcept -> hypotheses::process<
                    hypotheses::constant_signal<value_type>,
                    hypotheses::auto_regressive_noise<value_type, t_size>>
                {
                    using signal_type = hypotheses::constant_signal<value_type>;
                    using noise_type = hypotheses::auto_regressive_noise<value_type, t_size>;
                    using process_type = hypotheses::process<signal_type, noise_type>;
                    std::error_code ec {};

                    signal_type s(level, ec);
                    noise_type n(sigma, type::make_quad_array(), ec);
                    return process_type(s, n, mu);
                } // constant_ar(...)

                static auto transit_white(value_type mu = 1, value_type level = 1, value_type sigma = 1) noexcept -> hypotheses::process<
                    hypotheses::transitionary_signal<value_type, t_size>,
                    hypotheses::white_noise<value_type>>
                {
                    using signal_type = hypotheses::transitionary_signal<value_type, t_size>;
                    using noise_type = hypotheses::white_noise<value_type>;
                    using process_type = hypotheses::process<signal_type, noise_type>;
                    std::error_code ec {};

                    signal_type s(level, type::make_quad_array(), ec);
                    noise_type n(sigma, ec);
                    return process_type(s, n, mu);
                } // transit_white(...)

                static auto transit_ar(value_type mu = 1, value_type level = 1, value_type sigma = 1) noexcept -> hypotheses::process<
                    hypotheses::transitionary_signal<value_type, t_size>,
                    hypotheses::auto_regressive_noise<value_type, t_size>>
                {
                    using signal_type = hypotheses::transitionary_signal<value_type, t_size>;
                    using noise_type = hypotheses::auto_regressive_noise<value_type, t_size>;
                    using process_type = hypotheses::process<signal_type, noise_type>;
                    std::error_code ec {};

                    signal_type s(level, type::make_lin_array(), ec);
                    noise_type n(sigma, type::make_quad_array(), ec);
                    return process_type(s, n, mu);
                } // transit_ar(...)

                template <typename t_rule_type, typename t_process_type>
                static void reset_rule(t_rule_type& rule, t_process_type&& proc, std::error_code& ec)
                {
                    hypotheses::model<value_type> model {};
                    value_type analyzed_mu = 1;
                    value_type anticipated_run_length = 1;

                    std::vector<value_type> null_thresholds = { 1, 2, 3 };
                    std::vector<value_type> alt_thresholds = { 1, 2, 3 };
                    rule.initialize(model, analyzed_mu, anticipated_run_length, proc, null_thresholds, alt_thresholds, ec);
                } // reset_rule(...)
            }; // struct generator

            // ~~ Definitions ~~
            template <typename t_value_type, std::size_t t_size>
            std::size_t generator<t_value_type, t_size>::s_count = 0;
        } // namespace hypotheses_test
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_TESTS_GENERATOR_HPP_INCLUDED
