
#ifndef ROPUFU_SEQUENTIAL_INTERMITTENT_LLR_TRANSFORM_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_INTERMITTENT_LLR_TRANSFORM_HPP_INCLUDED

namespace ropufu::sequential::intermittent
{
    template <typename t_value_type>
    struct llr_transform
    {
        using type = llr_transform<t_value_type>;
        using value_type = t_value_type;

        value_type scale;
        value_type shift;

        value_type operator ()(value_type raw_value) const noexcept
        {
            raw_value *= this->scale;
            raw_value += this->shift;
            return raw_value;
        } // operator ()(...)
    }; // struct llr_transform
} // namespace ropufu::sequential::intermittent

#endif // ROPUFU_SEQUENTIAL_INTERMITTENT_LLR_TRANSFORM_HPP_INCLUDED
