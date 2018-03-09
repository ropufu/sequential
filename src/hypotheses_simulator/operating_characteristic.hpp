
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED

#include "key_value_pair.hpp"

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::int_fast32_t
#include <string>  // std::string, std::to_string
#include <type_traits> // std::underlying_type_t

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            enum struct operating_characteristic : std::int_fast32_t
            {
                unknown = 0,
                ess_under_null = 1,
                ess_under_alt = 2,
                probability_of_false_alarm = 3,
                probability_of_missed_signal = 4
            }; // enum struct operating_characteristic

            namespace detail
            {
                static constexpr std::int_fast32_t oc_array_from = 1;
                static constexpr std::int_fast32_t oc_array_to = 4;
                static constexpr std::int_fast32_t oc_capacity = 1 + oc_array_to - oc_array_from;

                bool mat_var_name(operating_characteristic oc, std::string& expected_value, std::string& variance) noexcept
                {
                    switch (oc)
                    {
                        case operating_characteristic::ess_under_null:
                            expected_value = "ess_null"; variance = "vss_null";
                            return true;
                        case operating_characteristic::ess_under_alt:
                            expected_value = "ess_alt"; variance = "vss_alt";
                            return true;
                        case operating_characteristic::probability_of_false_alarm:
                            expected_value = "pfa"; variance = "vfa";
                            return true;
                        case operating_characteristic::probability_of_missed_signal:
                            expected_value = "pms"; variance = "vms";
                            return true;
                        default: return false;
                    } // switch (...)
                } // mat_var_name
            } // namespace detail
        
            /** An iterator for \c enum_array to allow it to be used in range-based for loops. */
            template <typename t_value_type, typename t_value_pointer_type>
            struct oc_array_iterator
            {
                static constexpr std::size_t capacity = detail::oc_capacity;
                using type = oc_array_iterator<t_value_type, t_value_pointer_type>;
                using enum_type = operating_characteristic;
                using value_type = t_value_type;
                using value_pointer_type = t_value_pointer_type;

                using underlying_type = std::underlying_type_t<enum_type>;
                using iterator_result_type = detail::key_value_pair<enum_type, value_pointer_type>;

            private:
                value_pointer_type m_raw_data = nullptr;
                underlying_type m_position = 0;

            public:
                oc_array_iterator(value_pointer_type collection_data_pointer, underlying_type position) noexcept
                    : m_raw_data(collection_data_pointer), m_position(position)
                {
                } // oc_array_iterator(...)

                /** Termination condition. */
                bool operator !=(const type& other) const noexcept { return this->m_position != other.m_position; }
                /** Equality operator. */
                bool operator ==(const type& other) const noexcept { return this->m_position == other.m_position; }

                /** Returns the current enum key/value pair. Behavior undefined if iterator has reached the end of the collection. */
                iterator_result_type operator *() noexcept
                {
                    return iterator_result_type(
                        static_cast<enum_type>(detail::oc_array_from + this->m_position),
                        this->m_raw_data + this->m_position);
                } // operator *(...)

                /** If not at the end, advances the position of the iterator by one. */
                type& operator ++() noexcept
                {
                    if (this->m_position == type::capacity) return *this;
                    ++(this->m_position);
                    return *this;
                } // operator ++(...)
            }; // struct oc_array_iterator

            /** @brief Array indexed by \c operating_characteristic. */
            template <typename t_value_type>
            struct oc_array
            {
                static constexpr std::size_t capacity = detail::oc_capacity;
                using type = oc_array<t_value_type>;
                using value_type = t_value_type;

                using collection_type = std::array<t_value_type, detail::oc_capacity>;
                using underlying_type = std::underlying_type_t<operating_characteristic>;
                using iterator_type = oc_array_iterator<value_type, value_type*>;
                using const_iterator_type = oc_array_iterator<value_type, const value_type*>;

            private:
                collection_type m_collection = { };
                value_type m_invalid = { };

                value_type& unchecked_at(underlying_type k) noexcept { return this->m_collection[k - detail::oc_array_from]; }
                const value_type& unchecked_at(underlying_type k) const noexcept { return this->m_collection[k - detail::oc_array_from]; }

            public:
                oc_array() noexcept { }

                constexpr std::size_t size() const noexcept { return detail::oc_capacity; }
                constexpr std::size_t max_size() const noexcept { return detail::oc_capacity; }

                constexpr bool empty() const noexcept { return detail::oc_capacity == 0; }

                void fill(const value_type& value) noexcept { return this->m_collection.fill(value); }

                const value_type& at(operating_characteristic index) const noexcept
                {
                    underlying_type k = static_cast<underlying_type>(index);
                    if (k < detail::oc_array_from || k > detail::oc_array_to) return this->m_invalid;
                    return this->unchecked_at(k);
                } // at(...)

                value_type& at(operating_characteristic index) noexcept
                {
                    underlying_type k = static_cast<underlying_type>(index);
                    if (k < detail::oc_array_from || k > detail::oc_array_to) return this->m_invalid;
                    return this->unchecked_at(k);
                } // at(...)
                
                const value_type& operator [](operating_characteristic index) const noexcept { return this->unchecked_at(static_cast<underlying_type>(index)); }
                value_type& operator [](operating_characteristic index) noexcept { return this->unchecked_at(static_cast<underlying_type>(index)); }

                const_iterator_type cbegin() const noexcept { return const_iterator_type(this->m_collection.data(), 0); }
                const_iterator_type cend() const noexcept { return const_iterator_type(this->m_collection.data(), type::capacity); }

                const_iterator_type begin() const noexcept { return const_iterator_type(this->m_collection.data(), 0); }
                const_iterator_type end() const noexcept { return const_iterator_type(this->m_collection.data(), type::capacity); }

                iterator_type begin() noexcept { return iterator_type(this->m_collection.data(), 0); }
                iterator_type end() noexcept { return iterator_type(this->m_collection.data(), type::capacity); }

                bool operator !=(const type& other) const noexcept { return this->m_collection != other.m_collection; }
                bool operator ==(const type& other) const noexcept { return this->m_collection == other.m_collection; }
            }; // struct oc_array

            /** @brief List \c operating_characteristic. */
            template <>
            struct oc_array<void>
            {
                static constexpr std::size_t capacity = detail::oc_capacity;
                using type = oc_array<void>;
                using value_type = operating_characteristic;

                using collection_type = std::array<operating_characteristic, detail::oc_capacity>;
                using underlying_type = std::underlying_type_t<operating_characteristic>;
                using const_iterator_type = typename collection_type::const_iterator;

            private:
                collection_type m_collection = { };

                oc_array(bool) noexcept
                {
                    for (underlying_type i = 0; i < detail::oc_capacity; ++i)
                        this->m_collection[i] = static_cast<operating_characteristic>(i + detail::oc_array_from);
                } // oc_array(...)

            public:
                oc_array() noexcept
                {
                    static type s_instance(true);
                    this->m_collection = s_instance.m_collection;
                } // oc_array(...)

                constexpr std::size_t size() const noexcept { return detail::oc_capacity; }
                constexpr std::size_t max_size() const noexcept { return detail::oc_capacity; }

                constexpr bool empty() const noexcept { return detail::oc_capacity == 0; }

                const value_type& at(std::size_t index) const noexcept { return this->m_collection.at(index); }

                const value_type& operator [](std::size_t index) const noexcept { return this->m_collection[index]; }

                const_iterator_type cbegin() const noexcept { return this->m_collection.cbegin(); }
                const_iterator_type cend() const noexcept { return this->m_collection.cend(); }

                const_iterator_type begin() noexcept { return this->m_collection.cbegin(); }
                const_iterator_type end() noexcept { return this->m_collection.cend(); }

                bool operator !=(const type& other) const noexcept { return this->m_collection != other.m_collection; }
                bool operator ==(const type& other) const noexcept { return this->m_collection == other.m_collection; }
            }; // struct oc_array<...>
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
            case argument_type::ess_under_null: return "ess_null";
            case argument_type::ess_under_alt: return "ess_alt";
            case argument_type::probability_of_false_alarm: return "pfa";
            case argument_type::probability_of_missed_signal: return "pms";
            default: return "unknown";
        } // switch (...)
    }; // to_string(...)
} // namespace std

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_OPERATING_CHARACTERISTIC_HPP_INCLUDED
