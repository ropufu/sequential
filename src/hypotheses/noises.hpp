
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../draft/quiet_json.hpp"

#include "noise_base.hpp"
#include "noises/white_noise.hpp"
#include "noises/auto_regressive_noise.hpp"

#include <variant> // std::variant

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                template <typename t_noise_type, typename... t_more_noise_types>
                struct noise_discriminator
                {
                    template <typename... t_all_noise_types>
                    static bool discriminate(const nlohmann::json& j, std::variant<t_all_noise_types...>& result) noexcept
                    {
                        quiet_json q(j);
                        std::string noise_type_str { };
                        // Parse json entries.
                        if (!q.required(t_noise_type::jstr_noise_type, noise_type_str)) return false;
                        if (noise_type_str == t_noise_type::noise_type_name)
                        {
                            t_noise_type x = j;
                            result = x;
                            return true;
                        }
                        return noise_discriminator<t_more_noise_types...>::discriminate(j, result);
                    };
                }; // struct noise_discriminator

                template <typename t_noise_type>
                struct noise_discriminator<t_noise_type>
                {
                    /** @brief Try to parse the json entry \p j as \p noise. */
                    template <typename... t_all_noise_types>
                    static bool discriminate(const nlohmann::json& j, std::variant<t_all_noise_types...>& result) noexcept
                    {
                        quiet_json q(j);
                        std::string noise_type_str { };
                        // Parse json entries.
                        if (!q.required(t_noise_type::jstr_noise_type, noise_type_str)) return false;
                        if (noise_type_str == t_noise_type::noise_type_name)
                        {
                            t_noise_type x = j;
                            result = x;
                            return true;
                        }
                        return false;
                    };
                }; // struct noise_discriminator<...>
            } // namespace detail

            template <typename... t_noise_types>
            bool try_discriminate_noise(const nlohmann::json& j, std::variant<t_noise_types...>& result) noexcept
            {
                return detail::noise_discriminator<t_noise_types...>::discriminate(j, result);
            }; // discriminate_noise(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_HPP_INCLUDED
