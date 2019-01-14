
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_NOISE_BASE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_NOISE_BASE_HPP_INCLUDED

#include "type_traits.hpp"

#include <cstddef>     // std::size_t
#include <type_traits> // ...

namespace ropufu::sequential::hypotheses
{
    template <typename t_derived_type, typename t_value_type>
    struct noise_base;
    
    ROPUFU_SEQUENTIAL_HYPOTHESES_TYPE_TRAITS_CRTP(noise, noise_base, , typename core_type::value_type)

    /** @brief Base abstract structure for all noise types.
     *  @remark The inheriting type must friend the base class \c noise_base<...>.
     *  @remark The inheriting type must implement the following protected functions:
     *          on_reset() noexcept -> void
     *          next_value(t_value_type) noexcept -> t_value_type
     */
    template <typename t_derived_type, typename t_value_type>
    struct noise_base
    {
        using type = noise_base<t_derived_type, t_value_type>;
        using derived_type = t_derived_type;
        using core_type = t_derived_type;
        using value_type = t_value_type;

    private:
        value_type m_current_value = 0;

        static constexpr void traits_check() noexcept
        {
            // CRTP check.
            static_assert(std::is_base_of_v<type, derived_type>, "t_derived_type has to be derived from noise_base<t_derived_type>.");
        } // traits_check(...)

    protected:
        /** @brief Auxiliary function to be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            constexpr bool is_overwritten = !std::is_same_v<
                decltype(&derived_type::on_reset),
                decltype(&type::on_reset)>;
            static_assert(is_overwritten, "on_reset() noexcept -> void has not beed overloaded.");

            derived_type* that = static_cast<derived_type*>(this);
            that->on_reset();
        } // on_reset(...)

        /** @brief Updates the current value of the noise. */
        value_type next_value(value_type current_value) noexcept
        {
            constexpr bool is_overwritten = !std::is_same_v<
                decltype(&derived_type::next_value),
                decltype(&type::next_value)>;
            static_assert(is_overwritten, "next_value(t_value_type) noexcept -> t_value_type has not beed overloaded.");

            derived_type* that = static_cast<derived_type*>(this);
            return that->next_value(current_value);
        } // next_value(...)

    public:
        noise_base() noexcept { type::traits_check(); }

        core_type& as_noise() noexcept { return *static_cast<core_type*>(this); }
        const core_type& as_noise() const noexcept { return *static_cast<const core_type*>(this); }

        /** Latest observed value. */
        value_type current_value() const noexcept { return this->m_current_value; }

        /** @brief Resets the time to zero. */
        void reset() noexcept
        {
            this->on_reset();
            this->m_current_value = 0;
        } // reset(...)

        /** @brief Advances the time index by one unit. */
        void tic() noexcept
        {
            this->m_current_value = this->next_value(this->m_current_value);
        } // tic(...)
    }; // struct noise_base
    
    template <typename t_value_type>
    struct noise_base<void, t_value_type>
    {
        using type = noise_base<void, t_value_type>;
        using derived_type = void;
        using core_type = type;
        using value_type = t_value_type;

        noise_base() noexcept { }

        core_type& as_noise() noexcept { return *this; }
        const core_type& as_noise() const noexcept { return *this; }

        constexpr value_type current_value() const noexcept { return 0; }

        /** @brief Resets the time to zero. */
        void reset() noexcept { }

        /** @brief Advances the time index by one unit. */
        void tic() noexcept { }
    }; // struct noise_base<...>

    template <typename t_value_type>
    using no_noise_t = noise_base<void, t_value_type>;

    namespace detail
    {
        template <typename t_value_type>
        struct is_noise<no_noise_t<t_value_type>> : public std::true_type { };
    } // namespace detail
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_NOISE_BASE_HPP_INCLUDED
