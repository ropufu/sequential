
#ifndef ROPUFU_AFTERMATH_SLIDING_ARRAY_HPP_INCLUDED
#define ROPUFU_AFTERMATH_SLIDING_ARRAY_HPP_INCLUDED

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstring> // std::memmove
#include <utility> // std::swap

namespace ropufu::aftermath
{
    /** An auxiliary structure to discard old observations. */
    template <typename t_value_type, std::size_t t_size>
    struct sliding_array : public std::array<t_value_type, t_size>
    {
        using type = sliding_array<t_value_type, t_size>;
        using base_type = std::array<t_value_type, t_size>;
        using value_type = t_value_type;

        using base_type::array;

        /** Adds another element to the observed sequence. */
        void push_back(const value_type& value) noexcept
        {
            constexpr std::size_t tail_size = (t_size - 1);
            std::memmove(this->data(), this->data() + 1, tail_size * sizeof(value_type)); // Move the data.
            this->back() = value;
        } // push_back(...)
    }; // struct sliding_array

    /** Simple time window with two observations. */
    template <typename t_value_type>
    struct sliding_array<t_value_type, 2> : public std::array<t_value_type, 2>
    {
        using type = sliding_array<t_value_type, 2>;
        using base_type = std::array<t_value_type, 2>;
        using value_type = t_value_type;

        using base_type::array;

        /** Adds another element to the observed sequence. */
        void push_back(const value_type& value) noexcept
        {
            std::swap(this->front(), this->back());
            this->back() = value;
        } // push_back(...)
    }; // struct sliding_array

    /** Simple time window with one observation. */
    template <typename t_value_type>
    struct sliding_array<t_value_type, 1> : public std::array<t_value_type, 1>
    {
        using type = sliding_array<t_value_type, 1>;
        using base_type = std::array<t_value_type, 1>;
        using value_type = t_value_type;

        using base_type::array;

        /** Adds another element to the observed sequence. */
        void push_back(const value_type& value) noexcept
        {
            this->back() = value;
        } // push_back(...)
    }; // struct sliding_array

    /** Trivial time window. */
    template <typename t_value_type>
    struct sliding_array<t_value_type, 0> : public std::array<t_value_type, 0>
    {
        using type = sliding_array<t_value_type, 0>;
        using base_type = std::array<t_value_type, 0>;
        using value_type = t_value_type;

        using base_type::array;

        /** Adds another element to the observed sequence. */
        void push_back(const value_type& /*value*/) noexcept { }
    }; // struct sliding_array
} // namespace ropufu::aftermath

#endif // ROPUFU_AFTERMATH_SLIDING_ARRAY_HPP_INCLUDED
