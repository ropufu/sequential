
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include <ropufu/algebra/range.hpp>

#include "../hypotheses/model.hpp"
#include "../hypotheses/hypothesis_pair.hpp"
#include "../hypotheses/operating_characteristic.hpp"
#include "../hypotheses/change_of_measure.hpp"

#include "spacing.hpp"
#include "init_info.hpp"

#include <algorithm> // std::sort
#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error
#include <string>    // std::string, std::to_string
#include <system_error> // std::error_code, std::make_error_code, std::errc
#include <utility> // std::pair
#include <vector> // std::vector

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
        static constexpr char jstr_model[] = "model";
        static constexpr char jstr_threshold_spacing[] = "threshold spacing";
        static constexpr char jstr_threshold_count[] = "threshold count";
        static constexpr char jstr_signal_strengths[] = "signal strengths";
        static constexpr char jstr_inits[] = "inits";

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

        run(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Populate default values.
            model_type model = this->m_model;
            spacing threshold_spacing = this->m_threshold_spacing;
            hypothesis_pair<std::size_t> threshold_count = this->m_threshold_count;
            std::vector<change_of_measure_type> signal_strengths = this->m_signal_strengths;
            std::vector<init_info_type> inits = this->m_inits;
            
            // Parse json entries.
            aftermath::noexcept_json::required(j, type::jstr_model, model, ec);
            aftermath::noexcept_json::optional(j, type::jstr_threshold_spacing, threshold_spacing, ec);
            aftermath::noexcept_json::required(j, type::jstr_threshold_count, threshold_count, ec);
            aftermath::noexcept_json::optional(j, type::jstr_signal_strengths, signal_strengths, ec);
            aftermath::noexcept_json::required(j, type::jstr_inits, inits, ec);
            if (ec.value() != 0) return;
            
            // Populate values.
            this->m_model = model;
            this->m_threshold_spacing = threshold_spacing;
            this->m_threshold_count = threshold_count;
            this->m_signal_strengths = signal_strengths;
            this->m_inits = inits;

            // Misc.
            this->optimize();
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

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char run<t_value_type>::jstr_model[];
    template <typename t_value_type> constexpr char run<t_value_type>::jstr_threshold_spacing[];
    template <typename t_value_type> constexpr char run<t_value_type>::jstr_threshold_count[];
    template <typename t_value_type> constexpr char run<t_value_type>::jstr_signal_strengths[];
    template <typename t_value_type> constexpr char run<t_value_type>::jstr_inits[];
    
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
        using type = run<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <run> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_RUN_HPP_INCLUDED
