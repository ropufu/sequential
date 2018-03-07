
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "json.hpp"

#include "signal_base.hpp"
#include "signals/constant_signal.hpp"
#include "signals/transitionary_signal.hpp"
#include "signals/unit_signal.hpp"

#include <variant> // std::variant

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                template <typename t_signal_type, typename... t_more_signal_types>
                struct signal_discriminator
                {
                    template <typename... t_all_signal_types>
                    static bool discriminate(const nlohmann::json& j, std::variant<t_all_signal_types...>& result) noexcept
                    {
                        std::string signal_type_str { };
                        // Parse json entries.
                        if (!quiet_json::required(j, t_signal_type::jstr_signal_type, signal_type_str)) return false;
                        if (signal_type_str == t_signal_type::signal_type_name)
                        {
                            t_signal_type x = j;
                            result = x;
                            return true;
                        }
                        return signal_discriminator<t_more_signal_types...>::discriminate(j, result);
                    };
                }; // struct signal_discriminator

                template <typename t_signal_type>
                struct signal_discriminator<t_signal_type>
                {
                    /** @brief Try to parse the json entry \p j as \p signal. */
                    template <typename... t_all_signal_types>
                    static bool discriminate(const nlohmann::json& j, std::variant<t_all_signal_types...>& result) noexcept
                    {
                        std::string signal_type_str { };
                        // Parse json entries.
                        if (!quiet_json::required(j, t_signal_type::jstr_signal_type, signal_type_str)) return false;
                        if (signal_type_str == t_signal_type::signal_type_name)
                        {
                            t_signal_type x = j;
                            result = x;
                            return true;
                        }
                        return false;
                    };
                }; // struct signal_discriminator<...>
            } // namespace detail

            template <typename... t_signal_types>
            bool try_discriminate_signal(const nlohmann::json& j, std::variant<t_signal_types...>& result) noexcept
            {
                return detail::signal_discriminator<t_signal_types...>::discriminate(j, result);
            }; // discriminate_signal(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_HPP_INCLUDED
