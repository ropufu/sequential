#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_XSPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_XSPRT_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/algebra.hpp>  // aftermath::algebra::matrix
#include <ropufu/on_error.hpp> // aftermath::detail::on_error

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
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc
#include <utility> // std::forward
#include <variant> // std::variant, std::visit, std::monostate
#include <vector>  // std::vector

namespace ropufu::sequential::hypotheses
{
    /** Represents a collection of SPRT-based rule observers. */
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check = true>
    struct xsprt;
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void to_json(nlohmann::json& j, const xsprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept;
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void from_json(const nlohmann::json& j, xsprt<t_signal_type, t_noise_type, t_sync_check>& x);

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
        static constexpr char jstr_typename[] = "type";

    private:
        variant_type m_rule = {};

    protected:
        /** @brief Auxiliary function to be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            std::visit([] (auto&& arg) { arg.reset(); }, this->m_rule);
        } // on_reset(...)

        /** @brief Auxiliary function to be executed right after the \c tic() call. */
        void on_tic(const process_type& proc, std::error_code& ec) noexcept
        {
            std::visit([&] (auto&& arg) { arg.tic(proc, ec); }, this->m_rule);
        } // on_tic(...)
        
        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& proc, std::error_code& ec) noexcept
        {
            std::visit([&] (auto&& arg) { arg.toc(proc, ec); }, this->m_rule);
        } // on_toc(...)

    public:
        xsprt() noexcept { }

        template <typename t_other_signal_type, typename t_other_noise_type, bool t_other_sync_check>
        /*implicit*/ xsprt(const xsprt<t_other_signal_type, t_other_noise_type, t_other_sync_check>& similar) noexcept
        {
            std::visit([&] (auto&& arg) { this->m_rule = arg; }, similar.underlying());
        } // xsprt(...)

        /*implicit*/ xsprt(invalid_type&& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(type_0&& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(type_1&& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(type_2&& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(type_3&& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(type_4&& rule) noexcept : m_rule(rule) { }

        /*implicit*/ xsprt(const invalid_type& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(const type_0& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(const type_1& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(const type_2& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(const type_3& rule) noexcept : m_rule(rule) { }
        /*implicit*/ xsprt(const type_4& rule) noexcept : m_rule(rule) { }
        
        xsprt(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            invalid_type bad {};
            this->m_rule = bad;

            invalid_type unrecognized { j, ec };
            if (ec.value() != 0) return;
            this->m_rule = unrecognized;

            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (ec.value() != 0) return;

            if (typename_str == type_0::typename_string) this->m_rule = type_0(j, ec);
            if (typename_str == type_1::typename_string) this->m_rule = type_1(j, ec);
            if (typename_str == type_2::typename_string) this->m_rule = type_2(j, ec);
            if (typename_str == type_3::typename_string) this->m_rule = type_3(j, ec);
            if (typename_str == type_4::typename_string) this->m_rule = type_4(j, ec);

            if (ec.value() != 0) this->m_rule = unrecognized;
        } // xsprt(...)

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
            const std::vector<value_type>& alt_thresholds,
            std::error_code& ec) noexcept
        {
            std::visit([&] (auto&& arg) { arg.initialize(model, analyzed_mu, anticipated_run_length, proc, null_thresholds, alt_thresholds, ec); }, this->m_rule);
        } // set_thresholds(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct xsprt

    // ~~ Json name definitions ~~
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char xsprt<t_signal_type, t_noise_type, t_sync_check>::jstr_typename[];

    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void to_json(nlohmann::json& j, const xsprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
    {
        /*using type = xsprt<t_signal_type, t_noise_type, t_sync_check>;*/
        x.to_json(j);
    } // to_json(...)

    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void from_json(const nlohmann::json& j, xsprt<t_signal_type, t_noise_type, t_sync_check>& x)
    {
        using type = xsprt<t_signal_type, t_noise_type, t_sync_check>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_XSPRT_HPP_INCLUDED