
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "json.hpp"

#include <aftermath/not_an_error.hpp> // quiet_error, not_an_error, severity_level

#include "core.hpp"

#include <cmath>    // std::isnan, std::isinf
#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <string>   // std::string

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Describes the composite hypotheses testing setup. */
            template <typename t_value_type>
            struct model
            {
                using type = model<t_value_type>;
                using value_type = t_value_type;

                // ~~ Json names ~~
                static constexpr char jstr_null_mu[] = "null mu";
                static constexpr char jstr_smallest_alt_mu[] = "smallest alt mu";

            private:
                value_type m_null_mu = 0; // Signal "strength" under the null hypothesis.
                value_type m_smallest_alt_mu = 1; // Minimal signal "strength" under the alternative hypothesis.

                void coerce() noexcept
                {
                    if (std::isnan(this->m_null_mu) || std::isinf(this->m_null_mu))
                    {
                        this->m_null_mu = 0;
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "Null mu has to be a number. Coerced null mu to zero.", __FUNCTION__, __LINE__);
                    }

                    if (std::isnan(this->m_smallest_alt_mu) || std::isinf(this->m_smallest_alt_mu))
                    {
                        this->m_smallest_alt_mu = 0;
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "Smallest alternative mu has to be a number. Coerced smallest alt mu to zero.", __FUNCTION__, __LINE__);
                    }
                    
                    if (this->m_null_mu >= this->m_smallest_alt_mu)
                    {
                        this->m_smallest_alt_mu = this->m_null_mu + 1;
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "Smallest alternative mu does not exceed null mu. Coerced smallest alt mu to null mu plus one.", __FUNCTION__, __LINE__);
                    }
                } // coerce(...)

            public:
                // std::ostringstream& mat_prefix(std::ostringstream& os) const noexcept
                // {
                //     os << "_mu_null_" << this->m_null_mu << "_alt_" << this->m_smallest_alt_mu;
                //     return os;
                // } // mat_prefix(...)

                model() noexcept { }

                model(value_type null_mu, value_type smallest_alt_mu) noexcept
                    : m_null_mu(null_mu), m_smallest_alt_mu(smallest_alt_mu)
                {
                    this->coerce();
                } // model(...)

                /** @brief Linear combination of \c mu_under_null and \c mu_under_alt with weights (1 - \p p) and (\p p). */
                value_type mu_relative(value_type p) const noexcept { return (1 - p) * this->m_null_mu + (p) * this->m_smallest_alt_mu; }

                /** @brief Locates where the specified \c mu is on the relative scale between \c mu_under_null and \c mu_under_alt. */
                value_type where(value_type mu) const noexcept { return (mu - this->m_null_mu) / (this->m_smallest_alt_mu - this->m_null_mu); }
                
                /** @brief Signal "strength" under the null hypothesis. */
                value_type mu_under_null() const noexcept { return this->m_null_mu; }
                /** @brief Minimal signal "strength" under the alternative hypothesis. */
                value_type smallest_mu_under_alt() const noexcept { return this->m_smallest_alt_mu; }
                /** @brief Determines if the provided signal "strength" falls into the null category. */
                bool is_null(value_type theta) const noexcept { return theta == this->m_null_mu; }
                /** @brief Determines if the provided signal "strength" falls into the alternative category. */
                bool is_alt(value_type theta) const noexcept { return theta >= this->m_smallest_alt_mu; }

                void set_hypotheses(value_type null_mu, value_type smallest_alt_mu) noexcept
                {
                    this->m_null_mu = null_mu;
                    this->m_smallest_alt_mu = smallest_alt_mu;
                    this->coerce();
                } // set_hypotheses(...)

                std::string to_path_string(std::size_t decimal_places = 3) const noexcept
                {
                    std::string result = "model mu ";
                    result += detail::to_str(this->m_null_mu, decimal_places);
                    result += " to ";
                    result += detail::to_str(this->m_smallest_alt_mu, decimal_places);
                    return result;
                } // to_path_string(...)

                /** @brief Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct model

            // ~~ Json name definitions ~~
            template <typename t_value_type> constexpr char model<t_value_type>::jstr_null_mu[];
            template <typename t_value_type> constexpr char model<t_value_type>::jstr_smallest_alt_mu[];
            
            template <typename t_value_type>
            void to_json(nlohmann::json& j, const model<t_value_type>& x) noexcept
            {
                using type = model<t_value_type>;

                j = nlohmann::json{
                    {type::jstr_null_mu, x.mu_under_null()},
                    {type::jstr_smallest_alt_mu, x.smallest_mu_under_alt()}
                };
            } // to_json(...)
        
            template <typename t_value_type>
            void from_json(const nlohmann::json& j, model<t_value_type>& x) noexcept
            {
                quiet_json q(__FUNCTION__, __LINE__);
                using type = model<t_value_type>;

                // Populate default values.
                t_value_type null_mu = x.mu_under_null();
                t_value_type smallest_alt_mu = x.smallest_mu_under_alt();

                // Parse json entries.
                if (!quiet_json::optional(j, type::jstr_null_mu, null_mu)) return;
                if (!quiet_json::required(j, type::jstr_smallest_alt_mu, smallest_alt_mu)) return;
                
                // Reconstruct the object.
                x.set_hypotheses(null_mu, smallest_alt_mu);

                q.validate();
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
