
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED

#include <array>      // std::array
#include <string>     // std::string, std::to_string

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            // @todo Implement .mat var name function for <operating_characteristic> distinct from the existing std::to_string.
            enum struct operating_characteristic
            {
                unknown = 0,
                ess_under_null = 1,
                ess_under_alt = 2,
                probability_of_false_alarm = 3,
                probability_of_missed_signal = 4
            }; // enum struct operating_characteristic

            // @todo Make it a "proper" container.
            template <typename t_value_type>
            struct oc_array
            {
                using type = oc_array<t_value_type>;
                using value_type = t_value_type;

            private:
                std::array<t_value_type, 4> m_data = { };
                t_value_type m_invalid = { };

            public:
                oc_array() noexcept { }
                
                const t_value_type& operator [](operating_characteristic index) const noexcept
                {
                    std::size_t k = static_cast<std::size_t>(index);
                    if (k < 1 || k > 4) return this->m_invalid;
                    return this->m_data[k - 1];
                } // operator [](...)

                t_value_type& operator [](operating_characteristic index) noexcept
                {
                    std::size_t k = static_cast<std::size_t>(index);
                    if (k < 1 || k > 4) return this->m_invalid;
                    return this->m_data[k - 1];
                } // operator [](...)
            }; // struct oc_array
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

namespace std
{
    std::string to_string(ropufu::sequential::hypotheses::operating_characteristic x)
    {
        using argument_type = ropufu::sequential::hypotheses::operating_characteristic;
        switch (x)
        {
            case argument_type::ess_under_null: return "ESS[mu 0]";
            case argument_type::ess_under_alt: return "ESS[mu 1]";
            case argument_type::probability_of_false_alarm: return "PFA";
            case argument_type::probability_of_missed_signal: return "PMS";
            default: return "unknown";
        } // switch (...)
    }; // to_string(...)
} // namespace std

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED
