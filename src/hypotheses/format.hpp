
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_FORMAT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_FORMAT_HPP_INCLUDED

#include <cstddef> // std::size_t
#include <string>  // std::string, std::to_string
#include <type_traits> // std::is_floating_point_v
#include <vector>  // std::vector

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        template <typename t_type, bool t_do_rounding = std::is_floating_point_v<t_type>>
        struct format
        {
            static std::string to_string(t_type x, std::size_t decimal_places) noexcept { return std::to_string(x); }
        }; // struct format
        
        template <typename t_type>
        struct format<t_type, true>
        {
            static std::string to_string(t_type x, std::size_t decimal_places) noexcept
            {
                bool is_negative = (x < 0);
                if (is_negative) x = -x;

                constexpr std::size_t ten = 10;
                std::size_t denominator = 1;
                for (std::size_t i = 0; i < decimal_places; ++i) denominator *= ten;

                std::size_t upscaled = static_cast<std::size_t>(x * denominator + 0.5); // Round toward the nearest integer.
                std::size_t whole = upscaled / denominator;
                upscaled -= denominator * whole;

                std::string result = is_negative ? "-" : "";
                result += std::to_string(whole);
                if (upscaled != 0)
                {
                    result += ".";
                    while (upscaled != 0)
                    {
                        denominator /= ten;
                        whole = upscaled / denominator; // Next digit.
                        result += std::to_string(whole);
                        upscaled -= denominator * whole;
                    } // while(...)
                } // if (...)
                return result;
            } // to_string(...)
        }; // struct format<...>

        template <typename t_type>
        std::string to_str(t_type x, std::size_t decimal_places) noexcept { return format<t_type>::to_string(x, decimal_places); }

        template <typename t_transform_type>
        std::string transform(const std::string& value, t_transform_type&& transform) noexcept
        {
            std::vector<char> result {};
            result.reserve(value.size());
            for (const char& c : value) result.push_back(transform(c));
            return {result.data(), result.size()};
        } // transform(...)
        
        template <typename t_transform_type, typename t_predicate_type>
        std::string transform(const std::string& value, t_transform_type&& transform, t_predicate_type&& filter) noexcept
        {
            std::vector<char> result {};
            result.reserve(value.size());
            for (const char& c : value) if (filter(c)) result.push_back(transform(c));
            return {result.data(), result.size()};
        } // transform(...)
    } // namespace detail
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_FORMAT_HPP_INCLUDED
