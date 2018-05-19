
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED

#include <aftermath/enum_array.hpp>

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::int_fast32_t
#include <string>  // std::string, std::to_string
#include <type_traits> // std::underlying_type_t


namespace ropufu::sequential::hypotheses
{
    enum struct operating_characteristic : std::int_fast32_t
    {
        unknown = 0,
        ess_under_null = 1,
        ess_under_alt = 2,
        probability_of_false_alarm = 3,
        probability_of_missed_signal = 4
    }; // enum struct operating_characteristic
} // namespace ropufu::sequential::hypotheses

namespace std
{
    std::string to_string(ropufu::sequential::hypotheses::operating_characteristic x) noexcept
    {
        using argument_type = ropufu::sequential::hypotheses::operating_characteristic;
        switch (x)
        {
            case argument_type::ess_under_null: return "ess_null";
            case argument_type::ess_under_alt: return "ess_alt";
            case argument_type::probability_of_false_alarm: return "pfa";
            case argument_type::probability_of_missed_signal: return "pms";
            default: return "unknown";
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
            if (from == "ess_null" || from == "vss_null" || from == "ss_null") { to = enum_type::ess_under_null; return true; }
            if (from == "ess_alt" || from == "vss_alt" || from == "ss_alt") { to = enum_type::ess_under_alt; return true; }
            if (from == "pfa" || from == "vfa" || from == "fa") { to = enum_type::probability_of_false_alarm; return true; }
            if (from == "pms" || from == "vms" || from == "ms") { to = enum_type::probability_of_missed_signal; return true; }
            return false;
        }
    }; // struct enum_parser<...>
} // namespace ropufu::aftermath::detail

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    using oc_array_t = ropufu::aftermath::enum_array<ropufu::sequential::hypotheses::operating_characteristic, t_value_type>;

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
