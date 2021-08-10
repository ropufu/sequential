
#ifndef ROPUFU_SEQUENTIAL_INTERMITTENT_PROCESS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_INTERMITTENT_PROCESS_HPP_INCLUDED

namespace ropufu::sequential::intermittent
{
    template <typename t_value_type>
    struct process
    {
        using type = process<t_value_type>;
        using value_type = t_value_type;
        virtual value_type next() noexcept = 0;
    }; // struct process
} // namespace ropufu::sequential::intermittent

#endif // ROPUFU_SEQUENTIAL_INTERMITTENT_PROCESS_HPP_INCLUDED
