
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_CONSTANT_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_CONSTANT_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc

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
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_level[] = "level";

    private:
        value_type m_level = 0;

        static bool is_valid(value_type level, std::string& message) noexcept
        {
            if (!aftermath::is_finite(level))
            {
                message = "Signal level must be finite.";
                return false;
            } // if (...)
            return true;
        } // validate(...)

        void validate() const
        {
            std::string message {};
            if (!type::is_valid(this->m_level, message))
                throw std::logic_error(message);
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

        constant_signal(const nlohmann::json& j, std::error_code& ec) noexcept
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
            value_type level = this->m_level;
            aftermath::noexcept_json::required(j, type::jstr_level, level, ec);
            if (ec.value() != 0) return;

            // Validate entries.
            std::string message {};
            if (!type::is_valid(level, message))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
            
            // Populate values.
            this->m_level = level;
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
    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char constant_signal<t_value_type>::jstr_typename[];
    template <typename t_value_type> constexpr char constant_signal<t_value_type>::jstr_level[];
    
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
        using type = constant_signal<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <constant_signal> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNALS_CONSTANT_SIGNAL_HPP_INCLUDED
