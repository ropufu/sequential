
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_SPACING_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_SPACING_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/enum_parser.hpp>

#include <cstddef>      // std::size_t
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string, std::to_string
#include <system_error> // std::error_code, std::errc
#include <type_traits>  // std::underlying_type_t

namespace ropufu::sequential::hypotheses
{
    enum struct spacing : char
    {
        linear = 0,
        logarithmic = 1,
        exponential = 2
    }; // struct spacing

    void to_json(nlohmann::json& j, const spacing& x) noexcept;

    void from_json(const nlohmann::json& j, spacing& x);
} // namespace ropufu::sequential::hypotheses

namespace std
{
    std::string to_string(ropufu::sequential::hypotheses::spacing x) noexcept
    {
        using argument_type = ropufu::sequential::hypotheses::spacing;
        switch (x)
        {
            case argument_type::linear: return "linear";
            case argument_type::logarithmic: return "logarithmic";
            case argument_type::exponential: return "exponential";
            default: return "unknown <spacing> " + std::to_string(static_cast<std::size_t>(x));
        } // switch (...)
    } // to_string(...)
} // namespace std

namespace ropufu::aftermath::detail
{
    template <>
    struct enum_parser<ropufu::sequential::hypotheses::spacing>
    {
        using enum_type = ropufu::sequential::hypotheses::spacing;

        static std::string to_string(const enum_type& from) noexcept
        {
            return std::to_string(from);
        } // to_string(...)

        static bool try_parse(const std::string& from, enum_type& to) noexcept
        {
            if (from == "linear" || from == "lin") { to = enum_type::linear; return true; }
            if (from == "logarithmic" || from == "log" || from == "ln") { to = enum_type::logarithmic; return true; }
            if (from == "exponential" || from == "exp") { to = enum_type::exponential; return true; }
            return false;
        } // try_parse(...)
    }; // struct enum_parser<...>
} // namespace ropufu::aftermath::detail

namespace ropufu::sequential::hypotheses
{
    void to_json(nlohmann::json& j, const spacing& x) noexcept
    {
        j = aftermath::detail::enum_parser<spacing>::to_string(x);
    } // to_json(...)

    void from_json(const nlohmann::json& j, spacing& x)
    {
        if (!j.is_string()) throw std::runtime_error("Parsing <spacing> failed: " + j.dump());
        std::string s = j;
        if (!aftermath::detail::try_parse_enum(s, x)) throw std::runtime_error("<spacing> not recognized: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_SPACING_HPP_INCLUDED
