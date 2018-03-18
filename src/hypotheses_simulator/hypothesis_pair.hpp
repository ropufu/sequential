
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../draft/quiet_json.hpp"

#include <aftermath/not_an_error.hpp> // quiet_error, not_an_error, severity_level

#include <iostream> // std::ostream

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Describes a pair associated with a null-alternative hypotheses pair. */
            template <typename t_value_type>
            struct hypothesis_pair
            {
                using type = hypothesis_pair<t_value_type>;
                using value_type = t_value_type;

                // ~~ Json names ~~
                static constexpr char jstr_null[] = "null";
                static constexpr char jstr_alt[] = "alt";

            private:
                value_type m_null = { }; // Null hypothesis value.
                value_type m_alt = { }; // Alternative hypothesis value.

            public:
                hypothesis_pair() noexcept { }

                hypothesis_pair(const value_type& null_value, const value_type& alt_value) noexcept
                    : m_null(null_value), m_alt(alt_value)
                {
                } // hypothesis_pair(...)

                /** @brief Signal "strength" under the null hypothesis. */
                const value_type& null() const noexcept { return this->m_null; }
                /** @brief Minimal signal "strength" under the alternative hypothesis. */
                const value_type& alt() const noexcept { return this->m_alt; }

                /** Checks if the two objects are equal. */
                bool operator ==(const type& other) const noexcept
                {
                    return
                        this->m_null == other.m_null &&
                        this->m_alt == other.m_alt;
                } // operator ==(...)

                /** Checks if the two objects are not equal. */
                bool operator !=(const type& other) const noexcept { return !this->operator ==(other); }

                /** @brief Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct hypothesis_pair

            // ~~ Json name definitions ~~
            template <typename t_value_type> constexpr char hypothesis_pair<t_value_type>::jstr_null[];
            template <typename t_value_type> constexpr char hypothesis_pair<t_value_type>::jstr_alt[];
            
            template <typename t_value_type>
            void to_json(nlohmann::json& j, const hypothesis_pair<t_value_type>& x) noexcept
            {
                using type = hypothesis_pair<t_value_type>;

                j = nlohmann::json{
                    {type::jstr_null, x.null()},
                    {type::jstr_alt, x.alt()}
                };
            } // to_json(...)
        
            template <typename t_value_type>
            void from_json(const nlohmann::json& j, hypothesis_pair<t_value_type>& x) noexcept
            {
                quiet_json q(j);
                using type = hypothesis_pair<t_value_type>;

                // Populate default values.
                t_value_type null_value = x.null();
                t_value_type alt_value = x.alt();

                // Parse json entries.
                q.required(type::jstr_null, null_value);
                q.required(type::jstr_alt, alt_value);
                
                // Reconstruct the object.
                if (!q.good())
                {
                    aftermath::quiet_error::instance().push(
                        aftermath::not_an_error::runtime_error,
                        aftermath::severity_level::major,
                        q.message(), __FUNCTION__, __LINE__);
                    return;
                } // if (...)
                x = type(null_value, alt_value);
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
