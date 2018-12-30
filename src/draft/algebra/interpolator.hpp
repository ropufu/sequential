
#ifndef ROPUFU_MODULES_INTERPOLATOR_HPP_INCLUDED
#define ROPUFU_MODULES_INTERPOLATOR_HPP_INCLUDED

#include "numbers.hpp"

#include <system_error> // std::error_code, std::make_error_code
#include <vector>       // std::vector

namespace ropufu::modules
{
    template <typename t_value_type, typename t_position_type>
    struct interpolator
    {
        using type = interpolator<t_value_type, t_position_type>;
        using value_type = t_value_type;
        using position_type = t_position_type;
        using clipper_type = clipper<t_position_type>;

        static value_type interpolate(const value_type& left, const value_type& right, const position_type& relative_position, std::error_code& ec) noexcept
        {
            position_type p = relative_position; // Make a copy of <relative_position>.

            if (!clipper_type::was_finite(p, 0) || !clipper_type::was_between(p, 0, 1)) 
                ec = std::make_error_code(std::errc::argument_out_of_domain);

            position_type q = 1 - p;
            return (q) * left + (p) * right;
        } // interpolate(...)
    }; // struct interpolator

    template <typename t_value_type, typename t_position_type>
    struct interpolator<std::vector<t_value_type>, t_position_type>
    {
        using type = interpolator<t_value_type, t_position_type>;
        using value_type = std::vector<t_value_type>;
        using position_type = t_position_type;
        using clipper_type = clipper<t_position_type>;

        static value_type interpolate(const value_type& left, const value_type& right, const position_type& relative_position, std::error_code& ec) noexcept
        {
            value_type bad {};
            std::size_t count = left.size();
            if (left.size() != right.size())
            {
                ec = std::make_error_code(std::errc::invalid_argument);
                return bad;
            } // if (...)

            position_type p = relative_position; // Make a copy of <relative_position>.

            if (!clipper_type::was_finite(p, 0) || !clipper_type::was_between(p, 0, 1))
                ec = std::make_error_code(std::errc::argument_out_of_domain);

            position_type q = 1 - p;

            value_type result(count);
            for (std::size_t i = 0; i < count; ++i) result[i] = (q) * left[i] + (p) * right[i];
            return result;
        } // interpolate(...)
    }; // struct interpolator<...>
} // namespace ropufu::modules

#endif // ROPUFU_MODULES_INTERPOLATOR_HPP_INCLUDED
