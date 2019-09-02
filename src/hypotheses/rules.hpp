
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include "rules/adaptive_sprt_design.hpp"
#include "rules/double_sprt_design.hpp"
#include "rules/generalized_sprt_design.hpp"

#include "rules/two_sprt.hpp"
#include "rules/adaptive_sprt.hpp"
#include "rules/double_sprt.hpp"
#include "rules/generalized_sprt.hpp"

#include <stdexcept>    // std::runtime_error
#include <system_error> // std::error_code, std::errc
#include <variant> // std::variant, std::visit

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        template <typename t_design_type, typename... t_more_design_types>
        struct rule_design_discriminator
        {
            template <typename... t_all_design_types>
            static bool discriminate(const nlohmann::json& j, std::variant<t_all_design_types...>& result) noexcept
            {
                std::error_code ec {};
                std::string typename_str {};
                // Parse json entries.
                aftermath::noexcept_json::required(j, t_design_type::jstr_typename, typename_str, ec);
                if (ec.value() != 0) return false;
                if (typename_str == t_design_type::typename_string)
                {
                    t_design_type x { j, ec };
                    if (ec.value() != 0) return false;
                    result = x;
                    return true;
                } // if (...)
                return rule_design_discriminator<t_more_design_types...>::discriminate(j, result);
            } // discriminate(...)
        }; // struct rule_design_discriminator

        template <typename t_design_type>
        struct rule_design_discriminator<t_design_type>
        {
            /** @brief Try to parse the json entry \p j as \p noise. */
            template <typename... t_all_design_types>
            static bool discriminate(const nlohmann::json& j, std::variant<t_all_design_types...>& result) noexcept
            {
                std::error_code ec {};
                std::string typename_str {};
                // Parse json entries.
                aftermath::noexcept_json::required(j, t_design_type::jstr_typename, typename_str, ec);
                if (ec.value() != 0) return false;
                if (typename_str == t_design_type::typename_string)
                {
                    t_design_type x { j, ec };
                    if (ec.value() != 0) return false;
                    result = x;
                    return true;
                } // if (...)
                return false;
            } // discriminate(...)
        }; // struct rule_design_discriminator<...>
    } // namespace detail

    template <typename... t_design_types>
    bool try_discriminate_rule_design(const nlohmann::json& j, std::variant<t_design_types...>& result) noexcept
    {
        return detail::rule_design_discriminator<t_design_types...>::discriminate(j, result);
    } // try_discriminate_rule_design(...)

    template <typename t_value_type>
    struct rule_design_variant;
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const rule_design_variant<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, rule_design_variant<t_value_type>& x);

    template <typename t_value_type>
    struct rule_design_variant : public std::variant<
        adaptive_sprt_design<t_value_type>,
        generalized_sprt_design<t_value_type>,
        double_sprt_design<t_value_type>>
    {
        using type = rule_design_variant<t_value_type>;
        using value_type = t_value_type;
        using base_type = std::variant<
            adaptive_sprt_design<t_value_type>,
            generalized_sprt_design<t_value_type>,
            double_sprt_design<t_value_type>>;

        using base_type::variant;

        rule_design_variant(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            if (!try_discriminate_rule_design(j, *this))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
        } // rule_design_variant(...)

        std::size_t id() const noexcept
        {
            return std::visit([] (auto&& arg) { return arg.id(); }, static_cast<const base_type&>(*this));
        } // id(...)
    }; // struct rule_design_variant

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const rule_design_variant<t_value_type>& x) noexcept
    {
        std::visit([&j] (auto&& arg) { j = arg; }, x);
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, rule_design_variant<t_value_type>& x)
    {
        using type = rule_design_variant<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <rule_design_variant> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED
