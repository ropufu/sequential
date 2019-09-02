
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_FLAVOR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_FLAVOR_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/enum_parser.hpp>

#include <cstddef>      // std::size_t
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string, std::to_string
#include <system_error> // std::error_code, std::errc

namespace ropufu::sequential::hypotheses
{
    enum struct adaptive_sprt_flavor : char
    {
        general, // Taking most likely density over the entire alternative case.
        simple // Taking the density at the smallest alternative signal strength.
    }; // enum struct adaptive_sprt_flavor

    void to_json(nlohmann::json& j, const adaptive_sprt_flavor& x) noexcept;

    void from_json(const nlohmann::json& j, adaptive_sprt_flavor& x);
} // namespace ropufu::sequential::hypotheses

namespace std
{
    std::string to_string(ropufu::sequential::hypotheses::adaptive_sprt_flavor x) noexcept
    {
        using argument_type = ropufu::sequential::hypotheses::adaptive_sprt_flavor;
        switch (x)
        {
            case argument_type::general: return "general";
            case argument_type::simple: return "simple";
            default: return "unknown <adaptive_sprt_flavor> " + std::to_string(static_cast<std::size_t>(x));
        } // switch (...)
    } // to_string(...)
} // namespace std

namespace ropufu::aftermath::detail
{
    template <>
    struct enum_parser<ropufu::sequential::hypotheses::adaptive_sprt_flavor>
    {
        using enum_type = ropufu::sequential::hypotheses::adaptive_sprt_flavor;

        static std::string to_string(const enum_type& from) noexcept { return std::to_string(from); }

        static bool try_parse(const std::string& from, enum_type& to) noexcept
        {
            if (from == "general") { to = enum_type::general; return true; }
            if (from == "simple") { to = enum_type::simple; return true; }
            return false;
        } // try_parse(...)
    }; // struct enum_parser<...>
} // namespace ropufu::aftermath::detail

namespace ropufu::sequential::hypotheses
{
    void to_json(nlohmann::json& j, const adaptive_sprt_flavor& x) noexcept
    {
        j = aftermath::detail::enum_parser<adaptive_sprt_flavor>::to_string(x);
    } // to_json(...)

    void from_json(const nlohmann::json& j, adaptive_sprt_flavor& x)
    {
        if (!j.is_string()) throw std::runtime_error("Parsing <adaptive_sprt_flavor> failed: " + j.dump());
        std::string s = j;
        if (!aftermath::detail::try_parse_enum(s, x)) throw std::runtime_error("<adaptive_sprt_flavor> not recognized: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_FLAVOR_HPP_INCLUDED
