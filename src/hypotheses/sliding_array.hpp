
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SLIDING_ARRAY_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SLIDING_ARRAY_HPP_INCLUDED

#include <array>   // std::array
#include <cmath>   // std::nan
#include <cstddef> // std::size_t
#include <cstring> // std::memmove

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** An auxiliary structure to discard old observations. */
            template <typename t_value_type, std::size_t t_tail_size>
            struct sliding_array : public std::array<t_value_type, t_tail_size + 1>
            {
                using type = sliding_array<t_value_type, t_tail_size>;
                using base_type = std::array<t_value_type, t_tail_size + 1>;
                using value_type = t_value_type;

                using base_type::base_type;

                /** Number of the past observations stored. */
                constexpr std::size_t tail_size() const noexcept { return t_tail_size; }

                /** Adds another element to the observed sequence. */
                void push_back(const value_type& value) noexcept
                {
                    std::memmove(this->data(), this->data() + sizeof(value_type), t_tail_size * sizeof(value_type)); // Move the data.
                    this->back() = value;
                } // push_back(...)
            }; // struct sliding_array

            /** Trivial time window. */
            template <typename t_value_type>
            struct sliding_array<t_value_type, 0> : public std::array<t_value_type, 1>
            {
                using type = sliding_array<t_value_type, 0>;
                using base_type = std::array<t_value_type, 1>;
                using value_type = t_value_type;

                using base_type::base_type;

                /** Number of the past observations stored. */
                constexpr std::size_t tail_size() const noexcept { return 0; }

                /** Adds another element to the observed sequence. */
                void push_back(const value_type& value) noexcept
                {
                    this->back() = value;
                } // observe(...)
            }; // struct sliding_array
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SLIDING_ARRAY_HPP_INCLUDED
