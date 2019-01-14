
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TRANSITIONARY_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TRANSITIONARY_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/on_error.hpp> // aftermath::detail::on_error
#include "../../draft/algebra/numbers.hpp"

#include "../signal_base.hpp"

#include <array>    // std::array
#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc
#include <vector>   // std::vector

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        template <std::size_t... t_digits>
        struct transitionary_signal_chars
        {
            static constexpr char typename_string[] = { 't', 'r', 'a', 'n', 's', 'i', 't', ' ', ('0' + t_digits)..., 0};
        }; // struct transitionary_signal_chars
        template <> struct transitionary_signal_chars<> { static constexpr char typename_string[] = "transit 0"; };
        template <std::size_t... t_digits> constexpr char transitionary_signal_chars<t_digits...>::typename_string[];

        template <std::size_t t_remainder, std::size_t... t_digits>
        struct named_transitionary_signal : public named_transitionary_signal<t_remainder / 10, t_remainder % 10, t_digits...> { };

        template <std::size_t... t_digits>
        struct named_transitionary_signal<0, t_digits...> : public transitionary_signal_chars<t_digits...> { };
    } // namespace detail

    template <typename t_value_type, std::size_t t_transition_size>
    struct transitionary_signal;
    template <typename t_value_type, std::size_t t_transition_size>
    void to_json(nlohmann::json& j, const transitionary_signal<t_value_type, t_transition_size>& x) noexcept;
    template <typename t_value_type, std::size_t t_transition_size>
    void from_json(const nlohmann::json& j, transitionary_signal<t_value_type, t_transition_size>& x);

    /** Represents a transitionary signal. */
    template <typename t_value_type, std::size_t t_transition_size>
    struct transitionary_signal
        : public signal_base<transitionary_signal<t_value_type, t_transition_size>, t_value_type>,
        public detail::named_transitionary_signal<t_transition_size>
    {
        using type = transitionary_signal<t_value_type, t_transition_size>;
        using value_type = t_value_type;
        using transition_container_type = std::array<value_type, t_transition_size>;

        using base_type = signal_base<type, value_type>;
        friend base_type;

        static constexpr std::size_t transition_size = t_transition_size;

        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_transition[] = "transition";
        static constexpr char jstr_stationary_level[] = "stationary level";

    private:
        value_type m_stationary_level = 1;
        transition_container_type m_transition = {};

    protected:
        bool validate(std::error_code& ec) const noexcept
        {
            if (modules::is_nan(this->m_stationary_level) || modules::is_infinite(this->m_stationary_level)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Signal level has to be a finite number.", false);
            for (const value_type& x : this->m_transition)
            {
                if (modules::is_nan(x) || modules::is_infinite(x)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Signal level has to be a finite number.", false);
            } // for (...)
            return true;
        } // validate(...)

        void coerce() noexcept
        {
            if (modules::is_nan(this->m_stationary_level) || modules::is_infinite(this->m_stationary_level)) this->m_stationary_level = 0;
            for (value_type& x : this->m_transition)
            {
                if (modules::is_nan(x) || modules::is_infinite(x)) x = 0;
            } // for (...)
        } // coerce(...)

    public:
        /** Constant signal when no AR noise is present. */
        transitionary_signal() noexcept { }

        /** Constant signal when no AR noise is present. */
        transitionary_signal(value_type stationary_level, const transition_container_type& transition, std::error_code& ec) noexcept
            : m_stationary_level(stationary_level), m_transition(transition)
        {
            if (!this->validate(ec)) this->coerce();
        } // transitionary_signal(...)
        
        transitionary_signal(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                aftermath::detail::on_error(ec, std::errc::invalid_argument, "Signal type mismatch.");
                return;
            } // if (...)

            // Parse json entries.
            aftermath::noexcept_json::required(j, type::jstr_stationary_level, this->m_stationary_level, ec);
            aftermath::noexcept_json::required(j, type::jstr_transition, this->m_transition, ec);
            
            if (!this->validate(ec)) this->coerce();
        } // transitionary_signal(...)

        /** Signal level when in transition mode. */
        const transition_container_type& transition() const noexcept { return this->m_transition; }

        /** Signal level when in stationary mode. */
        value_type stationary_level() const noexcept { return this->m_stationary_level; }
        /** Signal level when in stationary mode. */
        void set_stationary_level(value_type value, std::error_code& ec) noexcept
        {
            this->m_stationary_level = value;
            if (!this->validate(ec)) this->coerce();
        } // set_stationary_level(...)

        /** Signal level when in transition mode. */
        value_type transitionary_level(std::size_t time_index) const noexcept { return this->m_transition[time_index]; }
        /** Signal level when in transition mode. */
        void set_transitionary_level(std::size_t time_index, value_type value, std::error_code& ec) noexcept
        {
            this->m_transition[time_index] = value;
            if (!this->validate(ec)) this->coerce();
        } // set_transitionary_level(...)

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
        } // operator <<(...)
    }; // struct transitionary_signal

    // ~~ Json name definitions ~~
    template <typename t_value_type, std::size_t t_transition_size> constexpr char transitionary_signal<t_value_type, t_transition_size>::jstr_typename[];
    template <typename t_value_type, std::size_t t_transition_size> constexpr char transitionary_signal<t_value_type, t_transition_size>::jstr_transition[];
    template <typename t_value_type, std::size_t t_transition_size> constexpr char transitionary_signal<t_value_type, t_transition_size>::jstr_stationary_level[];
    
    template <typename t_value_type, std::size_t t_transition_size>
    void to_json(nlohmann::json& j, const transitionary_signal<t_value_type, t_transition_size>& x) noexcept
    {
        using type = transitionary_signal<t_value_type, t_transition_size>;
        std::string typename_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, typename_str},
            {type::jstr_transition, x.transition()},
            {type::jstr_stationary_level, x.stationary_level()}
        };
    } // to_json(...)

    template <typename t_value_type, std::size_t t_transition_size>
    void from_json(const nlohmann::json& j, transitionary_signal<t_value_type, t_transition_size>& x)
    {
        using type = transitionary_signal<t_value_type, t_transition_size>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TRANSITIONARY_SIGNAL_HPP_INCLUDED
