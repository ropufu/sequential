
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TIMED_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TIMED_HPP_INCLUDED

#include "type_traits.hpp"

#include <cstddef>     // std::size_t
#include <type_traits> // ...

namespace ropufu::sequential::hypotheses
{
    template <typename t_derived_type>
    struct timed;
    
    ROPUFU_SEQUENTIAL_HYPOTHESES_TYPE_TRAITS_CRTP(timed, timed)

    /** @brief Base abstract structure of a timed process type.
     *  @remark The inheriting type must friend the base class \c timed<...>.
     *  @remark The inheriting type must implement the following protected functions:
     *          on_reset() noexcept -> void
     *          on_tic() noexcept -> void
     *  @remark \c time() and \c count() are connected as follows:
     *          ----0----1----2----3----4----...----> # count (# of observations)
     *          ----0----0----1----2----3----...----> # time
     *              ^
     *          initial state
     */
    template <typename t_derived_type>
    struct timed
    {
        using type = timed<t_derived_type>;
        using derived_type = t_derived_type;
        using core_type = t_derived_type;

    private:
        std::size_t m_count = 0; // Number of observations taken.
        std::size_t m_time = 0; // Current time index.

        static constexpr void traits_check() noexcept
        {
            // CRTP check.
            static_assert(std::is_base_of_v<type, derived_type>, "t_derived_type has to be derived from timed<t_derived_type>.");
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

        /** @brief Auxiliary function to be executed right after the \c tic() call. */
        void on_tic() noexcept
        {
            constexpr bool is_overwritten = !std::is_same_v<
                decltype(&derived_type::on_tic),
                decltype(&type::on_tic)>;
            static_assert(is_overwritten, "on_tic() noexcept -> void has not beed overloaded.");
            
            derived_type* that = static_cast<derived_type*>(this);
            that->on_tic();
        } // on_tic(...)

    public:
        timed() noexcept { type::traits_check(); }

        core_type& as_timed() noexcept { return *static_cast<core_type*>(this); }
        const core_type& as_timed() const noexcept { return *static_cast<const core_type*>(this); }

        /** @brief The number of tics up to this moment. */
        std::size_t count() const noexcept { return this->m_count; }

        /** @brief The number of tics up to this moment. */
        std::size_t time() const noexcept { return this->m_time; }

        /** Indicates if any observations have been made. */
        bool empty() const noexcept { return this->m_count == 0; }

        /** @brief Resets the time to zero. */
        void reset() noexcept
        {
            this->on_reset();
            this->m_count = 0;
            this->m_time = 0;
        } // reset(...)

        /** @brief Advances the time index by one unit. */
        void tic() noexcept
        {
            ++this->m_count;
            this->m_time = this->m_count - 1;
            this->on_tic();
        } // tic(...)
    }; // struct timed
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TIMED_HPP_INCLUDED
