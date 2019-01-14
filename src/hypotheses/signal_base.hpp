
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNAL_BASE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNAL_BASE_HPP_INCLUDED

#include "type_traits.hpp"

#include <cstddef>     // std::size_t
#include <type_traits> // ...

namespace ropufu::sequential::hypotheses
{
    template <typename t_derived_type, typename t_value_type>
    struct signal_base;
    
    ROPUFU_SEQUENTIAL_HYPOTHESES_TYPE_TRAITS_CRTP(signal, signal_base, , typename core_type::value_type)

    /** @brief Base abstract structure for all noise types.
     *  @remark The inheriting type must friend the base class \c signal_base<...>.
     *  @remark The inheriting type must implement the following public functions:
     *          at(std::size_t) const noexcept -> t_value_type
     */
    template <typename t_derived_type, typename t_value_type>
    struct signal_base
    {
        using type = signal_base<t_derived_type, t_value_type>;
        using derived_type = t_derived_type;
        using core_type = t_derived_type;
        using value_type = t_value_type;

    private:
        static constexpr void traits_check() noexcept
        {
            // CRTP check.
            static_assert(std::is_base_of_v<type, derived_type>, "t_derived_type has to be derived from signal_base<t_derived_type>.");
        } // traits_check(...)

    public:
        signal_base() noexcept { type::traits_check(); }

        core_type& as_signal() noexcept { return *static_cast<core_type*>(this); }
        const core_type& as_signal() const noexcept { return *static_cast<const core_type*>(this); }

        /** @brief Signal value at an arbitrary time. */
        value_type at(std::size_t time_index) const noexcept
        {
            constexpr bool is_overwritten = !std::is_same_v<
                decltype(&derived_type::at),
                decltype(&type::at)>;
            static_assert(is_overwritten, "at() const noexcept -> t_value_type has not beed overloaded.");

            const derived_type* that = static_cast<const derived_type*>(this);
            return that->at(time_index);
        } // at(...)

        /** @brief Signal value at an arbitrary time. */
        value_type operator ()(std::size_t time_index) const noexcept { return this->at(time_index); }
        
        /** @brief Signal value at an arbitrary time. */
        value_type operator [](std::size_t time_index) const noexcept { return this->at(time_index); }
    }; // struct signal_base
    
    template <typename t_value_type>
    struct signal_base<void, t_value_type>
    {
        using type = signal_base<void, t_value_type>;
        using derived_type = void;
        using core_type = type;
        using value_type = t_value_type;

        signal_base() noexcept { }

        core_type& as_signal() noexcept { return *this; }
        const core_type& as_signal() const noexcept { return *this; }

        /** @brief Signal value at an arbitrary time. */
        constexpr value_type at(std::size_t /*time_index*/) const noexcept { return 0; }

        /** @brief Signal value at an arbitrary time. */
        constexpr value_type operator ()(std::size_t /*time_index*/) const noexcept { return 0; }

        /** @brief Signal value at an arbitrary time. */
        constexpr value_type operator [](std::size_t /*time_index*/) const noexcept { return 0; }
    }; // struct signal_base<...>

    template <typename t_value_type>
    using no_signal_t = signal_base<void, t_value_type>;

    namespace detail
    {
        template <typename t_value_type>
        struct is_signal<no_signal_t<t_value_type>> : public std::true_type { };
    } // namespace detail
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNAL_BASE_HPP_INCLUDED
