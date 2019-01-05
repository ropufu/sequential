
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED

#include "two_sprt.hpp"

#include "rules/adaptive_sprt_a.hpp"
#include "rules/adaptive_sprt_b.hpp"
#include "rules/double_sprt.hpp"
#include "rules/generalized_sprt_a.hpp"
#include "rules/generalized_sprt_b.hpp"

#include "rules/adaptive_sprt_a_design.hpp"
#include "rules/adaptive_sprt_b_design.hpp"
#include "rules/double_sprt_design.hpp"
#include "rules/generalized_sprt_a_design.hpp"
#include "rules/generalized_sprt_b_design.hpp"

#include <system_error> // std::error_code, std::errc
#include <variant> // std::variant

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
    bool try_discriminate_rule(const nlohmann::json& j, std::variant<t_design_types...>& result) noexcept
    {
        return detail::rule_design_discriminator<t_design_types...>::discriminate(j, result);
    } // try_discriminate_rule(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED
