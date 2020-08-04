
#ifndef ROPUFU_MODULES_INTERPOLATOR_HPP_INCLUDED
#define ROPUFU_MODULES_INTERPOLATOR_HPP_INCLUDED

#include <ropufu/number_traits.hpp>

#include <concepts>  // std::floating_point
#include <stdexcept> // std::logic_error
#include <vector>    // std::vector

namespace ropufu::modules
{
    template <typename t_value_type, std::floating_point t_position_type>
    struct interpolator
    {
        using type = interpolator<t_value_type, t_position_type>;
        using value_type = t_value_type;
        using position_type = t_position_type;

        static value_type interpolate(const value_type& left, const value_type& right, const position_type& relative_position)
        {
            if (!aftermath::is_probability(relative_position)) throw std::logic_error("Relative position not in [0, 1].");
            return (1 - relative_position) * left + (relative_position) * right;
        } // interpolate(...)
    }; // struct interpolator

    template <typename t_scalar_type, std::floating_point t_position_type>
    struct interpolator<std::vector<t_scalar_type>, t_position_type>
    {
        using type = interpolator<t_scalar_type, t_position_type>;
        using scalar_type = t_scalar_type;
        using value_type = std::vector<scalar_type>;
        using position_type = t_position_type;

        static value_type interpolate(const value_type& left, const value_type& right, const position_type& relative_position)
        {
            std::size_t count = left.size();
            if (left.size() != right.size()) throw std::logic_error("Vectors incompatible.");
            if (!aftermath::is_probability(relative_position)) throw std::logic_error("Relative position not in [0, 1].");

            position_type q = 1 - relative_position;

            value_type result {};
            result.reserve(count);
            for (std::size_t i = 0; i < count; ++i) result.push_back((q) * left[i] + (relative_position) * right[i]);
            result.shrink_to_fit();
            return result;
        } // interpolate(...)
    }; // struct interpolator<...>
} // namespace ropufu::modules

#endif // ROPUFU_MODULES_INTERPOLATOR_HPP_INCLUDED
