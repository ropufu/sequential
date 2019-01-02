
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RUN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RUN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/algebra.hpp>  //  aftermath::algebra::range
#include <ropufu/on_error.hpp> // aftermath::detail::on_error
#include "../draft/algebra/interpolator.hpp"
#include "../draft/algebra/numbers.hpp"

#include "../hypotheses/model.hpp"
#include "hypothesis_pair.hpp"
#include "init_info.hpp"
#include "operating_characteristic.hpp"
#include "simulation_pair.hpp"

#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <map>      // std::map
#include <set>      // std::set
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string, std::to_string
#include <system_error> // std::error_code, std::make_error_code, std::errc
#include <utility>  // std::pair
#include <vector>   // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct run;
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const run<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, run<t_value_type>& x);

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
        model_type m_model = {};
        hypothesis_pair<std::size_t> m_threshold_count = {};
        aftermath::algebra::spacing m_threshold_spacing = aftermath::algebra::spacing::logarithmic;
        std::vector<simulation_pair<value_type>> m_simulation_pairs = {};
        std::map<std::size_t, init_info<value_type>> m_init_rules = {};

    public:
        run() noexcept { }

        run(const model_type& model) noexcept : m_model(model) { }

        run(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Auxiliary.
            std::vector<value_type> analyzed_mu {};
            std::vector<value_type> simulated_mu {};
            for (const simulation_pair<value_type>& pair : this->m_simulation_pairs)
            {
                analyzed_mu.push_back(pair.analyzed_mu());
                simulated_mu.push_back(pair.simulated_mu());
            } // for (...)
            
            std::string threshold_spacing_str = std::to_string(this->m_threshold_spacing);

            std::map<std::size_t, init_info<value_type>> init_rules_map = this->m_init_rules;
            std::vector<init_info<value_type>> init_rules {};
            init_rules.reserve(init_rules_map.size());
            for (const std::pair<std::size_t, init_info<value_type>>& item : init_rules_map) init_rules.push_back(item.second);

            // Parse json entries.
            aftermath::noexcept_json::required(j, type::jstr_model, this->m_model, ec);
            aftermath::noexcept_json::optional(j, type::jstr_analyzed_mu, analyzed_mu, ec);
            aftermath::noexcept_json::optional(j, type::jstr_simulated_mu, simulated_mu, ec);
            aftermath::noexcept_json::required(j, type::jstr_threshold_count, this->m_threshold_count, ec);
            aftermath::noexcept_json::required(j, type::jstr_threshold_spacing, threshold_spacing_str, ec);
            aftermath::noexcept_json::required(j, type::jstr_init_rules, init_rules, ec);
            
            // Enum structures etc.
            if (!aftermath::detail::try_parse_enum(threshold_spacing_str, this->m_threshold_spacing))
            {
                aftermath::detail::on_error(ec, std::errc::illegal_byte_sequence, std::string("Threshold spacing unrecognized: ") + threshold_spacing_str + std::string("."));
                return;
            } // if (...)
            if (analyzed_mu.size() != simulated_mu.size())
            {
                aftermath::detail::on_error(ec, std::errc::invalid_argument, "Analyzed and simulated mu's have to be of the same size.");
                return;
            } // if (...)

            for (std::size_t i = 0; i < analyzed_mu.size(); ++i) this->study(analyzed_mu[i], simulated_mu[i]);
            for (const init_info<value_type>& item : init_rules) this->study(item);
        } // run(...)

        /** @brief Adds \p count_in_between runs in-between each pair of existing \p runs. */
        static std::vector<type> interpolate(const std::vector<type>& keyframe_runs, std::size_t count_in_between) noexcept
        {
            if (count_in_between == 0) return keyframe_runs;
            if (keyframe_runs.size() <= 1) return keyframe_runs;
            /* Key frames are marked with 'o'-s.
                o--^--o--^--o--^--o--...--o--^--o 
                    /.\   /.\   /.\           /.\   
                    ...   ...   ...           ...   
            */
            
            std::size_t count_pairs = keyframe_runs.size() - 1;
            std::vector<type> result {};
            result.reserve(keyframe_runs.size() + count_pairs * count_in_between);
            
            result.push_back(keyframe_runs.front());
            std::error_code ec {};
            for (std::size_t i = 1; i < keyframe_runs.size(); ++i)
            {
                const type& left = keyframe_runs[i - 1];
                const type& right = keyframe_runs[i];
                for (std::size_t j = 0; j < count_in_between; ++j)
                {
                    // left ----- 1  ----- 2  ----- ... ----- n  ----- right 
                    //    0 ----- h  ----- 2h ----- ... ----- nh ----- 1     
                    value_type p = (j + 1) / static_cast<value_type>(count_in_between + 1);
                    result.push_back(modules::interpolator<type, value_type>::interpolate(left, right, p, ec));
                    if (ec.value() != 0) return keyframe_runs;
                }
                result.push_back(right);
            }
            return result;
        } // interpolate(...)

        const model_type& model() const noexcept { return this->m_model; }

        const hypothesis_pair<std::size_t>& threshold_count() const noexcept { return this->m_threshold_count; }
        aftermath::algebra::spacing threshold_spacing() const noexcept { return this->m_threshold_spacing; }

        void configure_thresholds(std::size_t null_count, std::size_t alt_count, aftermath::algebra::spacing transform) noexcept
        {
            this->m_threshold_count = hypothesis_pair<std::size_t>(null_count, alt_count);
            this->m_threshold_spacing = transform;
        } // configure_thresholds(...)

        /** Explicit simulation pairs to be run---in addition to the standard OC runs. */
        const std::vector<simulation_pair<value_type>>& simulation_pairs() const noexcept { return this->m_simulation_pairs; }

        /** Add an explicit simulation pair to be run---in addition to the standard OC runs. */
        void study(value_type analyzed_mu, value_type simulated_mu) noexcept { this->m_simulation_pairs.emplace_back(analyzed_mu, simulated_mu); }

        const std::map<std::size_t, init_info<value_type>>& init_rules() const noexcept { return this->m_init_rules; }

        void study(const init_info<value_type>& init) noexcept { this->m_init_rules.emplace(init.rule_id(), init); }

        static bool is_comparable(const type& left, const type& right) noexcept
        {
            // Make sure the runs have the same threshold setup.
            if (left.m_threshold_count != right.m_threshold_count) return false;
            if (left.m_threshold_spacing != right.m_threshold_spacing) return false;

            // Make sure the runs have compatible simulation pairs.
            if (left.m_simulation_pairs.size() != right.m_simulation_pairs.size()) return false;

            // Make sure the runs have the same rules.
            std::set<std::size_t> left_rule_ids {};
            std::set<std::size_t> right_rule_ids {};
            for (const std::pair<std::size_t, init_info<value_type>>& item : left.m_init_rules) left_rule_ids.insert(item.first);
            for (const std::pair<std::size_t, init_info<value_type>>& item : right.m_init_rules) right_rule_ids.insert(item.first);

            if (left_rule_ids != right_rule_ids) return false;

            return true;
        } // is_comparable(...)

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

        std::vector<t_value_type> analyzed_mu {};
        std::vector<t_value_type> simulated_mu {};
        for (const simulation_pair<t_value_type>& pair : x.simulation_pairs())
        {
            analyzed_mu.push_back(pair.analyzed_mu());
            simulated_mu.push_back(pair.simulated_mu());
        }
        std::string threshold_spacing_str = std::to_string(x.threshold_spacing());
        std::vector<init_info<t_value_type>> init_rules {};
        for (const std::pair<std::size_t, init_info<t_value_type>>& item : x.init_rules()) init_rules.push_back(item.second);

        j = nlohmann::json{
            {type::jstr_model, x.model()},
            {type::jstr_analyzed_mu, analyzed_mu},
            {type::jstr_simulated_mu, simulated_mu},
            {type::jstr_threshold_count, x.threshold_count()},
            {type::jstr_threshold_spacing, threshold_spacing_str},
            {type::jstr_init_rules, init_rules}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, run<t_value_type>& x)
    {
        using type = run<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu::modules
{
    template <typename t_value_type, typename t_position_type>
    struct interpolator<sequential::hypotheses::run<t_value_type>, t_position_type>
    {
        using type = interpolator<sequential::hypotheses::run<t_value_type>, t_position_type>;
        using value_type = sequential::hypotheses::run<t_value_type>;
        using position_type = t_position_type;
        using clipper_type = clipper<t_position_type>;

        using model_type = sequential::hypotheses::model<t_value_type>;
        using simulation_pair_type = sequential::hypotheses::simulation_pair<t_value_type>;
        using init_info_type = sequential::hypotheses::init_info<t_value_type>;

        template <typename t_type>
        using interpolator_t = interpolator<t_type, position_type>;

        static value_type interpolate(const value_type& left, const value_type& right, const position_type& relative_position, std::error_code& ec) noexcept
        {
            value_type bad {};
            // First, make sure the runs are comparable.
            if (!value_type::is_comparable(left, right))
            {
                return aftermath::detail::on_error(ec, std::errc::function_not_supported, "Runs incompatible.", bad);
            } // if (...)
            sequential::hypotheses::hypothesis_pair<std::size_t> threshold_count = left.threshold_count();
            aftermath::algebra::spacing threshold_spacing = left.threshold_spacing();
            std::size_t pair_count = left.simulation_pairs().size();
            std::size_t rule_count = left.init_rules().size();

            // Second, coerce the relative position.
            position_type p = relative_position; // Make a copy of <relative_position>.
            if (!clipper_type::was_finite(p, 0) || !clipper_type::was_between(p, 0, 1))
            {
                return aftermath::detail::on_error(ec, std::errc::argument_out_of_domain, "Relative position out of range.", bad);
            } // if (...)

            // Third, interpolate the model.
            model_type model = interpolator_t<model_type>::interpolate(left.model(), right.model(), p, ec);
            if (ec.value() != 0) return bad;

            // Fourth, interpolate the simulation pairs.
            std::vector<simulation_pair_type> simulation_pairs(pair_count);
            for (std::size_t i = 0; i < pair_count; ++i) 
            {
                simulation_pairs[i] = interpolator_t<simulation_pair_type>::interpolate(left.simulation_pairs()[i], right.simulation_pairs()[i], p, ec);
                if (ec.value() != 0) return bad;
            } // for (...)

            // Fifth, interpolate the init rules.
            std::vector<init_info_type> init_rules {};
            init_rules.reserve(rule_count);
            for (const std::pair<std::size_t, init_info_type>& item : left.init_rules())
            {
                init_rules.push_back(interpolator_t<init_info_type>::interpolate(item.second, right.init_rules().find(item.first)->second, p, ec));
                if (ec.value() != 0) return bad;
            } // for (...)

            // Finally, reconstruct the object.
            value_type x(model);
            for (const simulation_pair_type& item : simulation_pairs) x.study(item.analyzed_mu(), item.simulated_mu());
            x.configure_thresholds(threshold_count.null(), threshold_count.alt(), threshold_spacing);
            for (const init_info_type& item : init_rules) x.study(item);
            return x;
        } // interpolate(...)
    }; // struct interpolator<...>
} // namespace ropufu::modules

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RUN_HPP_INCLUDED
