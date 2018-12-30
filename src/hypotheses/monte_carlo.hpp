
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED

#include "observer.hpp"
#include "process.hpp"
#include "rules.hpp"

#include <ropufu/on_error.hpp> // aftermath::detail::on_error

#include <cstddef> // std::size_t, std::nullptr_t
#include <type_traits>  // std::enable_if_t, std::is_base_of
#include <vector>  // std::vector

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        template <typename t_container_type>
        struct observer_container
        {
            using type = observer_container<t_container_type>;
            using observer_type = typename t_container_type::value_type;
            using container_type = t_container_type;
            using process_type = typename observer_type::process_type;

            static void reset(container_type& rules) noexcept { for (observer_type& item : rules) item.reset(); }
            
            static void tic(container_type& rules, const process_type& proc, std::error_code& ec) noexcept { for (observer_type& item : rules) item.tic(proc, ec); }
            static void toc(container_type& rules, const process_type& proc, std::error_code& ec) noexcept { for (observer_type& item : rules) item.toc(proc, ec); }

            static bool is_listening(const container_type& rules) noexcept
            {
                bool is_listening = false;
                for (const observer_type& item : rules) is_listening |= item.is_listening(); // Keep on going as long as at least one observer is listening.
                return is_listening;
            } // is_listening(...)
        }; // struct observer_container

        // template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
        // struct observer_container<std::vector<xsprt<t_signal_type, t_noise_type, t_sync_check>>>
        // {
        //     using type = observer_container<std::vector<xsprt<t_signal_type, t_noise_type, t_sync_check>>>;
        //     using observer_type = xsprt<t_signal_type, t_noise_type, t_sync_check>;
        //     using container_type = std::vector<xsprt<t_signal_type, t_noise_type, t_sync_check>>;
        //     using process_type = typename observer_type::process_type;

        //     static void reset(container_type& rules) noexcept { for (observer_type& item : rules) item.reset(); }
            
        //     static void tic(container_type& rules, const process_type& proc) noexcept { for (observer_type& item : rules) item.tic(proc); }
        //     static void toc(container_type& rules, const process_type& proc) noexcept { for (observer_type& item : rules) item.toc(proc); }

        //     static bool is_listening(const container_type& rules) noexcept
        //     {
        //         bool is_listening = false;
        //         for (const observer_type& item : rules) is_listening |= item.is_listening(); // Keep on going as long as at least one observer is listening.
        //         return is_listening;
        //     } // is_listening(...)
        // }; // struct observer_container<...>
    } // namespace detail
    
    /** Structure responsible for simulations. */
    template <typename t_signal_type, typename t_noise_type>
    struct monte_carlo;

    template <typename t_process_type>
    using monte_carlo_t = monte_carlo<typename t_process_type::signal_type, typename t_process_type::noise_type>;

    /** Structure responsible for simulations. */
    template <typename t_signal_type, typename t_noise_type>
    struct monte_carlo
    {
        using type = monte_carlo<t_signal_type, t_noise_type>;

        using signal_type = t_signal_type;
        using noise_type = t_noise_type;
        using process_type = process<t_signal_type, t_noise_type>;
        using value_type = typename process_type::value_type;

    private:
        std::size_t m_count_simulations = 0;

    public:
        monte_carlo() noexcept { }

        monte_carlo(std::size_t count_simulations) noexcept
            : m_count_simulations(count_simulations)
        {
            
        } // monte_carlo(...)

        std::size_t count_simulations() const noexcept { return this->m_count_simulations; }

        /** @brief Runs a process simulation while at least one of \p proc is listening.
         *  Performs the following:
         *  - Resets \p proc.
         *  - Resets all \p rules.
         *  - Calls \p on_start().
         *  - While at least one of \p proc is listening:
         *      -- Calls \c tic() on \p proc.
         *      -- Calls \c tic(proc) on each of \p rules.
         *  - Calls \c toc(proc) on each of \p rules.
         *  - Calls \c reset() on \p proc.
         *  - Calls \p on_stop().
         *  @param on_start Callback that will initialize the \p rules.
         *  @param on_stop Callback that will collect the information from the \p rules.
         */
        template <typename t_container_type, typename t_on_start_type, typename t_on_stop_type>
        void run(process_type& proc, t_container_type& rules, t_on_start_type&& on_start, t_on_stop_type&& on_stop, std::error_code& ec, std::size_t max_length = 1'000'000) noexcept
        {
            using helper_type = detail::observer_container<t_container_type>;

            if (this->m_count_simulations == 0) return;
            if (ec.value() != 0) return;

            // ~~ Clean up ~~
            proc.reset(); // Make sure the process starts from scratch.
            helper_type::reset(rules); // Make sure the observer doesn't have any lingering data.

            // ~~ Start ~~
            on_start(ec);
            if (ec.value() != 0) return; // Check for quiet errors.

            for (std::size_t i = 0; i < this->m_count_simulations; ++i)
            {
                bool is_listening = helper_type::is_listening(rules);

                std::size_t t = 0;
                while (is_listening)
                {
                    proc.tic();
                    helper_type::tic(rules, proc, ec);

                    is_listening = helper_type::is_listening(rules);
                    if (ec.value() != 0) return; // Check for quiet errors.
                    
                    ++t; // Advance time index.
                    if (t == max_length)
                    {
                        aftermath::detail::on_error(ec, std::errc::function_not_supported, "Maximum run length exceeded.");
                        return;
                    } // if (...)
                } // while (...)
                helper_type::toc(rules, proc, ec); // Finalize another experiment run.
                proc.reset();
                if (ec.value() != 0) return;
            } // for (...)

            // ~~ Stop ~~
            on_stop();
        } // run(...)
    }; // struct monte_carlo
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED
