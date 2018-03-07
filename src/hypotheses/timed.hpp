
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TIMED_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TIMED_HPP_INCLUDED

#include <cstddef>     // std::size_t
#include <type_traits> // std::is_same

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Virtual class (CRTP) outlining the basic structure of a timed process type.
             *  @remark The inheriting type must friend the base class \c timed<...>.
             *  @remark The inheriting type may implement the following protected functions:
             *          void on_reset() noexcept
             *          void on_tic() noexcept
             */
            template <typename t_derived_type>
            struct timed
            {
                using type = timed<t_derived_type>;
                using timed_type = t_derived_type; // Type that this CRTP is templated on.

            private:
                using derived_type = t_derived_type;

                std::size_t m_count = 0; // Number of observations taken.
                std::size_t m_time = 0; // Current time index.

            protected:
                /** @brief Auxiliary function to be executed right before the \c reset() call. */
                void on_reset() noexcept
                {
                    constexpr bool is_overwritten = !std::is_same<
                        decltype(&derived_type::on_reset),
                        decltype(&type::on_reset)>::value;

                    if (!is_overwritten) return;
                    derived_type* that = static_cast<derived_type*>(this);
                    that->on_reset();
                } // on_reset(...)

                /** @brief Auxiliary function to be executed right after the \c tic() call. */
                void on_tic() noexcept
                {
                    constexpr bool is_overwritten = !std::is_same<
                        decltype(&derived_type::on_tic),
                        decltype(&type::on_tic)>::value;
                    
                    if (!is_overwritten) return;
                    derived_type* that = static_cast<derived_type*>(this);
                    that->on_tic();
                } // on_tic(...)

            public:
                /** @brief The number of tics up to this moment. */
                std::size_t count() const noexcept { return this->m_count; }

                /** @brief The number of tics up to this moment. */
                std::size_t time() const noexcept { return this->m_time; }

                /** Indicates if any observations have been made. */
                bool empty() const noexcept { return this->m_count == 0; }

                /** @brief Resets the time to zero. */
                void reset() noexcept
                {
                    this->on_reset();
                    this->m_count = 0;
                    this->m_time = 0;
                } // reset(...)

                /** @brief Advances the time index by one unit. */
                void tic() noexcept
                {
                    ++this->m_count;
                    this->m_time = this->m_count - 1;
                    this->on_tic();
                } // tic(...)
            }; // struct timed
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TIMED_HPP_INCLUDED
