
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATION_PAIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATION_PAIR_HPP_INCLUDED

#include <ropufu/on_error.hpp> // aftermath::detail::on_error
#include "../draft/algebra/interpolator.hpp"
#include "../draft/algebra/numbers.hpp"

#include "../hypotheses/core.hpp"
#include "../hypotheses/model.hpp"
#include "operating_characteristic.hpp"

#include <array>      // std::array
#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <string>     // std::string, std::to_string

namespace ropufu::sequential::hypotheses
{
    /** Represents operating characteristics of a hypothesis testing rule. */
    template <typename t_value_type>
    struct simulation_pair
    {
        using type = simulation_pair<t_value_type>;
        using value_type = t_value_type;
        using model_type = hypotheses::model<t_value_type>;

    private:
        value_type m_analyzed_mu = 0;
        value_type m_simulated_mu = 0;

    public:
        simulation_pair() noexcept { }

        simulation_pair(value_type analyzed_mu, value_type simulated_mu) noexcept
            : m_analyzed_mu(analyzed_mu), m_simulated_mu(simulated_mu)
        {
        } // simulation_pair(...)

        simulation_pair(operating_characteristic oc, const model_type& model, std::error_code& ec) noexcept
        {
            switch (oc)
            {
                case operating_characteristic::ess_under_null:
                    this->m_analyzed_mu = model.mu_under_null();
                    this->m_simulated_mu = model.mu_under_null();
                    break;
                case operating_characteristic::ess_under_alt:
                    this->m_analyzed_mu = model.smallest_mu_under_alt();
                    this->m_simulated_mu = model.smallest_mu_under_alt();
                    break;
                case operating_characteristic::probability_of_false_alarm:
                    this->m_analyzed_mu = model.mu_under_null();
                    this->m_simulated_mu = model.smallest_mu_under_alt();
                    break;
                case operating_characteristic::probability_of_missed_signal:
                    this->m_analyzed_mu = model.smallest_mu_under_alt();
                    this->m_simulated_mu = model.mu_under_null();
                    break;
                default:
                    aftermath::detail::on_error(ec, std::errc::invalid_argument, std::string("OC not recognized: ") + std::to_string(oc) + std::string("."));
                    break;
            } // switch (...)
        } // simulation_pair(...)

        template <typename t_rule_type>
        auto read_oc(operating_characteristic oc, const t_rule_type& rule, std::error_code& ec) const noexcept -> typename t_rule_type::moment_statistic_type
        {
            switch (oc)
            {
                case operating_characteristic::ess_under_null: return rule.run_lengths();
                case operating_characteristic::ess_under_alt: return rule.run_lengths();
                case operating_characteristic::probability_of_false_alarm: return rule.errors();
                case operating_characteristic::probability_of_missed_signal: return rule.errors();
                default:
                    aftermath::detail::on_error(ec, std::errc::invalid_argument, std::string("OC not recognized: ") + std::to_string(oc) + std::string("."));
                    return {};
            } // switch (...)
        } // read_oc(...)

        value_type analyzed_mu() const noexcept { return this->m_analyzed_mu; }
        value_type simulated_mu() const noexcept { return this->m_simulated_mu; }
        
        std::string to_path_string(std::size_t decimal_places = 3) const noexcept
        {
            std::string result = "analyze ";
            result += detail::to_str(this->m_analyzed_mu, decimal_places);
            result += " simulate ";
            result += detail::to_str(this->m_simulated_mu, decimal_places);
            return result;
        } // to_path_string(...)

        /** Checks if the two objects are equal. */
        bool operator ==(const type& other) const noexcept
        {
            return
                //this->m_oc == other.m_oc &&
                this->m_analyzed_mu == other.m_analyzed_mu &&
                this->m_simulated_mu == other.m_simulated_mu;
        } // operator ==(...)

        /** Checks if the two objects are not equal. */
        bool operator !=(const type& other) const noexcept { return !this->operator ==(other); }
    }; // struct simulation_pair
} // namespace ropufu::sequential::hypotheses

namespace std
{
    template <typename t_value_type>
    struct hash<ropufu::sequential::hypotheses::simulation_pair<t_value_type>>
    {
        using argument_type = ropufu::sequential::hypotheses::simulation_pair<t_value_type>;
        using result_type = std::size_t;

        result_type operator ()(argument_type const& x) const noexcept
        {
            std::hash<typename argument_type::value_type> value_hash = {};
            //std::hash<ropufu::sequential::hypotheses::operating_characteristic> oc_hash = {};

            return
                //oc_hash(x.oc()) ^
                value_hash(x.analyzed_mu()) ^ 
                value_hash(x.simulated_mu());
        } // operator ()(...)
    }; // struct hash<...>
} // namespace std

namespace ropufu::modules
{
    template <typename t_value_type, typename t_position_type>
    struct interpolator<sequential::hypotheses::simulation_pair<t_value_type>, t_position_type>
    {
        using type = interpolator<sequential::hypotheses::simulation_pair<t_value_type>, t_position_type>;
        using value_type = sequential::hypotheses::simulation_pair<t_value_type>;
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
                (q) * left.analyzed_mu() + (p) * right.analyzed_mu(),
                (q) * left.simulated_mu() + (p) * right.simulated_mu());
        } // interpolate(...)
    }; // struct interpolator<...>
} // namespace ropufu::modules

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATION_PAIR_HPP_INCLUDED
