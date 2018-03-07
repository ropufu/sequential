
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TRANSITIONARY_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TRANSITIONARY_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../json.hpp"

#include "../signal_base.hpp"

#include <array>    // std::array
#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <string>   // std::string

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                template <std::size_t... t_digits>
                struct transitionary_signal_chars
                {
                    static constexpr char signal_type_name[] = { 't', 'r', 'a', 'n', 's', 'i', 't', ' ', ('0' + t_digits)..., 0};
                };
                template <> struct transitionary_signal_chars<> { static constexpr char signal_type_name[] = "transit 0"; };
                template <std::size_t... t_digits> constexpr char transitionary_signal_chars<t_digits...>::signal_type_name[];

                template <std::size_t t_remainder, std::size_t... t_digits>
                struct named_transitionary_signal : public named_transitionary_signal<t_remainder / 10, t_remainder % 10, t_digits...> { };

                template <std::size_t... t_digits>
                struct named_transitionary_signal<0, t_digits...> : public transitionary_signal_chars<t_digits...> { };
            } // namespace detail

            /** Represents a transitionary signal. */
            template <typename t_value_type, std::size_t t_transition_size>
            struct transitionary_signal
                : public signal_base<transitionary_signal<t_value_type, t_transition_size>, t_value_type>,
                public detail::named_transitionary_signal<t_transition_size>
            {
                using type = transitionary_signal<t_value_type, t_transition_size>;
                using base_type = signal_base<type, t_value_type>;
                friend base_type;

                using value_type = typename base_type::value_type;
                using signal_base_type = typename base_type::signal_base_type;
                using transition_container_type = std::array<t_value_type, t_transition_size>;
                static constexpr std::size_t transition_size = t_transition_size;

                // ~~ Json names ~~
                static constexpr char jstr_signal_type[] = "type";
                static constexpr char jstr_transition[] = "transition";
                static constexpr char jstr_stationary_level[] = "stationary level";

            private:
                value_type m_stationary_level = 1;
                transition_container_type m_transition = { };

            public:
                /** Constant signal when no AR noise is present. */
                transitionary_signal() noexcept { }

                /** Constant signal when no AR noise is present. */
                transitionary_signal(value_type stationary_level, const transition_container_type& transition) noexcept
                    : m_stationary_level(stationary_level), m_transition(transition)
                {
                } // transitionary_signal(...)

                /** Signal level when in transition mode. */
                const transition_container_type& transition() const noexcept { return this->m_transition; }

                /** Signal level when in stationary mode. */
                value_type stationary_level() const noexcept { return this->m_stationary_level; }
                /** Signal level when in stationary mode. */
                void set_stationary_level(value_type value) noexcept { this->m_stationary_level = value; }

                /** Signal level when in transition mode. */
                value_type transitionary_level(std::size_t time_index) const noexcept { return this->m_transition[time_index]; }
                /** Signal level when in transition mode. */
                void set_transitionary_level(std::size_t time_index, value_type value) noexcept { this->m_transition[time_index] = value; }

                /** @brief Signal value at an arbitrary time. */
                value_type at(std::size_t time_index) const noexcept
                {
                    if (time_index < transition_size) return this->transitionary_level(time_index);
                    return this->stationary_level();
                } // at(...)

                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                }
            }; // struct transitionary_signal

            // ~~ Json name definitions ~~
            template <typename t_value_type, std::size_t t_transition_size> constexpr char transitionary_signal<t_value_type, t_transition_size>::jstr_signal_type[];
            template <typename t_value_type, std::size_t t_transition_size> constexpr char transitionary_signal<t_value_type, t_transition_size>::jstr_transition[];
            template <typename t_value_type, std::size_t t_transition_size> constexpr char transitionary_signal<t_value_type, t_transition_size>::jstr_stationary_level[];
            
            template <typename t_value_type, std::size_t t_transition_size>
            void to_json(nlohmann::json& j, const transitionary_signal<t_value_type, t_transition_size>& x) noexcept
            {
                using type = transitionary_signal<t_value_type, t_transition_size>;
                std::string signal_type_str = type::signal_type_name;

                j = nlohmann::json{
                    {type::jstr_signal_type, signal_type_str},
                    {type::jstr_transition, x.transition()},
                    {type::jstr_stationary_level, x.stationary_level()}
                };
            } // to_json(...)
        
            template <typename t_value_type, std::size_t t_transition_size>
            void from_json(const nlohmann::json& j, transitionary_signal<t_value_type, t_transition_size>& x) noexcept
            {
                quiet_json q(__FUNCTION__, __LINE__);
                using type = transitionary_signal<t_value_type, t_transition_size>;

                // Populate default values.
                std::string signal_type_str = type::signal_type_name;
                std::array<t_value_type, t_transition_size> transition = x.transition();
                t_value_type stationary_level = x.stationary_level();

                // Parse json entries.
                if (!quiet_json::required(j, type::jstr_signal_type, signal_type_str)) return;
                if (!quiet_json::required(j, type::jstr_transition, transition)) return;
                if (!quiet_json::required(j, type::jstr_stationary_level, stationary_level)) return;
                
                // Reconstruct the object.
                for (std::size_t i = 0; i < t_transition_size; ++i) x.set_transitionary_level(i, transition[i]);
                x.set_stationary_level(stationary_level);

                q.validate();
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TRANSITIONARY_SIGNAL_HPP_INCLUDED
