#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_XSPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_XSPRT_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../../draft/quiet_json.hpp"

#include <aftermath/algebra.hpp> // aftermath::algebra::matrix
#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include "invalid_sprt.hpp"
#include "adaptive_sprt.hpp"
#include "adaptive_sprt_star.hpp"
#include "double_sprt.hpp"
#include "generalized_sprt.hpp"
#include "generalized_sprt_star.hpp"

#include "../core.hpp"
#include "../likelihood.hpp"
#include "../model.hpp"
#include "../moment_statistic.hpp"
#include "../observer.hpp"
#include "../process.hpp"

#include <cstddef> // std::size_t
#include <iostream> // std::ostream
#include <utility> // std::forward
#include <variant> // std::variant, std::visit, std::monostate
#include <vector>  // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** Represents a collection of SPRT-based rule observers. */
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check = true>
            struct xsprt;

            template <typename t_process_type, bool t_sync_check = true>
            using xsprt_t = xsprt<typename t_process_type::signal_type, typename t_process_type::noise_type, t_sync_check>;

            /** Represents a collection of SPRT-based rule observers. */
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            struct xsprt : public observer<xsprt<t_signal_type, t_noise_type, t_sync_check>, t_signal_type, t_noise_type, t_sync_check>
            {
                using type = xsprt<t_signal_type, t_noise_type, t_sync_check>;
                using base_type = observer<type, t_signal_type, t_noise_type, t_sync_check>;
                friend base_type;

                using signal_type = typename base_type::signal_type;
                using noise_type = typename base_type::noise_type;
                using process_type = typename base_type::process_type;
                using value_type = typename base_type::value_type;
                using model_type = hypotheses::model<value_type>;
                using likelihood_type = hypotheses::likelihood<t_signal_type, t_noise_type, t_sync_check>;

                // ~~ Supported 2-SPRT types ~~
                using invalid_type = invalid_sprt<t_signal_type, t_noise_type, t_sync_check>;
                using type_0 = adaptive_sprt<t_signal_type, t_noise_type, t_sync_check>;
                using type_1 = adaptive_sprt_star<t_signal_type, t_noise_type, t_sync_check>;
                using type_2 = double_sprt<t_signal_type, t_noise_type, t_sync_check>;
                using type_3 = generalized_sprt<t_signal_type, t_noise_type, t_sync_check>;
                using type_4 = generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>;

                using variant_type = std::variant<invalid_type, type_0, type_1, type_2, type_3, type_4>;

                template <typename t_data_type>
                using matrix_t = aftermath::algebra::matrix<t_data_type>;
                using statistic_type = moment_statistic<matrix_t<value_type>>;

                // ~~ Json names ~~
                static constexpr char jstr_sprt_type[] = "type";

            private:
                variant_type m_rule = { };

            protected:
                /** @brief Auxiliary function to be executed right before the \c reset() call. */
                void on_reset() noexcept
                {
                    std::visit([] (auto&& arg) { arg.reset(); }, this->m_rule);
                } // on_reset(...)

                /** @brief Auxiliary function to be executed right after the \c tic() call. */
                void on_tic(const process_type& proc) noexcept
                {
                    std::visit([&] (auto&& arg) { arg.tic(proc); }, this->m_rule);
                } // on_tic(...)
                
                /** @brief Auxiliary function to be executed right before the \c toc() call. */
                void on_toc(const process_type& proc) noexcept
                {
                    std::visit([&] (auto&& arg) { arg.toc(proc); }, this->m_rule);
                } // on_toc(...)

            public:
                xsprt() noexcept { }

                template <typename t_other_signal_type, typename t_other_noise_type, bool t_other_sync_check>
                /**implicit*/ xsprt(const xsprt<t_other_signal_type, t_other_noise_type, t_other_sync_check>& similar) noexcept
                {
                    std::visit([&] (auto&& arg) { this->m_rule = arg; }, similar.underlying());
                } // xsprt(...)

                /**implicit*/ xsprt(invalid_type&& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(type_0&& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(type_1&& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(type_2&& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(type_3&& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(type_4&& rule) noexcept : m_rule(rule) { }

                /**implicit*/ xsprt(const invalid_type& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(const type_0& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(const type_1& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(const type_2& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(const type_3& rule) noexcept : m_rule(rule) { }
                /**implicit*/ xsprt(const type_4& rule) noexcept : m_rule(rule) { }

                /** @brief Indicates if this \c xsprt represents a valid \c two_sprt. */
                bool good() const noexcept { return this->m_rule.index() != 0; }

                const variant_type& underlying() const noexcept { return this->m_rule; }

                void to_json(nlohmann::json& j) const noexcept
                {
                    std::visit([&] (auto&& arg) { j = arg; }, this->m_rule);
                } // to_json(...)

                
                // ~~ Forwarded from \c two_sprt ~~
                std::size_t id() const noexcept { return std::visit([] (auto&& arg) { return arg.id(); }, this->m_rule); }

                const likelihood_type& likelihood() const noexcept { return std::visit([] (auto&& arg) -> const likelihood_type& { return arg.likelihood(); }, this->m_rule); }

                /** @brief Indicates whether the observer is still active. */
                bool is_listening() const noexcept { return std::visit([] (auto&& arg) { return arg.is_listening(); }, this->m_rule); } // is_listening(...)
                bool has_stopped() const noexcept { return std::visit([] (auto&& arg) { return arg.has_stopped(); }, this->m_rule); }
                
                /** Signal "strength" conrresponding to what measure we want to analyze. */
                value_type analyzed_mu() const noexcept { return std::visit([] (auto&& arg) { return arg.analyzed_mu(); }, this->m_rule); }

                /** An auxiliary quantity to improve accuracy of statistics. */
                value_type anticipated_run_length() const noexcept { return std::visit([] (auto&& arg) { return arg.anticipated_run_length(); }, this->m_rule); }

                /** Track erroneous decisions made by the stopping time. */
                const statistic_type& errors() const noexcept { return std::visit([] (auto&& arg) -> const statistic_type& { return arg.errors(); }, this->m_rule); }

                /** Track run lengths of the stopping time. */
                const statistic_type& run_lengths() const noexcept { return std::visit([] (auto&& arg) -> const statistic_type& { return arg.run_lengths(); }, this->m_rule); }

                const std::vector<value_type>& unscaled_null_thresholds() const noexcept { return std::visit([] (auto&& arg) -> const std::vector<value_type>& { return arg.unscaled_null_thresholds(); }, this->m_rule); }
                const std::vector<value_type>& unscaled_alt_thresholds() const noexcept { return std::visit([] (auto&& arg) -> const std::vector<value_type>& { return arg.unscaled_alt_thresholds(); }, this->m_rule); }

                std::string to_path_string(std::size_t decimal_places = 3) const noexcept { return std::visit([decimal_places] (auto&& arg) { return arg.to_path_string(decimal_places); }, this->m_rule); }

                bool do_decide_null(value_type threshold) const noexcept { return std::visit([threshold] (auto&& arg) { return arg.do_decide_null(threshold); }, this->m_rule); }
                bool do_decide_alt(value_type threshold) const noexcept { return std::visit([threshold] (auto&& arg) { return arg.do_decide_alt(threshold); }, this->m_rule); }

                /** @remark Thresholds have to be of the same size; they are independently(!) sorted and then paired up. */
                void initialize(const model_type& model, value_type analyzed_mu, value_type anticipated_run_length,
                    const process_type& proc,
                    const std::vector<value_type>& null_thresholds,
                    const std::vector<value_type>& alt_thresholds) noexcept
                {
                    std::visit([&] (auto&& arg) { arg.initialize(model, analyzed_mu, anticipated_run_length, proc, null_thresholds, alt_thresholds); }, this->m_rule);
                } // set_thresholds(...)

                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct xsprt

            // ~~ Json name definitions ~~
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char xsprt<t_signal_type, t_noise_type, t_sync_check>::jstr_sprt_type[];

            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            void to_json(nlohmann::json& j, const xsprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
            {
                /**using type = xsprt<t_signal_type, t_noise_type, t_sync_check>;*/
                x.to_json(j);
            } // to_json(...)
        
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            void from_json(const nlohmann::json& j, xsprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
            {
                quiet_json q(j);

                using type = xsprt<t_signal_type, t_noise_type, t_sync_check>;
                using invalid_type = typename type::invalid_type;
                using type_0 = typename type::type_0;
                using type_1 = typename type::type_1;
                using type_2 = typename type::type_2;
                using type_3 = typename type::type_3;
                using type_4 = typename type::type_4;

                std::string sprt_type_str { };

                // Parse json entries.
                if (!q.required(type::jstr_sprt_type, sprt_type_str)) return;

                if (sprt_type_str == type_0::sprt_type_name) { type_0 y = j; x = y; return; }
                if (sprt_type_str == type_1::sprt_type_name) { type_1 y = j; x = y; return; }
                if (sprt_type_str == type_2::sprt_type_name) { type_2 y = j; x = y; return; }
                if (sprt_type_str == type_3::sprt_type_name) { type_3 y = j; x = y; return; }
                if (sprt_type_str == type_4::sprt_type_name) { type_4 y = j; x = y; return; }

                invalid_type invalid = j;
                x = invalid;
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_XSPRT_HPP_INCLUDED