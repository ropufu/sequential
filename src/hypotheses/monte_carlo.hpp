
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
        template <typename t_observer_type, typename t_on_start_type, typename t_on_stop_type>
        void run(process_type& proc, t_observer_type& observer, t_on_start_type&& on_start, t_on_stop_type&& on_stop, std::error_code& ec, std::size_t max_length = 1'000'000) noexcept
        {
            static_assert(is_observer_v<t_observer_type>, "t_observer_type has to be an observer.");
            if (this->m_count_simulations == 0) return;
            if (ec.value() != 0) return;

            // ~~ Clean up ~~
            proc.reset(); // Make sure the process starts from scratch.
            observer.reset(); // Make sure the observer doesn't have any lingering data.

            // ~~ Start ~~
            on_start(ec);
            if (ec.value() != 0) return; // Check for quiet errors.

            for (std::size_t i = 0; i < this->m_count_simulations; ++i)
            {
                bool is_listening = observer.is_listening();

                std::size_t t = 0;
                while (is_listening)
                {
                    proc.tic();
                    observer.tic(proc, ec);

                    is_listening = observer.is_listening();
                    if (ec.value() != 0) return; // Check for quiet errors.
                    
                    ++t; // Advance time index.
                    if (t == max_length)
                    {
                        aftermath::detail::on_error(ec, std::errc::function_not_supported, "Maximum run length exceeded.");
                        return;
                    } // if (...)
                } // while (...)
                observer.toc(proc, ec); // Finalize another experiment run.
                proc.reset();
                if (ec.value() != 0) return;
            } // for (...)

            // ~~ Stop ~~
            on_stop();
        } // run(...)
    }; // struct monte_carlo
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED
