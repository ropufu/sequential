
#ifndef ROPUFU_SEQUENTIAL_INTERMITTENT_IID_PROCESS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_INTERMITTENT_IID_PROCESS_HPP_INCLUDED

#include "process.h"

#include <chrono>  // std::chrono::system_clock
#include <cstddef> // std::size_t
#include <random>  // std::seed_seq

namespace ropufu::sequential::intermittent
{
    template <typename t_engine_type, typename t_sampler_type>
    struct iid_process : public process<typename t_sampler_type::value_type>
    {
        using type = iid_process<t_engine_type, t_sampler_type>;
        using base_type = process<typename t_sampler_type::value_type>;
        using engine_type = t_engine_type;
        using sampler_type = t_sampler_type;

	    using distribution_type = typename sampler_type::distribution_type;
        using value_type = typename sampler_type::value_type;

    private:
        engine_type m_engine;
        sampler_type m_sampler;

        static engine_type make_engine() noexcept
        {
            engine_type result{};
            int time_seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
            std::seed_seq sequence{ 1, 1, 2, 3, 5, 8, 1729, time_seed };
            result.seed(sequence);
            return result;
        } // make_engine(...)

    public:
        explicit iid_process(const distribution_type& dist) noexcept
            : m_engine(type::make_engine()), m_sampler(dist)
        {
        } // process(...)

        value_type next() noexcept override
        {
            return this->m_sampler(this->m_engine);
        } // next(...)
    }; // struct iid_process
} // namespace ropufu::sequential::intermittent

#endif // ROPUFU_SEQUENTIAL_INTERMITTENT_IID_PROCESS_HPP_INCLUDED
