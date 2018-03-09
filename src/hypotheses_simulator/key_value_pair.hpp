
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_KEY_VALUE_PAIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_KEY_VALUE_PAIR_HPP_INCLUDED

#include <type_traits> // std::false_type

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                /** Auxiliary structure to help iterate through \c oc_array. */
                template <typename t_key_type, typename t_value_pointer_type>
                struct key_value_pair : public std::false_type { };

                /** Auxiliary structure to help iterate through \c oc_array. */
                template <typename t_key_type, typename t_value_type>
                struct key_value_pair<t_key_type, t_value_type*>
                {
                    using type = key_value_pair<t_key_type, t_value_type*>;
                    using key_type = t_key_type;
                    using value_type = t_value_type;

                private:
                    key_type m_key = { };
                    value_type* m_value_pointer = nullptr;

                public:
                    key_value_pair(key_type key, value_type* value_pointer) noexcept
                        : m_key(key), m_value_pointer(value_pointer)
                    {
                    } // key_value_pair(...)

                    key_type key() const noexcept { return this->m_key; }

                    value_type& value() noexcept { return *(this->m_value_pointer); }

                    const value_type& value() const noexcept { return *(this->m_value_pointer); }
                }; // struct key_value_pair<...>

                /** Auxiliary structure to help iterate through \c oc_array. */
                template <typename t_key_type, typename t_value_type>
                struct key_value_pair<t_key_type, const t_value_type*>
                {
                    using type = key_value_pair<t_key_type, const t_value_type*>;
                    using key_type = t_key_type;
                    using value_type = t_value_type;

                private:
                    key_type m_key = { };
                    const value_type* m_value_pointer = nullptr;

                public:
                    key_value_pair(key_type key, const value_type* value_pointer) noexcept
                        : m_key(key), m_value_pointer(value_pointer)
                    {
                    } // key_value_pair(...)

                    key_type key() const noexcept { return this->m_key; }

                    const value_type& value() const noexcept { return *(this->m_value_pointer); }
                }; // struct key_value_pair<...>
            } // namespace detail
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_KEY_VALUE_PAIR_HPP_INCLUDED
