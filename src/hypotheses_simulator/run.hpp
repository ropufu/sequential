
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RUN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RUN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../hypotheses/json.hpp"

#include "../hypotheses/model.hpp"
#include "init_info.hpp"
#include "simulation_pair.hpp"

#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <map>      // std::map
#include <string>   // std::string
#include <unordered_set> // std::unordered_set
#include <utility>  // std::pair
#include <vector>   // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Description of a simulation.
             *  The signal and noise informations, as well as the list of procedures to be run, are stored in the config file.
             *  A \c run corresponds to specific state of the above, namely:
             *  - List of rules to execute.
             *  - Thresholds for them.
             */
            template <typename t_value_type>
            struct run
            {
                using type = run<t_value_type>;

                using value_type = t_value_type;
                using model_type = hypotheses::model<t_value_type>;
                
                // ~~ Json names ~~
                static constexpr char jstr_model[] = "model";
                static constexpr char jstr_analyzed_mu[] = "analyzed mu";
                static constexpr char jstr_simulated_mu[] = "simulated mu";
                static constexpr char jstr_threshold_count[] = "threshold count";
                static constexpr char jstr_threshold_spacing[] = "threshold spacing";
                static constexpr char jstr_init_rules[] = "rule init";

            private:
                model_type m_model = { };
                std::unordered_set<simulation_pair<value_type>> m_simulation_pairs = { };
                std::size_t m_threshold_count = 0;
                std::string m_threshold_spacing = "log";
                std::map<std::size_t, init_info<value_type>> m_init_rules = { };

            public:
                run() noexcept { }

                run(const model_type& model) noexcept
                    : m_model(model)
                {
                    this->m_simulation_pairs.emplace(model.mu_under_null(), model.mu_under_null(), operating_characteristic::ess_under_null); // ESS[mu_0]
                    this->m_simulation_pairs.emplace(model.mu_under_null(), model.smallest_mu_under_alt(), operating_characteristic::probability_of_false_alarm); // PFA
                    this->m_simulation_pairs.emplace(model.smallest_mu_under_alt(), model.mu_under_null(), operating_characteristic::probability_of_missed_signal); // PMS
                    this->m_simulation_pairs.emplace(model.smallest_mu_under_alt(), model.smallest_mu_under_alt(), operating_characteristic::ess_under_alt); // ESS[mu_1]
                } // run(...)

                const model_type& model() const noexcept { return this->m_model; }

                const std::unordered_set<simulation_pair<value_type>>& simulation_pairs() const noexcept { return this->m_simulation_pairs; }

                void study(value_type analyzed_mu, value_type simulated_mu) noexcept { this->m_simulation_pairs.emplace(analyzed_mu, simulated_mu); }

                std::size_t threshold_count() const noexcept { return this->m_threshold_count; }
                const std::string& threshold_spacing() const noexcept { return this->m_threshold_spacing; }

                void configure_thresholds(std::size_t count, const std::string& spacing) noexcept
                {
                    this->m_threshold_count = count;
                    this->m_threshold_spacing = spacing;
                } // configure_thresholds(...)

                const std::map<std::size_t, init_info<value_type>>& init_rules() const noexcept { return this->m_init_rules; }

                void study(const init_info<value_type>& init) noexcept { this->m_init_rules.emplace(init.rule_id(), init); }

                /** @brief Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct run

            // ~~ Json name definitions ~~
            template <typename t_value_type> constexpr char run<t_value_type>::jstr_model[];
            template <typename t_value_type> constexpr char run<t_value_type>::jstr_analyzed_mu[];
            template <typename t_value_type> constexpr char run<t_value_type>::jstr_simulated_mu[];
            template <typename t_value_type> constexpr char run<t_value_type>::jstr_threshold_count[];
            template <typename t_value_type> constexpr char run<t_value_type>::jstr_threshold_spacing[];
            template <typename t_value_type> constexpr char run<t_value_type>::jstr_init_rules[];
            
            template <typename t_value_type>
            void to_json(nlohmann::json& j, const run<t_value_type>& x) noexcept
            {
                using type = run<t_value_type>;

                std::vector<t_value_type> analyzed_mu { };
                std::vector<t_value_type> simulated_mu { };
                for (const simulation_pair<t_value_type>& pair : x.simulation_pairs())
                {
                    analyzed_mu.push_back(pair.analyzed_mu());
                    simulated_mu.push_back(pair.simulated_mu());
                }
                std::vector<init_info<t_value_type>> init_rules { };
                for (const std::pair<std::size_t, init_info<t_value_type>>& item : x.init_rules()) init_rules.push_back(item.second);

                j = nlohmann::json{
                    {type::jstr_model, x.model()},
                    {type::jstr_analyzed_mu, analyzed_mu},
                    {type::jstr_simulated_mu, simulated_mu},
                    {type::jstr_threshold_count, x.threshold_count()},
                    {type::jstr_threshold_spacing, x.threshold_spacing()},
                    {type::jstr_init_rules, init_rules}
                };
            } // to_json(...)
        
            template <typename t_value_type>
            void from_json(const nlohmann::json& j, run<t_value_type>& x) noexcept
            {
                quiet_json q(__FUNCTION__, __LINE__);
                using type = run<t_value_type>;
                using model_type = model<t_value_type>;

                // Populate default values.
                model_type model = x.model();
                std::vector<t_value_type> analyzed_mu { };
                std::vector<t_value_type> simulated_mu { };
                for (const simulation_pair<t_value_type>& pair : x.simulation_pairs())
                {
                    analyzed_mu.push_back(pair.analyzed_mu());
                    simulated_mu.push_back(pair.simulated_mu());
                }
                std::size_t threshold_count = x.threshold_count();
                std::string threshold_spacing = x.threshold_spacing();
                std::map<std::size_t, init_info<t_value_type>> init_rules = x.init_rules();

                // Parse json entries.
                if (quiet_json::is_missing(j, type::jstr_model)) return;
                model = j[type::jstr_model];
                if (!quiet_json::optional(j, type::jstr_analyzed_mu, analyzed_mu)) return;
                if (!quiet_json::optional(j, type::jstr_simulated_mu, simulated_mu)) return;
                if (!quiet_json::required(j, type::jstr_threshold_count, threshold_count)) return;
                if (!quiet_json::required(j, type::jstr_threshold_spacing, threshold_spacing)) return;
                if (quiet_json::is_missing(j, type::jstr_init_rules)) return;
                init_rules.clear();
                for (const nlohmann::json& k : j[type::jstr_init_rules])
                {
                    init_info<t_value_type> z = k;
                    init_rules.emplace(z.rule_id(), z);
                }

                // Validate.
                if (analyzed_mu.size() != simulated_mu.size())
                {
                    aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "Analyzed and simulated mu's have to be of the same size.", __FUNCTION__, __LINE__);
                    return;
                }
                
                // Reconstruct the object.
                x = type(model);
                for (std::size_t i = 0; i < analyzed_mu.size(); ++i) x.study(analyzed_mu[i], simulated_mu[i]);
                x.configure_thresholds(threshold_count, threshold_spacing);
                for (const std::pair<std::size_t, init_info<t_value_type>>& item : init_rules) x.study(item.second);

                q.validate();
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RUN_HPP_INCLUDED
