
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/enum_parser.hpp>
#include <ropufu/enum_array.hpp>

#include "../draft/format.hpp"

#include <cstddef>      // std::size_t
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string, std::to_string
#include <system_error> // std::error_code, std::errc
#include <type_traits>  // std::underlying_type_t

namespace ropufu::sequential::hypotheses
{
    enum struct operating_characteristic : char
    {
        unknown = 0,
        ess_under_null = 1,
        ess_under_alt = 2,
        probability_of_false_alarm = 3,
        probability_of_missed_signal = 4
    }; // enum struct operating_characteristic

    void to_json(nlohmann::json& j, const operating_characteristic& x) noexcept;

    void from_json(const nlohmann::json& j, operating_characteristic& x);
} // namespace ropufu::sequential::hypotheses

namespace std
{
    std::string to_string(ropufu::sequential::hypotheses::operating_characteristic x) noexcept
    {
        using argument_type = ropufu::sequential::hypotheses::operating_characteristic;
        switch (x)
        {
            case argument_type::unknown: return "unknown";
            case argument_type::ess_under_null: return "ess null";
            case argument_type::ess_under_alt: return "ess alt";
            case argument_type::probability_of_false_alarm: return "pfa";
            case argument_type::probability_of_missed_signal: return "pms";
            default: return "unknown <operating_characteristic> " + std::to_string(static_cast<std::size_t>(x));
        } // switch (...)
    } // to_string(...)
} // namespace std

namespace ropufu::aftermath::detail
{
    template <>
    struct enum_array_keys<ropufu::sequential::hypotheses::operating_characteristic>
    {
        using underlying_type = std::underlying_type_t<ropufu::sequential::hypotheses::operating_characteristic>;
        static constexpr underlying_type first_index = 1;
        static constexpr underlying_type past_the_last_index = 5;
    }; // struct enum_array_keys<...>

    template <>
    struct enum_parser<ropufu::sequential::hypotheses::operating_characteristic>
    {
        using enum_type = ropufu::sequential::hypotheses::operating_characteristic;

        static std::string to_string(const enum_type& from) noexcept
        {
            return std::to_string(from);
        } // to_string(...)

        static bool try_parse(const std::string& from, enum_type& to) noexcept
        {
            std::string x = ropufu::draft::detail::transform(from,
                [] (const char& c) { return (c == '_' ? ' ' : c); });

            if (x == "unknown") { to = enum_type::unknown; return true; }
            if (x == "ess null" || x == "vss null" || x == "ss null") { to = enum_type::ess_under_null; return true; }
            if (x == "ess alt" || x == "vss alt" || x == "ss alt") { to = enum_type::ess_under_alt; return true; }
            if (x == "pfa" || x == "vfa" || x == "fa") { to = enum_type::probability_of_false_alarm; return true; }
            if (x == "pms" || x == "vms" || x == "ms") { to = enum_type::probability_of_missed_signal; return true; }
            return false;
        } // try_parse(...)
    }; // struct enum_parser<...>
} // namespace ropufu::aftermath::detail

namespace ropufu::sequential::hypotheses
{
    void to_json(nlohmann::json& j, const operating_characteristic& x) noexcept
    {
        j = aftermath::detail::enum_parser<operating_characteristic>::to_string(x);
    } // to_json(...)

    void from_json(const nlohmann::json& j, operating_characteristic& x)
    {
        if (!j.is_string()) throw std::runtime_error("Parsing <operating_characteristic> failed: " + j.dump());
        std::string s = j;
        if (!aftermath::detail::try_parse_enum(s, x)) throw std::runtime_error("<operating_characteristic> not recognized: " + j.dump());
    } // from_json(...)

    template <typename t_value_type>
    using oc_array_t = aftermath::enum_array<operating_characteristic, t_value_type>;

    namespace detail
    {
        bool mat_var_name(operating_characteristic oc, std::string& expected_value, std::string& variance) noexcept
        {
            switch (oc)
            {
                case operating_characteristic::ess_under_null:
                    expected_value = "ess_null"; variance = "vss_null";
                    return true;
                case operating_characteristic::ess_under_alt:
                    expected_value = "ess_alt"; variance = "vss_alt";
                    return true;
                case operating_characteristic::probability_of_false_alarm:
                    expected_value = "pfa"; variance = "vfa";
                    return true;
                case operating_characteristic::probability_of_missed_signal:
                    expected_value = "pms"; variance = "vms";
                    return true;
                default: return false;
            } // switch (...)
        } // mat_var_name(...)
    } // namespace detail
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED
