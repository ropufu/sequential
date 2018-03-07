
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_UNIT_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_UNIT_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../json.hpp"

#include "../signal_base.hpp"

#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <string>   // std::string

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** Represents a constant signal. */
            template <typename t_value_type>
            struct unit_signal : public signal_base<unit_signal<t_value_type>, t_value_type>
            {
                using type = unit_signal<t_value_type>;
                using base_type = signal_base<type, t_value_type>;
                friend base_type;

                using value_type = typename base_type::value_type;
                using signal_base_type = typename base_type::signal_base_type;
                static constexpr t_value_type unit = 1;
                static constexpr char signal_type_name[] = "unit";

                // ~~ Json names ~~
                static constexpr char jstr_signal_type[] = "type";

                /** Constant signal. */
                unit_signal() noexcept { }

                /** @brief Signal value at an arbitrary time. */
                value_type at(std::size_t /**time_index*/) const noexcept { return type::unit; }

                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                }
            }; // struct unit_signal

            // ~~ Some definitions ~~
            template <typename t_value_type> constexpr char unit_signal<t_value_type>::signal_type_name[];
            // ~~ Json name definitions ~~
            template <typename t_value_type> constexpr char unit_signal<t_value_type>::jstr_signal_type[];
            
            template <typename t_value_type>
            void to_json(nlohmann::json& j, const unit_signal<t_value_type>& /**x*/) noexcept
            {
                using type = unit_signal<t_value_type>;
                std::string signal_type_str = type::signal_type_name;

                j = nlohmann::json{
                    {type::jstr_signal_type, signal_type_str}
                };
            } // to_json(...)
        
            template <typename t_value_type>
            void from_json(const nlohmann::json& j, unit_signal<t_value_type>& /**x*/) noexcept
            {
                quiet_json q(__FUNCTION__, __LINE__);
                using type = unit_signal<t_value_type>;
                std::string signal_type_str = type::signal_type_name;

                // Parse json entries.
                if (!quiet_json::required(j, type::jstr_signal_type, signal_type_str)) return;

                q.validate();
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_UNIT_SIGNAL_HPP_INCLUDED
