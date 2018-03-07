
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATION_PAIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATION_PAIR_HPP_INCLUDED

#include "../hypotheses/core.hpp"
#include "operating_characteristic.hpp"

#include <array>      // std::array
#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <string>     // std::string

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** Represents operating characteristics of a hypothesis testing rule. */
            template <typename t_value_type>
            struct simulation_pair
            {
                using type = simulation_pair<t_value_type>;
                using value_type = t_value_type;

            private:
                value_type m_analyzed_mu = 0;
                value_type m_simulated_mu = 0;
                operating_characteristic m_oc = operating_characteristic::unknown;

            public:
                simulation_pair() noexcept { }

                simulation_pair(value_type analyzed_mu, value_type simulated_mu) noexcept
                    : m_analyzed_mu(analyzed_mu), m_simulated_mu(simulated_mu)
                {
                } // simulation_pair(...)

                simulation_pair(value_type analyzed_mu, value_type simulated_mu, operating_characteristic oc) noexcept
                    : m_analyzed_mu(analyzed_mu), m_simulated_mu(simulated_mu), m_oc(oc)
                {
                } // simulation_pair(...)

                bool special() const noexcept { return this->m_oc != operating_characteristic::unknown; }
                operating_characteristic oc() const noexcept { return this->m_oc; }
                value_type analyzed_mu() const noexcept { return this->m_analyzed_mu; }
                value_type simulated_mu() const noexcept { return this->m_simulated_mu; }

                template <typename t_rule_type>
                auto oc(const t_rule_type& rule) const noexcept -> typename t_rule_type::statistic_type
                {
                    switch (this->m_oc)
                    {
                        case operating_characteristic::ess_under_null: return rule.run_lengths();
                        case operating_characteristic::ess_under_alt: return rule.run_lengths();
                        case operating_characteristic::probability_of_false_alarm: return rule.errors();
                        case operating_characteristic::probability_of_missed_signal: return rule.errors();
                        default: return { };
                    } // switch (...)
                } // oc(...)

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
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

namespace std
{
    template <typename t_value_type>
    struct hash<ropufu::sequential::hypotheses::simulation_pair<t_value_type>>
    {
        using argument_type = ropufu::sequential::hypotheses::simulation_pair<t_value_type>;
        using result_type = std::size_t;

        result_type operator ()(argument_type const& x) const noexcept
        {
            std::hash<typename argument_type::value_type> value_hash = { };
            //std::hash<ropufu::sequential::hypotheses::operating_characteristic> oc_hash = { };

            return
                //oc_hash(x.oc()) ^
                value_hash(x.analyzed_mu()) ^ 
                value_hash(x.simulated_mu());
        } // operator ()(...)
    }; // struct hash<...>
} // namespace std

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATION_PAIR_HPP_INCLUDED
