
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_TRANSITIONARY_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_TRANSITIONARY_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include <array>     // std::array
#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error, std::logic_error
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc
#include <vector>    // std::vector

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
        : public detail::named_transitionary_signal<t_transition_size>
    {
        using type = transitionary_signal<t_value_type, t_transition_size>;
        using value_type = t_value_type;
        using transition_container_type = std::array<value_type, t_transition_size>;

        static constexpr std::size_t transition_size = t_transition_size;

        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_transition[] = "transition";
        static constexpr char jstr_stationary_level[] = "stationary level";

    private:
        value_type m_stationary_level = 0;
        transition_container_type m_transition = {};

        static bool is_valid(value_type stationary_level, const transition_container_type& transition, std::string& message) noexcept
        {
            if (!aftermath::is_finite(stationary_level))
            {
                message = "Signal level must be finite.";
                return false;
            } // if (...)
            for (const value_type& x : transition)
            {
                if (!aftermath::is_finite(x))
                {
                    message = "Signal level must be finite.";
                    return false;
                } // if (...)
            } // for (...)
            return true;
        } // validate(...)

        void validate() const
        {
            std::string message {};
            if (!type::is_valid(this->m_stationary_level, this->m_transition, message))
                throw std::logic_error(message);
        } // validate(...)

    public:
        /** Zero signal. */
        transitionary_signal() noexcept { }

        /** @brief Constant signal that starts at the stationary level.
         *  @exception std::logic_error \p stationary_level is not finite.
         */
        explicit transitionary_signal(value_type stationary_level)
            : m_stationary_level(stationary_level)
        {
            this->m_transition.fill(stationary_level);
            this->validate();
        } // transitionary_signal(...)

        /** @brief Transitionary signal.
         *  @exception std::logic_error \p stationary_level is not finite.
         *  @exception std::logic_error \p transition is not finite.
         */
        transitionary_signal(value_type stationary_level, const transition_container_type& transition)
            : m_stationary_level(stationary_level), m_transition(transition)
        {
            this->validate();
        } // transitionary_signal(...)
        
        transitionary_signal(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                ec = std::make_error_code(std::errc::bad_message); // Signal type mismatch.
                return;
            } // if (...)

            // Parse json entries.
            value_type stationary_level = this->m_stationary_level;
            transition_container_type transition = this->m_transition;
            aftermath::noexcept_json::required(j, type::jstr_stationary_level, stationary_level, ec);
            aftermath::noexcept_json::required(j, type::jstr_transition, transition, ec);
            if (ec.value() != 0) return;

            // Validate entries.
            std::string message {};
            if (!type::is_valid(stationary_level, transition, message))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
            
            // Populate values.
            this->m_stationary_level = stationary_level;
            this->m_transition = transition;
        } // transitionary_signal(...)

        /** @brief Signal level when in stationary mode. */
        value_type stationary_level() const noexcept { return this->m_stationary_level; }
        /** @brief Signal level when in stationary mode.
         *  @exception std::logic_error \p value is not finite.
         */
        void set_stationary_level(value_type value) noexcept
        {
            this->m_stationary_level = value;
            this->validate();
        } // set_stationary_level(...)

        /** @brief Signal level when in transition mode. */
        const transition_container_type& transition() const noexcept { return this->m_transition; }
        /** @brief Signal level when in transition mode. */
        value_type transition(std::size_t time_index) const { return this->m_transition.at(time_index); }
        /** @brief Signal level when in transition mode.
         *  @exception std::logic_error \p value is not finite.
         */
        void set_transition(std::size_t time_index, value_type value) noexcept
        {
            this->m_transition.at(time_index) = value;
            this->validate();
        } // set_transition(...)

        /** @brief Signal value at an arbitrary time. */
        value_type at([[maybe_unused]] std::size_t time_index) const noexcept
        {
            if constexpr (type::transition_size == 0) return this->m_stationary_level;
            else
            {
                if (time_index < type::transition_size) return this->m_transition[time_index];
                return this->m_stationary_level;
            } // if constexpr (...)
        } // at(...)

        /** @brief Signal value at an arbitrary time. */
        constexpr value_type operator ()(std::size_t time_index) const noexcept { return this->at(time_index); }
        
        /** @brief Signal value at an arbitrary time. */
        constexpr value_type operator [](std::size_t time_index) const noexcept { return this->at(time_index); }

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_stationary_level == other.m_stationary_level &&
                this->m_transition == other.m_transition;
        } // operator ==(...)

        bool operator !=(const type& other) const noexcept
        {
            return !this->operator ==(other);
        } // operator !=(...)

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
        if (ec.value() != 0) throw std::runtime_error("Parsing <transitionary_signal> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_TRANSITIONARY_SIGNAL_HPP_INCLUDED
