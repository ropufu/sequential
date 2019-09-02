
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include "noises/auto_regressive_noise.hpp"
#include "noises/white_noise.hpp"

#include <system_error> // std::error_code, std::errc
#include <variant> // std::variant

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        template <typename t_noise_type, typename... t_more_noise_types>
        struct noise_discriminator
        {
            template <typename... t_all_noise_types>
            static bool discriminate(const nlohmann::json& j, std::variant<t_all_noise_types...>& result) noexcept
            {
                std::error_code ec {};
                std::string typename_str {};
                // Parse json entries.
                aftermath::noexcept_json::required(j, t_noise_type::jstr_typename, typename_str, ec);
                if (ec.value() != 0) return false;
                if (typename_str == t_noise_type::typename_string)
                {
                    t_noise_type x { j, ec };
                    if (ec.value() != 0) return false;
                    result = x;
                    return true;
                } // if (...)
                return noise_discriminator<t_more_noise_types...>::discriminate(j, result);
            } // discriminate(...)
        }; // struct noise_discriminator

        template <typename t_noise_type>
        struct noise_discriminator<t_noise_type>
        {
            /** @brief Try to parse the json entry \p j as \p noise. */
            template <typename... t_all_noise_types>
            static bool discriminate(const nlohmann::json& j, std::variant<t_all_noise_types...>& result) noexcept
            {
                std::error_code ec {};
                std::string typename_str {};
                // Parse json entries.
                aftermath::noexcept_json::required(j, t_noise_type::jstr_typename, typename_str, ec);
                if (ec.value() != 0) return false;
                if (typename_str == t_noise_type::typename_string)
                {
                    t_noise_type x { j, ec };
                    if (ec.value() != 0) return false;
                    result = x;
                    return true;
                } // if (...)
                return false;
            } // discriminate(...)
        }; // struct noise_discriminator<...>
    } // namespace detail

    template <typename... t_noise_types>
    bool try_discriminate_noise(const nlohmann::json& j, std::variant<t_noise_types...>& result) noexcept
    {
        return detail::noise_discriminator<t_noise_types...>::discriminate(j, result);
    } // try_discriminate_noise(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_HPP_INCLUDED
