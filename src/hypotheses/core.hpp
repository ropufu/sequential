
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_CORE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_CORE_HPP_INCLUDED

#include <cstddef> // std::size_t
#include <string>  // std::string, std::to_string
#include <type_traits> // std::is_floating_point

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                template <typename t_type, bool t_do_rounding = std::is_floating_point<t_type>::value>
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
                            }
                        }
                        return result;
                    } // to_string(...)
                }; // struct format<...>

                template <typename t_type>
                std::string to_str(t_type x, std::size_t decimal_places) noexcept { return format<t_type>::to_string(x, decimal_places); }
            } // namespace detail
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_CORE_HPP_INCLUDED
