
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "format.hpp"

#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc

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
        static constexpr char jstr_null_mu[] = "null mu";
        static constexpr char jstr_smallest_alt_mu[] = "smallest alt mu";

    private:
        value_type m_null_mu = 0; // Signal strength under the null hypothesis.
        value_type m_smallest_alt_mu = 1; // Minimal signal strength under the alternative hypothesis.

        static bool is_valid(value_type null_mu, value_type smallest_alt_mu, std::string& message) noexcept
        {
            if (!aftermath::is_finite(null_mu))
            {
                message = "Null mu must be finite.";
                return false;
            } // if (...)
            if (!aftermath::is_finite(smallest_alt_mu))
            {
                message = "Smallest alternative mu must be finite.";
                return false;
            } // if (...)
            if (null_mu >= smallest_alt_mu)
            {
                message = "Smallest alternative mu must be greater than null mu.";
                return false;
            } // if (...)
            return true;
        } // validate(...)

        void validate() const
        {
            std::string message {};
            if (!type::is_valid(this->m_null_mu, this->m_smallest_alt_mu, message))
                throw std::logic_error(message);
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
        
        model(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Parse json entries.
            value_type null_mu = this->m_null_mu;
            value_type smallest_alt_mu = this->m_smallest_alt_mu;
            aftermath::noexcept_json::optional(j, type::jstr_null_mu, null_mu, ec);
            aftermath::noexcept_json::required(j, type::jstr_smallest_alt_mu, smallest_alt_mu, ec);
            if (ec.value() != 0) return;
            
            // Validate entries.
            std::string message {};
            if (!type::is_valid(null_mu, smallest_alt_mu, message))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
            
            // Populate values.
            this->m_null_mu = null_mu;
            this->m_smallest_alt_mu = smallest_alt_mu;
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
            result += detail::to_str(this->m_null_mu, decimal_places);
            result += " to ";
            result += detail::to_str(this->m_smallest_alt_mu, decimal_places);
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
    void from_json(const nlohmann::json& j, model<t_value_type>& x)
    {
        using type = model<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <model> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
