
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED

#include "model.hpp"
#include "likelihood.hpp"
#include "simple_process.hpp"
#include "change_of_measure.hpp"
#include "observer.hpp"

#include <cstddef>     // std::size_t
#include <type_traits> // std::is_convertible_v
#include <vector>      // std::vector

namespace ropufu::sequential::hypotheses
{
    /** Structure responsible for simulations. */
    template <typename t_engine_type, typename t_value_type>
    struct monte_carlo
    {
        using type = monte_carlo<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using likelihood_type = likelihood<value_type>;
        using model_type = model<t_value_type>;
        using process_type = simple_process<engine_type, value_type>;
        using observer_type = observer<engine_type, value_type>;
        using change_of_measure_type = change_of_measure<value_type>;

    private:
        std::size_t m_count_simulations = 0;

    public:
        monte_carlo() noexcept { }

        explicit monte_carlo(std::size_t count_simulations) noexcept
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
         *  - Calls \p on_stop().
         *  @param on_start Callback that will initialize the \p rules.
         *  @param on_stop Callback that will collect the information from the \p rules.
         */
        template <typename t_observer_collection_type>
        void run(engine_type& engine, process_type& proc, const model_type& model, const change_of_measure_type& signal_strength,
            t_observer_collection_type& observer_pointers, std::size_t max_length = 1'000'000)
        {
            static_assert(
                std::is_convertible_v<decltype(**observer_pointers.begin()), observer_type&>,
                "Elements of the collection must be dereferencable to observer references.");
            if (this->m_count_simulations == 0) return;
            likelihood_type likelihood {model};

            // ~~ Set up ~~
            proc.set_signal_strength(signal_strength.simulated());
			// ~~ Clean up ~~
            for (auto& o : observer_pointers) o->clean_up();

            for (std::size_t i = 0; i < this->m_count_simulations; ++i)
            {
                proc.reset();
                for (auto& o : observer_pointers) o->reset();

                bool is_listening = observer_type::any_listening(observer_pointers);
                while (is_listening)
                {
                    proc.tic(engine);
                    likelihood.tic(proc);
                    for (auto& o : observer_pointers) o->tic(proc, likelihood);

                    is_listening = observer_type::any_listening(observer_pointers);
                    if (proc.time() == max_length) throw std::length_error("Maximum run length exceeded.");
                } // while (...)
                for (auto& o : observer_pointers) o->toc(proc, likelihood, signal_strength); // Finalize another experiment run.
            } // for (...)
        } // run(...)
    }; // struct monte_carlo
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MONTE_CARLO_HPP_INCLUDED
