
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_NOISE_BASE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_NOISE_BASE_HPP_INCLUDED

#include "timed.hpp"

#include <cstddef>     // std::size_t
#include <type_traits> // std::is_same, std::enable_if_t, std::is_base_of, std::false_type, std::true_type

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Abstract CRTP structure for all noise types.
             *  @remark The inheriting type must friend the base class \c noise_base<...>.
             *  @remark The inheriting type must implement the following protected functions:
             *          void on_reset_override() noexcept
             *          t_value_type next_value(t_value_type current_value) noexcept
             */
            template <typename t_derived_type, typename t_value_type>
            struct noise_base : public timed<noise_base<t_derived_type, t_value_type>>
            {
                using type = noise_base<t_derived_type, t_value_type>;
                using base_type = timed<type>;
                friend base_type;

                using timed_type = typename base_type::timed_type;
                using noise_base_type = t_derived_type; // Type that this CRTP is templated on.
                using value_type = t_value_type;

            private:
                using derived_type = t_derived_type;

                value_type m_current_value = 0;

            protected:
                /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
                void on_reset_override() noexcept
                {
                    constexpr bool is_overwritten = !std::is_same<
                        decltype(&derived_type::on_reset_override),
                        decltype(&type::on_reset_override)>::value;
                    static_assert(is_overwritten, "static polymorphic function <on_reset_override> was not overwritten.");

                    derived_type* that = static_cast<derived_type*>(this);
                    that->on_reset_override();
                } // on_reset_override(...)

                /** @brief Updates the current value of the noise. */
                value_type next_value(value_type current_value) noexcept
                {
                    constexpr bool is_overwritten = !std::is_same<
                        decltype(&derived_type::next_value),
                        decltype(&type::next_value)>::value;
                    static_assert(is_overwritten, "static polymorphic function <next_value> was not overwritten.");

                    derived_type* that = static_cast<derived_type*>(this);
                    return that->next_value(current_value);
                } // next_value(...)

                /** @brief To be executed right before the \c reset() call. */
                void on_reset() noexcept
                {
                    this->on_reset_override();
                    this->m_current_value = 0;
                } // on_reset(...)

                /** @brief To be executed right after the \c tic() call. */
                void on_tic() noexcept
                {
                    this->m_current_value = this->next_value(this->m_current_value);
                } // on_tic(...)

            public:
                noise_base() noexcept { }

                /** Latest observed value. */
                value_type current_value() const noexcept { return this->m_current_value; }
            }; // struct noise_base
            
            template <typename t_value_type>
            struct noise_base<void, t_value_type> : public timed<noise_base<void, t_value_type>>
            {
                using type = noise_base<void, t_value_type>;
                using noise_base_type = void;
                using value_type = t_value_type;

            private:
                using base_type = timed<type>;
                friend timed<type>;

            protected:
                void on_reset() noexcept { }
                void on_tic() noexcept { }

            public:
                noise_base() noexcept { }
                constexpr value_type current_value() const noexcept { return 0; }
            }; // struct noise_base<...>

            template <typename t_value_type>
            using no_noise_t = noise_base<void, t_value_type>;
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_NOISE_BASE_HPP_INCLUDED
