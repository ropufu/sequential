
#ifndef ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_SIMULATOR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_SIMULATOR_HPP_INCLUDED

#include <ropufu/probability/standard_normal_distribution.hpp>
#include <ropufu/random/standard_normal_sampler_512.hpp>
#include <ropufu/sequential/iid_process.hpp>

#include "model.hpp"
#include "xsprt.hpp"

#include <concepts>    // std::floating_point
#include <cstddef>     // std::size_t
#include <random>      // std::seed_seq

namespace ropufu::sequential::gaussian_mean_hypotheses
{
    template <std::floating_point t_value_type, typename t_engine_type>
    struct simulator
    {
        using type = simulator<t_value_type, t_engine_type>;
        using value_type = t_value_type;
        using engine_type = t_engine_type;

        using sampler_type = ropufu::aftermath::random::standard_normal_sampler_512<engine_type, value_type>;
        using process_type = ropufu::aftermath::sequential::iid_process<sampler_type>;
        using statistic_type = xsprt<value_type>;
        using output_type = typename statistic_type::output_type;

        static constexpr std::size_t block_size = 100;

    private:
        process_type m_noise = {};
        statistic_type m_statistic = {};

    public:
        simulator() noexcept = default;

        explicit simulator(const statistic_type& statistic) noexcept
            : m_statistic(statistic)
        {
        } // simulator(...)

        void seed(std::seed_seq& sequence) noexcept
        {
            this->m_noise.seed(sequence);
        } // seed(...)

        output_type operator ()() noexcept
        {
            using model_type = typename statistic_type::model_type;
            using observation_container_type = typename process_type::container_type;

            const model_type& model = this->m_statistic.model();
            value_type signal_strength = this->m_statistic.simulated_signal_strength();
            
            this->m_noise.clear(); // Reset driving process.
            this->m_statistic.reset(); // Reset the statistic.

            // Pre-allocate observations block.
            observation_container_type block = observation_container_type(type::block_size);
            while (this->m_statistic.is_running())
            {
                std::size_t time = this->m_noise.count();
                // Generate new signal + noise values.
                this->m_noise.next(block);
                for (value_type& x : block) x += signal_strength * model.signal_at(++time);
                // Update stopping times.
                for (value_type& x : block) this->m_statistic.observe(x);
            } // while (...)
            
            return this->m_statistic.output();
        } // operator ()(...)
    }; // struct simulator
} // namespace ropufu::sequential::gaussian_mean_hypotheses

#endif // ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_SIMULATOR_HPP_INCLUDED
