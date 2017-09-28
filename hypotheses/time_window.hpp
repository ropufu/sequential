
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TIME_WINDOW_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TIME_WINDOW_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <deque>
#include <stdexcept>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** An auxiliary structure to discard old observations. */
            template <typename t_key_type>
            struct time_window
            {
                using type = time_window<t_key_type>;
                using key_type = t_key_type;

            private:
                std::size_t m_capacity; // Number of observations in history.
                std::size_t m_next_time_index = 0; // Time index of the next observation.
                std::deque<key_type> m_history; // History of observations.

            public:
                /** Initializes a new time window of a given width. */
                explicit time_window(std::size_t size) noexcept
                    : m_capacity(size), m_history()
                {
                }

                void reset() noexcept
                {
                    this->m_next_time_index = 0;
                    this->m_history.clear();
                }

                /** Size of the time window. */
                std::size_t size() const noexcept { return this->m_capacity; }

                /** Time index of the next observation. */
                std::size_t time() const noexcept { return this->m_next_time_index; }

                /** Adds another element to the observed sequence. */
                void observe(key_type value) noexcept
                {
                    this->m_history.push_front(value);
                    if (this->m_history.size() > this->m_capacity) this->m_history.pop_back();
                    this->m_next_time_index++;
                }

                /** Returns the latest observed value of the process. */
                double current_value() const noexcept { return this->m_history.front();  }

                /** @brief Retrieves the observation at a given time index.
                 *  @exception std::out_of_range \p time_index points to the future observations.
                 *  @exception std::logic_error \p time_index points to distant past, that element has been discarded.
                 */
                double operator [](std::size_t time_index) const
                {
                    if (this->m_next_time_index <= time_index) throw std::out_of_range("<time_index> points to the future observations.");
                    
                    std::size_t offset = this->m_next_time_index - 1 - time_index;
                    if (this->m_capacity <= offset) throw std::logic_error("<time_index> points to a discarded element.");
                    
                    return this->m_history[offset];
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TIME_WINDOW_HPP_INCLUDED
