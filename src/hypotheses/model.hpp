
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../draft/format.hpp"

#include <cstddef>     // std::size_t
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct model;
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const model<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, model<t_value_type>& x);

    /** @brief Describes the composite hypotheses testing setup. */
    template <typename t_value_type>
    struct model
    {
        using type = model<t_value_type>;
        using value_type = t_value_type;

        // ~~ Json names ~~
        static constexpr std::string_view jstr_null_mu = "null mu";
        static constexpr std::string_view jstr_smallest_alt_mu = "smallest alt mu";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        value_type m_null_mu = 0; // Signal strength under the null hypothesis.
        value_type m_smallest_alt_mu = 1; // Minimal signal strength under the alternative hypothesis.

        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_finite(this->m_null_mu)) return "Null mu must be finite.";
            if (!aftermath::is_finite(this->m_smallest_alt_mu)) return "Smallest alternative mu must be finite.";
            if (this->m_null_mu >= this->m_smallest_alt_mu) return "Smallest alternative mu must be greater than null mu.";
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        /** @brief Hypothesis test of "mu = 0" vs "mu > 1". */
        model() noexcept { }

        /** @brief Hypothesis test of "mu = \p null_mu" vs "mu > \p m_smallest_alt_mu".
         *  @exception std::logic_error \p null_mu is not finite.
         *  @exception std::logic_error \p smallest_alt_mu is not finite.
         *  @exception std::logic_error \p null_mu is not smaller than \p smallest_alt_mu.
         */
        model(value_type null_mu, value_type smallest_alt_mu)
            : m_null_mu(null_mu), m_smallest_alt_mu(smallest_alt_mu)
        {
            this->validate();
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

        /** @brief Resets the hypotheses.
         *  @exception std::logic_error \p null_mu is not finite.
         *  @exception std::logic_error \p smallest_alt_mu is not finite.
         *  @exception std::logic_error \p null_mu is not smaller than \p smallest_alt_mu.
         */
        void set_hypotheses(value_type null_mu, value_type smallest_alt_mu)
        {
            this->m_null_mu = null_mu;
            this->m_smallest_alt_mu = smallest_alt_mu;
            this->validate();
        } // set_hypotheses(...)

        std::string to_path_string(std::size_t decimal_places = 3) const noexcept
        {
            std::string result = "model mu ";
            result += ropufu::draft::detail::to_str(this->m_null_mu, decimal_places);
            result += " to ";
            result += ropufu::draft::detail::to_str(this->m_smallest_alt_mu, decimal_places);
            return result;
        } // to_path_string(...)

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_null_mu == other.m_null_mu &&
                this->m_smallest_alt_mu == other.m_smallest_alt_mu;
        } // operator ==(...)

        bool operator !=(const type& other) const noexcept
        {
            return !this->operator ==(other);
        } // operator !=(...)

        /** @brief Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct model
    
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
    void from_json(const nlohmann::json& j, model<t_value_type>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <model> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::model<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::model<t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            // Parse json entries.
            if (!noexcept_json::optional(j, result_type::jstr_null_mu, x.m_null_mu)) return false;
            if (!noexcept_json::required(j, result_type::jstr_smallest_alt_mu, x.m_smallest_alt_mu)) return false;

            // Validate entries.
            if (x.error_message().has_value()) return false;
            
            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
