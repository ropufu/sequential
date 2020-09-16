
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_CONSTANT_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_CONSTANT_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include <cstddef>     // std::size_t
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct constant_signal;

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const constant_signal<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, constant_signal<t_value_type>& x);

    /** Represents a constant signal. */
    template <typename t_value_type>
    struct constant_signal
    {
        using type = constant_signal<t_value_type>;
        using value_type = t_value_type;

        static constexpr char typename_string[] = "const";

        // ~~ Json names ~~
        static constexpr std::string_view jstr_typename = "type";
        static constexpr std::string_view jstr_level = "level";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        value_type m_level = 0;

        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_finite(this->m_level)) return "Signal level must be finite.";
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        /** Zero signal. */
        constant_signal() noexcept { }

        /** @brief Constant signal.
         *  @exception std::logic_error \p level is not finite.
         */
        explicit constant_signal(value_type level)
            : m_level(level)
        {
            this->validate();
        } // constant_signal(...)

        /** @brief Signal level. */
        value_type level() const noexcept { return this->m_level; }
        /** @brief Signal level.
         *  @exception std::logic_error \p value is not finite.
         */
        void set_level(value_type value) noexcept
        {
            this->m_level = value;
            this->validate();
        } // set_stationary_level(...)

        /** @brief Signal value at an arbitrary time. */
        value_type at(std::size_t /*time_index*/) const noexcept { return this->m_level; }

        /** @brief Signal value at an arbitrary time. */
        value_type operator ()(std::size_t /*time_index*/) const noexcept { return this->m_level; }
        
        /** @brief Signal value at an arbitrary time. */
        value_type operator [](std::size_t /*time_index*/) const noexcept { return this->m_level; }

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_level == other.m_level;
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
    }; // struct constant_signal

    // ~~ Some definitions ~~
    template <typename t_value_type> constexpr char constant_signal<t_value_type>::typename_string[];
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const constant_signal<t_value_type>& x) noexcept
    {
        using type = constant_signal<t_value_type>;
        std::string typename_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, typename_str},
            {type::jstr_level, x.level()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, constant_signal<t_value_type>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <constant_signal> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::constant_signal<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::constant_signal<t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            if (!noexcept_json::required(j, result_type::jstr_typename, typename_str)) return false;
            if (typename_str != result_type::typename_string) return false; // Signal type mismatch.

            // Parse json entries.
            if (!noexcept_json::required(j, result_type::jstr_level, x.m_level)) return false;
            
            // Validate entries.
            if (x.error_message().has_value()) return false;

            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_CONSTANT_SIGNAL_HPP_INCLUDED
