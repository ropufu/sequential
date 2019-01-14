
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED

#include <ropufu/on_error.hpp> // aftermath::detail::on_error

#include "process.hpp"

#include "type_traits.hpp"

#include <stdexcept>    // std::runtime_error
#include <string>       // std::string
#include <system_error> // std::error_code, std::errc
#include <type_traits>  // ...

namespace ropufu::sequential::hypotheses
{
    template <typename t_derived_type, typename t_signal_type, typename t_noise_type>
    struct observer;

    ROPUFU_SEQUENTIAL_HYPOTHESES_TYPE_TRAITS_CRTP(observer, observer, , typename core_type::signal_type, typename core_type::noise_type)

    /** @brief Base abstract structure of a process observer type.
     *  @remark The inheriting type must friend the base class \c observer<...>.
     *  @remark The inheriting type is required to implement the following protected functions:
     *          on_reset() noexcept -> void
     *          on_tic(const process<t_signal_type, t_noise_type>&, std::error_code&) noexcept -> void
     *          on_toc(const process<t_signal_type, t_noise_type>&, std::error_code&) noexcept -> void
     *  @remark The inheriting type is required to implement the following public functions:
     *          is_listening() const noexcept -> bool
     */
    template <typename t_derived_type, typename t_signal_type, typename t_noise_type>
    struct observer
    {
        using type = observer<t_derived_type, t_signal_type, t_noise_type>;
        using derived_type = t_derived_type;
        using core_type = t_derived_type;
        using signal_type = t_signal_type;
        using noise_type = t_noise_type;
        using process_type = process<signal_type, noise_type>;
        using value_type = typename process_type::value_type;

    private:
        static constexpr void traits_check() noexcept
        {
            // CRTP check.
            static_assert(std::is_base_of_v<type, derived_type>, "t_derived_type has to be derived from observer<t_derived_type>.");
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
        void on_tic(const process_type& proc, std::error_code& ec) noexcept
        {
            constexpr bool is_overwritten = !std::is_same_v<
                decltype(&derived_type::on_tic),
                decltype(&type::on_tic)>;
            static_assert(is_overwritten, "on_tic(const process_type&, std::error_code&) noexcept -> void has not beed overloaded.");
            
            derived_type* that = static_cast<derived_type*>(this);
            that->on_tic(proc, ec);
        } // on_tic(...)

        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& proc, std::error_code& ec) noexcept
        {
            constexpr bool is_overwritten = !std::is_same_v<
                decltype(&derived_type::on_toc),
                decltype(&type::on_toc)>;
            static_assert(is_overwritten, "on_toc(const process_type&, std::error_code&) noexcept -> void has not beed overloaded.");
            
            derived_type* that = static_cast<derived_type*>(this);
            that->on_toc(proc, ec);
        } // on_toc(...)

    public:
        observer() noexcept { type::traits_check(); }

        core_type& as_observer() noexcept { return *static_cast<core_type*>(this); }
        const core_type& as_observer() const noexcept { return *static_cast<const core_type*>(this); }

        /** @brief Resets the observer to its original state. */
        void reset() noexcept
        {
            this->on_reset();
        } // reset(...)

        /** @brief Indicates whether the observer is still active. */
        bool is_listening() const noexcept
        {
            constexpr bool is_overwritten = !std::is_same_v<
                decltype(&derived_type::is_listening),
                decltype(&type::is_listening)>;
            static_assert(is_overwritten, "is_listening() const noexcept -> bool has not beed overloaded.");

            const derived_type* that = static_cast<const derived_type*>(this);
            return that->is_listening();
        } // is_listening(...)

        /** @brief Listens to new observations from \p proc. */
        void tic(const process_type& proc, std::error_code& ec) noexcept
        {
            this->on_tic(proc, ec);
        } // tic(...)

        /** @brief Completes one simulation (the sequence of observations from \p proc). */
        void toc(const process_type& proc, std::error_code& ec) noexcept
        {
            this->on_toc(proc, ec);
        } // toc(...)
    }; // struct observer
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED
