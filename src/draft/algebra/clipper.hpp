
#ifndef ROPUFU_MODULES_NUMBERS_HPP_INCLUDED
#define ROPUFU_MODULES_NUMBERS_HPP_INCLUDED

#include <ropufu/number_traits.hpp>

namespace ropufu::modules
{
    template <typename t_value_type>
    struct clipper
    {
        using type = clipper<t_value_type>;
        using value_type = t_value_type;

        static bool was_finite(value_type& value, const value_type& fallback) noexcept
        {
            if (!aftermath::is_finite(value))
            {
                value = fallback;
                return false;
            } // if (...)
            return true;
        } // finite(...)

        static bool was_below(value_type& value, const value_type& upper_bound) noexcept
        {
            if (upper_bound < value)
            {
                value = upper_bound;
                return false;
            } // if (...)
            return true;
        } // below(...)

        static bool was_above(value_type& value, const value_type& lower_bound) noexcept
        {
            if (value < lower_bound)
            {
                value = lower_bound;
                return false;
            } // if (...)
            return true;
        } // above(...)

        static bool was_between(value_type& value, const value_type& lower_bound, const value_type& upper_bound) noexcept
        {
            if (!type::was_above(value, lower_bound)) return false;
            if (!type::was_below(value, upper_bound)) return false;
            return true;
        } // between(...)
    }; // struct clipper
} // namespace ropufu::modules

#endif // ROPUFU_MODULES_NUMBERS_HPP_INCLUDED
