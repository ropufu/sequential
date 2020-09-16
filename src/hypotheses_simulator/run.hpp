
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include <ropufu/algebra/interval.hpp>

#include "../hypotheses/model.hpp"
#include "../hypotheses/hypothesis_pair.hpp"
#include "../hypotheses/operating_characteristic.hpp"
#include "../hypotheses/change_of_measure.hpp"

#include "spacing.hpp"
#include "init_info.hpp"

#include <algorithm>   // std::sort
#include <cstddef>     // std::size_t
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <vector>      // std::vector

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

        using model_type = hypotheses::model<value_type>;
        using change_of_measure_type = hypotheses::change_of_measure<value_type>;
        using init_info_type = hypotheses::init_info<value_type>;
        
        // ~~ Json names ~~
        static constexpr std::string_view jstr_model = "model";
        static constexpr std::string_view jstr_threshold_spacing = "threshold spacing";
        static constexpr std::string_view jstr_threshold_count = "threshold count";
        static constexpr std::string_view jstr_signal_strengths = "signal strengths";
        static constexpr std::string_view jstr_inits = "inits";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        model_type m_model = {};
        spacing m_threshold_spacing = spacing::logarithmic;
        hypothesis_pair<std::size_t> m_threshold_count = {};
        std::vector<change_of_measure_type> m_signal_strengths = {};
        std::vector<init_info_type> m_inits = {};

        void optimize() noexcept
        {
            std::sort(
                this->m_signal_strengths.begin(), this->m_signal_strengths.end(),
                [] (const change_of_measure_type& a, const change_of_measure_type& b) {
                if (a.analyzed() == b.analyzed()) return a.simulated() < b.simulated();
                return a.analyzed() < b.analyzed();
            });

            std::sort(
                this->m_inits.begin(), this->m_inits.end(),
                [] (const init_info_type& a, const init_info_type& b) {
                return a.rule_id() < b.rule_id();
            });
        } // optimize(...)

    public:
        run() noexcept { }

        run(const model_type& model) noexcept
            : m_model(model)
        {
        } // run(...)

        const model_type& model() const noexcept { return this->m_model; }

        const hypothesis_pair<std::size_t>& threshold_count() const noexcept { return this->m_threshold_count; }

        spacing threshold_spacing() const noexcept { return this->m_threshold_spacing; }

        void set_thresholds(std::size_t null_count, std::size_t alt_count, spacing transform) noexcept
        {
            this->m_threshold_count = hypothesis_pair<std::size_t>(null_count, alt_count);
            this->m_threshold_spacing = transform;
        } // set_thresholds(...)

        /** Explicit simulation pairs to be run. */
        const std::vector<change_of_measure_type>& signal_strengths() const noexcept { return this->m_signal_strengths; }

        /** Add an explicit simulation pair to be run. */
        void study(value_type analyzed_mu, value_type simulated_mu) noexcept
        {
            this->m_signal_strengths.emplace_back(analyzed_mu, simulated_mu);
            this->optimize();
        } // study(...)

        /** Rule initialization information. */
        const std::vector<init_info_type>& inits() const noexcept { return this->m_inits; }

        /** Add an rule initialization information. */
        void study(const init_info_type& rule_info) noexcept
        {
            this->m_inits.push_back(rule_info);
            this->optimize();
        } // study(...)

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_model == other.m_model &&
                this->m_threshold_spacing == other.m_threshold_spacing &&
                this->m_threshold_count == other.m_threshold_count &&
                this->m_signal_strengths == other.m_signal_strengths &&
                this->m_inits == other.m_inits;
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
    }; // struct run

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const run<t_value_type>& x) noexcept
    {
        using type = run<t_value_type>;

        j = nlohmann::json{
            {type::jstr_model, x.model()},
            {type::jstr_threshold_spacing, x.threshold_spacing()},
            {type::jstr_threshold_count, x.threshold_count()},
            {type::jstr_signal_strengths, x.signal_strengths()},
            {type::jstr_inits, x.inits()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, run<t_value_type>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <run> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::run<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::run<t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            // Parse json entries.
            if (!noexcept_json::required(j, result_type::jstr_model, x.m_model)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_threshold_spacing, x.m_threshold_spacing)) return false;
            if (!noexcept_json::required(j, result_type::jstr_threshold_count, x.m_threshold_count)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_signal_strengths, x.m_signal_strengths)) return false;
            if (!noexcept_json::required(j, result_type::jstr_inits, x.m_inits)) return false;

            // Misc.
            x.optimize();

            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED
