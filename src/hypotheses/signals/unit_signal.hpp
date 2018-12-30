
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_UNIT_SIGNAL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_UNIT_SIGNAL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/on_error.hpp> // aftermath::detail::on_error

#include "../signal_base.hpp"

#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct unit_signal;
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const unit_signal<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, unit_signal<t_value_type>& x);

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
        static constexpr char typename_string[] = "unit";

        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";

        /** Constant signal. */
        unit_signal() noexcept { }

        unit_signal(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                aftermath::detail::on_error(ec, std::errc::invalid_argument, "Signal type mismatch.");
                return;
            } // if (...)
        } // unit_signal(...)

        /** @brief Signal value at an arbitrary time. */
        constexpr value_type at(std::size_t /*time_index*/) const noexcept { return type::unit; }

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct unit_signal

    // ~~ Some definitions ~~
    template <typename t_value_type> constexpr char unit_signal<t_value_type>::typename_string[];
    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char unit_signal<t_value_type>::jstr_typename[];
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const unit_signal<t_value_type>& /*x*/) noexcept
    {
        using type = unit_signal<t_value_type>;
        std::string typename_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, typename_str}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, unit_signal<t_value_type>& x)
    {
        using type = unit_signal<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_UNIT_SIGNAL_HPP_INCLUDED
