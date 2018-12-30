
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include "signal_base.hpp"
#include "signals/constant_signal.hpp"
#include "signals/transitionary_signal.hpp"
#include "signals/unit_signal.hpp"

#include <system_error> // std::error_code, std::errc
#include <variant> // std::variant

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        template <typename t_signal_type, typename... t_more_signal_types>
        struct signal_discriminator
        {
            template <typename... t_all_signal_types>
            static bool discriminate(const nlohmann::json& j, std::variant<t_all_signal_types...>& result) noexcept
            {
                std::error_code ec {};
                std::string typename_str {};
                // Parse json entries.
                aftermath::noexcept_json::required(j, t_signal_type::jstr_typename, typename_str, ec);
                if (ec.value() != 0) return false;
                if (typename_str == t_signal_type::typename_string)
                {
                    t_signal_type x { j, ec };
                    if (ec.value() != 0) return false;
                    result = x;
                    return true;
                } // if (...)
                return signal_discriminator<t_more_signal_types...>::discriminate(j, result);
            } // discriminate(...)
        }; // struct signal_discriminator

        template <typename t_signal_type>
        struct signal_discriminator<t_signal_type>
        {
            /** @brief Try to parse the json entry \p j as \p signal. */
            template <typename... t_all_signal_types>
            static bool discriminate(const nlohmann::json& j, std::variant<t_all_signal_types...>& result) noexcept
            {
                std::error_code ec {};
                std::string typename_str {};
                // Parse json entries.
                aftermath::noexcept_json::required(j, t_signal_type::jstr_typename, typename_str, ec);
                if (ec.value() != 0) return false;
                if (typename_str == t_signal_type::typename_string)
                {
                    t_signal_type x { j, ec };
                    if (ec.value() != 0) return false;
                    result = x;
                    return true;
                } // if (...)
                return false;
            } // discriminate(...)
        }; // struct signal_discriminator<...>
    } // namespace detail

    template <typename... t_signal_types>
    bool try_discriminate_signal(const nlohmann::json& j, std::variant<t_signal_types...>& result) noexcept
    {
        return detail::signal_discriminator<t_signal_types...>::discriminate(j, result);
    } // try_discriminate_signal(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED
