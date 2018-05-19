
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_CONSTANT_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_CONSTANT_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <aftermath/quiet_json.hpp>

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
            struct constant_signal : public signal_base<constant_signal<t_value_type>, t_value_type>
            {
                using type = constant_signal<t_value_type>;
                using base_type = signal_base<type, t_value_type>;
                friend base_type;
                
                using value_type = typename base_type::value_type;
                using signal_base_type = typename base_type::signal_base_type;
                static constexpr char signal_type_name[] = "const";

                // ~~ Json names ~~
                static constexpr char jstr_signal_type[] = "type";
                static constexpr char jstr_level[] = "level";

            private:
                value_type m_level = 1;

            public:
                /** Constant signal. */
                constant_signal() noexcept { }

                /** Constant signal. */
                constant_signal(value_type level) noexcept
                    : m_level(level)
                {
                } // constant_signal(...)

                /** Signal level. */
                value_type level() const noexcept { return this->m_level; }
                /** Signal level. */
                void set_level(value_type value) noexcept { this->m_level = value; }

                /** @brief Signal value at an arbitrary time. */
                value_type at(std::size_t /**time_index*/) const noexcept { return this->level(); }

                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct constant_signal

            // ~~ Some definitions ~~
            template <typename t_value_type> constexpr char constant_signal<t_value_type>::signal_type_name[];
            // ~~ Json name definitions ~~
            template <typename t_value_type> constexpr char constant_signal<t_value_type>::jstr_signal_type[];
            template <typename t_value_type> constexpr char constant_signal<t_value_type>::jstr_level[];
            
            template <typename t_value_type>
            void to_json(nlohmann::json& j, const constant_signal<t_value_type>& x) noexcept
            {
                using type = constant_signal<t_value_type>;
                std::string signal_type_str = type::signal_type_name;

                j = nlohmann::json{
                    {type::jstr_signal_type, signal_type_str},
                    {type::jstr_level, x.level()}
                };
            } // to_json(...)
        
            template <typename t_value_type>
            void from_json(const nlohmann::json& j, constant_signal<t_value_type>& x) noexcept
            {
                aftermath::quiet_json q(j);
                using type = constant_signal<t_value_type>;

                // Populate default values.
                std::string signal_type_str = type::signal_type_name;
                t_value_type level = x.level();

                // Parse json entries.
                q.required(type::jstr_signal_type, signal_type_str);
                q.required(type::jstr_level, level);
                
                // Reconstruct the object.
                if (!q.good())
                {
                    aftermath::quiet_error::instance().push(
                        aftermath::not_an_error::runtime_error,
                        aftermath::severity_level::major,
                        q.message(), __FUNCTION__, __LINE__);
                    return;
                } // if (...)
                x.set_level(level);
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_CONSTANT_SIGNAL_HPP_INCLUDED
