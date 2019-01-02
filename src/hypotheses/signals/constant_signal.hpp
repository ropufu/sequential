
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_CONSTANT_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_CONSTANT_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/on_error.hpp> // aftermath::detail::on_error
#include "../draft/algebra/numbers.hpp"

#include "../signal_base.hpp"

#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
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
    struct constant_signal : public signal_base<constant_signal<t_value_type>, t_value_type>
    {
        using type = constant_signal<t_value_type>;
        using base_type = signal_base<type, t_value_type>;
        friend base_type;
        
        using value_type = typename base_type::value_type;
        using signal_base_type = typename base_type::signal_base_type;
        static constexpr char typename_string[] = "const";

        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_level[] = "level";

    private:
        value_type m_level = 1;

    protected:
        bool validate(std::error_code& ec) const noexcept
        {
            if (modules::is_nan(this->m_level) || modules::is_infinite(this->m_level)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Signal level has to be a finite number.", false);
            return true;
        } // validate(...)

        void coerce() noexcept
        {
            if (modules::is_nan(this->m_level) || modules::is_infinite(this->m_level)) this->m_level = 0;
        } // coerce(...)

    public:
        /** Constant signal. */
        constant_signal() noexcept { }

        /** Constant signal. */
        constant_signal(value_type level, std::error_code& ec) noexcept
            : m_level(level)
        {
            if (!this->validate(ec)) this->coerce();
        } // constant_signal(...)

        constant_signal(const nlohmann::json& j, std::error_code& ec) noexcept
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
            aftermath::noexcept_json::required(j, type::jstr_level, this->m_level, ec);
            
            if (!this->validate(ec)) this->coerce();
        } // constant_signal(...)

        /** Signal level. */
        value_type level() const noexcept { return this->m_level; }
        /** Signal level. */
        void set_level(value_type value, std::error_code& ec) noexcept
        { 
            this->m_level = value;
            if (!this->validate(ec)) this->coerce();
        } // set_level(...)

        /** @brief Signal value at an arbitrary time. */
        value_type at(std::size_t /*time_index*/) const noexcept { return this->level(); }

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
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_CONSTANT_SIGNAL_HPP_INCLUDED
