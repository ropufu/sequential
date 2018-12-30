
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/on_error.hpp> // aftermath::detail::on_error

#include "core.hpp"
#include "../draft/algebra/interpolator.hpp"
#include "../draft/algebra/numbers.hpp"

#include <cmath>    // std::isnan, std::isinf
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
        value_type m_null_mu = 0; // Signal "strength" under the null hypothesis.
        value_type m_smallest_alt_mu = 1; // Minimal signal "strength" under the alternative hypothesis.

    protected:
        bool validate(std::error_code& ec) const noexcept
        {
            if (std::isnan(this->m_null_mu) || std::isinf(this->m_null_mu)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Null mu has to be a finite number.", false);
            if (std::isnan(this->m_smallest_alt_mu) || std::isinf(this->m_smallest_alt_mu)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Smallest alternative mu has to be a finite number.", false);
            if (this->m_null_mu >= this->m_smallest_alt_mu) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Smallest alternative has to be greater than null mu.", false);
            return true;
        } // validate(...)

        void coerce() noexcept
        {
            if (std::isnan(this->m_null_mu) || std::isinf(this->m_null_mu)) this->m_null_mu = 0;
            if (std::isnan(this->m_smallest_alt_mu) || std::isinf(this->m_smallest_alt_mu)) this->m_smallest_alt_mu = 0;
            if (this->m_null_mu >= this->m_smallest_alt_mu) this->m_smallest_alt_mu = this->m_null_mu + 1;
        } // coerce(...)

    public:
        model() noexcept { }

        model(value_type null_mu, value_type smallest_alt_mu, std::error_code& ec) noexcept
            : m_null_mu(null_mu), m_smallest_alt_mu(smallest_alt_mu)
        {
            if (!this->validate(ec)) this->coerce();
        } // model(...)
        
        model(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Parse json entries.
            aftermath::noexcept_json::optional(j, type::jstr_null_mu, this->m_null_mu, ec);
            aftermath::noexcept_json::required(j, type::jstr_smallest_alt_mu, this->m_smallest_alt_mu, ec);
            
            if (!this->validate(ec)) this->coerce();
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

        void set_hypotheses(value_type null_mu, value_type smallest_alt_mu, std::error_code& ec) noexcept
        {
            this->m_null_mu = null_mu;
            this->m_smallest_alt_mu = smallest_alt_mu;
            if (!this->validate(ec)) this->coerce();
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
    void from_json(const nlohmann::json& j, model<t_value_type>& x)
    {
        using type = model<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu

namespace ropufu::modules
{
    template <typename t_value_type, typename t_position_type>
    struct interpolator<sequential::hypotheses::model<t_value_type>, t_position_type>
    {
        using type = interpolator<sequential::hypotheses::model<t_value_type>, t_position_type>;
        using value_type = sequential::hypotheses::model<t_value_type>;
        using position_type = t_position_type;
        using clipper_type = clipper<t_position_type>;

        static value_type interpolate(const value_type& left, const value_type& right, const position_type& relative_position, std::error_code& ec) noexcept
        {
            value_type bad {};

            position_type p = relative_position; // Make a copy of <relative_position>.
            if (!clipper_type::was_finite(p, 0) || !clipper_type::was_between(p, 0, 1))
            {
                return aftermath::detail::on_error(ec, std::errc::argument_out_of_domain, "Relative position out of range.", bad);
            } // if (...)
            
            position_type q = 1 - p;

            return value_type(
                (q) * left.mu_under_null() + (p) * right.mu_under_null(),
                (q) * left.smallest_mu_under_alt() + (p) * right.smallest_mu_under_alt(), ec);
        } // interpolate(...)
    }; // struct interpolator<...>
} // namespace ropufu::modules

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MODEL_HPP_INCLUDED
