
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED

#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include "timed.hpp"
#include "process.hpp"

#include <type_traits> // std::is_same

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                template <bool t_sync_check>
                struct sync_counter
                {
                    using type = sync_counter<t_sync_check>;

                private:
                    std::size_t m_count = 0; // Number of observations taken.

                public:
                    void reset() noexcept { this->m_count = 0; }

                    /* @brief Prefix increment operator. */
                    type& operator ++() noexcept
                    {
                        ++this->m_count;
                        return *this;
                    } // operator ++(...)

                    template <typename t_timed_type>
                    void sync(const timed<t_timed_type>& proc) const noexcept
                    {
                        if (proc.count() != this->m_count)
                            aftermath::quiet_error::instance().push(
                                aftermath::not_an_error::logic_error,
                                aftermath::severity_level::major,
                                "Synchronization failed.", __FUNCTION__, __LINE__);
                    }
                }; // struct sync_counter

                template <>
                struct sync_counter<false>
                {
                    using type = sync_counter<false>;

                    void reset() noexcept { }

                    /* @brief Prefix increment operator. */
                    type& operator ++() noexcept { return *this; }
                    
                    template <typename t_timed_type>
                    void sync(const timed<t_timed_type>& proc) const noexcept { }
                }; // struct sync_counter<...>
            } // namespace detail

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
            template <typename t_derived_type, typename t_signal_type, typename t_noise_type, bool t_sync_check = true>
            struct observer
            {
                using type = observer<t_derived_type, t_signal_type, t_noise_type, t_sync_check>;
                using observer_type = t_derived_type; // Type that this CRTP is templated on.

                using signal_type = t_signal_type;
                using noise_type = t_noise_type;
                using process_type = process<t_signal_type, t_noise_type>;
                using value_type = typename process_type::value_type;

            private:
                using derived_type = t_derived_type;

                detail::sync_counter<t_sync_check> m_counter = { };

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
                void on_tic(const process_type& proc) noexcept
                {
                    constexpr bool is_overwritten = !std::is_same<
                        decltype(&derived_type::on_tic),
                        decltype(&type::on_tic)>::value;
                    
                    if (!is_overwritten) return;
                    derived_type* that = static_cast<derived_type*>(this);
                    that->on_tic(proc);
                } // on_tic(...)

                /** @brief Auxiliary function to be executed right before the \c toc() call. */
                void on_toc(const process_type& proc) noexcept
                {
                    constexpr bool is_overwritten = !std::is_same<
                        decltype(&derived_type::on_toc),
                        decltype(&type::on_toc)>::value;
                    
                    if (!is_overwritten) return;
                    derived_type* that = static_cast<derived_type*>(this);
                    that->on_toc(proc);
                } // on_toc(...)

            public:
                /** @brief Resets the observer to its original state. */
                void reset() noexcept
                {
                    this->m_counter.reset();
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
                void tic(const process_type& proc) noexcept
                {
                    ++this->m_counter;
                    this->m_counter.sync(proc);
                    this->on_tic(proc);
                } // tic(...)

                /** @brief Completes one simulation (the sequence of observations from \p proc). */
                void toc(const process_type& proc) noexcept
                {
                    this->on_toc(proc);
                    this->m_counter.reset();
                } // toc(...)
            }; // struct attack_sequence
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED