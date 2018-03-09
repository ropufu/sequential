
#ifndef ROPUFU_MODULES_NUMBERS_HPP_INCLUDED
#define ROPUFU_MODULES_NUMBERS_HPP_INCLUDED

#include <cmath> // std::isnan, std::isinf
#include <type_traits> // std::is_floating_point_v

namespace ropufu
{
    namespace modules
    {
        template <typename t_value_type>
        struct numbers
        {
            static bool is_nan(const t_value_type& value) noexcept
            {
                if constexpr (std::is_floating_point_v<t_value_type>) return std::isnan(value);
                else return false;
            } // is_nan(...)

            static bool is_infinite(const t_value_type& value) noexcept
            {
                if constexpr (std::is_floating_point_v<t_value_type>) return std::isinf(value);
                else return false;
            } // is_infinite(...)
        }; // struct numbers<...>

        template <typename t_value_type>
        bool is_nan(const t_value_type& value) noexcept { return numbers<t_value_type>::is_nan(value); }

        template <typename t_value_type>
        bool is_infinite(const t_value_type& value) noexcept { return numbers<t_value_type>::is_infinite(value); }

        template <typename t_value_type>
        struct clipper
        {
            using type = clipper<t_value_type>;
            using value_type = t_value_type;
            using numbers_type = numbers<t_value_type>;

            static bool was_finite(value_type& value, const value_type& fallback) noexcept
            {
                if (numbers_type::is_nan(value) || numbers_type::is_infinite(value))
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
    } // namespace modules
} // namespace ropufu

#endif // ROPUFU_MODULES_NUMBERS_HPP_INCLUDED
