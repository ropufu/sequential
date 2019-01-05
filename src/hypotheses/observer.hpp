
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED

#include <ropufu/on_error.hpp> // aftermath::detail::on_error

#include "timed.hpp"
#include "process.hpp"

#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc
#include <type_traits> // std::is_same

namespace ropufu::sequential::hypotheses
{
    /** @brief Abstract class (CRTP) outlining the basic structure of an observer type.
     *  @remark The inheriting type must friend the base class \c observer<...>.
     *  @remark The inheriting type is required to implement the following public functions:
     *          bool is_listening() const noexcept
     *  @remark The inheriting type may implement the following public functions:
     *          void start_simulation() noexcept
     *          void stop_simulation() noexcept
     *  @remark The inheriting type may implement the following protected functions:
     *          void on_reset() noexcept
     *          void on_tic(const process<t_signal_type, t_noise_type>&) noexcept
     *          void on_toc(const process<t_signal_type, t_noise_type>&) noexcept
     */
    template <typename t_derived_type, typename t_signal_type, typename t_noise_type>
    struct observer
    {
        using type = observer<t_derived_type, t_signal_type, t_noise_type>;
        using observer_type = t_derived_type; // Type that this CRTP is templated on.

        using signal_type = t_signal_type;
        using noise_type = t_noise_type;
        using process_type = process<t_signal_type, t_noise_type>;
        using value_type = typename process_type::value_type;

    private:
        using derived_type = t_derived_type;

    protected:
        /** @brief Auxiliary function to be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_reset),
                decltype(&type::on_reset)>::value;

            if (!is_overwritten) return;
            derived_type* that = static_cast<derived_type*>(this);
            that->on_reset();
        } // on_reset(...)

        /** @brief Auxiliary function to be executed right after the \c tic() call. */
        void on_tic(const process_type& proc, std::error_code& ec) noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_tic),
                decltype(&type::on_tic)>::value;
            
            if (!is_overwritten) return;
            derived_type* that = static_cast<derived_type*>(this);
            that->on_tic(proc, ec);
        } // on_tic(...)

        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& proc, std::error_code& ec) noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_toc),
                decltype(&type::on_toc)>::value;
            
            if (!is_overwritten) return;
            derived_type* that = static_cast<derived_type*>(this);
            that->on_toc(proc, ec);
        } // on_toc(...)

    public:
        /** @brief Resets the observer to its original state. */
        void reset() noexcept
        {
            this->on_reset();
        } // reset(...)

        /** @brief Indicates whether the observer is still active. */
        bool is_listening() const noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::is_listening),
                decltype(&type::is_listening)>::value;
            static_assert(is_overwritten, "static polymorphic function <is_listening> was not overwritten.");

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
