
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNAL_BASE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNAL_BASE_HPP_INCLUDED

#include <cstddef>     // std::size_t
#include <type_traits> // std::is_same, std::enable_if_t, std::is_base_of, std::false_type, std::true_type

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Abstract CRTP structure for all noise types.
             *  @remark The inheriting type must friend the base class \c signal_base<...>.
             *  @remark The inheriting type must implement the following public functions:
             *          t_value_type at(std::size_t) const noexcept
             */
            template <typename t_derived_type, typename t_value_type>
            struct signal_base
            {
                using type = signal_base<t_derived_type, t_value_type>;
                using signal_base_type = t_derived_type; // Type that this CRTP is templated on.
                using value_type = t_value_type;

            private:
                using derived_type = t_derived_type;

            public:
                signal_base() noexcept { }

                /** @brief Signal value at an arbitrary time. */
                value_type at(std::size_t time_index) const noexcept
                {
                    constexpr bool is_overwritten = !std::is_same<
                        decltype(&derived_type::at),
                        decltype(&type::at)>::value;
                    static_assert(is_overwritten, "static polymorphic function <at> was not overwritten.");

                    const derived_type* that = static_cast<const derived_type*>(this);
                    return that->at(time_index);
                } // at(...)

                /** @brief Signal value at an arbitrary time. */
                value_type operator ()(std::size_t time_index) const noexcept { return this->at(time_index); }
                
                /** @brief Signal value at an arbitrary time. */
                value_type operator [](std::size_t time_index) const noexcept {return this->at(time_index); }
            }; // struct signal_base
            
            template <typename t_value_type>
            struct signal_base<void, t_value_type>
            {
                using type = signal_base<void, t_value_type>;
                using signal_base_type = void;
                using value_type = t_value_type;

                signal_base() noexcept { }
                constexpr value_type at(std::size_t time_index) const noexcept { return 0; }
                constexpr value_type operator ()(std::size_t time_index) const noexcept { return 0; }
                constexpr value_type operator [](std::size_t time_index) const noexcept { return 0; }
            }; // struct signal_base<...>

            template <typename t_value_type>
            using no_signal_t = signal_base<void, t_value_type>;
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIGNAL_BASE_HPP_INCLUDED
