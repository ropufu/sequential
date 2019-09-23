
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_SPRT_FACTORY_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_SPRT_FACTORY_HPP_INCLUDED

#include "../hypotheses/model.hpp"
#include "../hypotheses/hypothesis_pair.hpp"
#include "../hypotheses/observer.hpp"
#include "../hypotheses/rules.hpp"

#include "spacing.hpp"
#include "init_info.hpp"

#include <cstddef>   // std::size_t
#include <memory>    // std::shared_ptr, std::make_shared
#include <stdexcept> // std::runtime_error
#include <vector>    // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_engine_type, typename t_value_type>
    struct sprt_factory
    {
        using type = sprt_factory<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using process_type = hypotheses::simple_process<engine_type, value_type>;
        using model_type = hypotheses::model<value_type>;
        using observer_type = hypotheses::observer<engine_type, value_type>;
        using observer_ptr_type = observer_type*;

        using init_info_type = init_info<value_type>;

        using asprt_simple_type = hypotheses::adaptive_sprt<engine_type, value_type, hypotheses::adaptive_sprt_flavor::simple>;
        using asprt_general_type = hypotheses::adaptive_sprt<engine_type, value_type, hypotheses::adaptive_sprt_flavor::general>;
        using asprt_unconstrained_type = hypotheses::adaptive_sprt<engine_type, value_type, hypotheses::adaptive_sprt_flavor::unconstrained>;
        using gsprt_cutoff_type = hypotheses::generalized_sprt<engine_type, value_type, hypotheses::generalized_sprt_flavor::cutoff>;
        using gsprt_general_type = hypotheses::generalized_sprt<engine_type, value_type, hypotheses::generalized_sprt_flavor::general>;
        using dsprt_type = hypotheses::double_sprt<engine_type, value_type>;

    private:
        std::vector<asprt_simple_type> m_asprt_simple_rules = {};
        std::vector<asprt_general_type> m_asprt_general_rules = {};
        std::vector<asprt_unconstrained_type> m_asprt_unconstrained_rules = {};
        std::vector<gsprt_cutoff_type> m_gsprt_cutoff_rules = {};
        std::vector<gsprt_general_type> m_gsprt_general_rules = {};
        std::vector<dsprt_type> m_dsprt_rules = {};

        value_type m_log_likelihood_scale = 0;
        init_info_type m_init = {};
        model_type m_model = {};
        spacing m_threshold_spacing = spacing::logarithmic;
        hypothesis_pair<std::size_t> m_threshold_count = {};

    public:
        explicit sprt_factory(const process_type& process) noexcept
            : m_log_likelihood_scale(process.log_likelihood_scale())
        { }

        void initialize_visitor(const init_info_type& init, const model_type& model,
            spacing threshold_spacing, const hypothesis_pair<std::size_t>& threshold_count) noexcept
        {
            this->m_init = init;
            this->m_model = model;
            this->m_threshold_spacing = threshold_spacing;
            this->m_threshold_count = threshold_count;
        } // initialize_visitor(...)

        std::size_t size() const noexcept
        {
            return 
                this->m_asprt_simple_rules.size() + this->m_asprt_general_rules.size() + this->m_asprt_unconstrained_rules.size() +
                this->m_gsprt_cutoff_rules.size() + this->m_gsprt_general_rules.size() +
                this->m_dsprt_rules.size();
        } // size(...)

        std::vector<observer_ptr_type> observer_pointers() noexcept
        {
            std::vector<observer_ptr_type> result {};
            result.reserve(this->size());

            // Adaptive SPRT.
            for (asprt_simple_type& x : this->m_asprt_simple_rules) result.push_back(&x);
            for (asprt_general_type& x : this->m_asprt_general_rules) result.push_back(&x);
            for (asprt_unconstrained_type& x : this->m_asprt_unconstrained_rules) result.push_back(&x);
            // Generalized SPRT.
            for (gsprt_cutoff_type& x : this->m_gsprt_cutoff_rules) result.push_back(&x);
            for (gsprt_general_type& x : this->m_gsprt_general_rules) result.push_back(&x);
            // Double SPRT.
            for (dsprt_type& x : this->m_dsprt_rules) result.push_back(&x);

            result.shrink_to_fit();
            return result;
        } // observer_pointers(...)

        template <typename t_design_type>
        void operator ()(const t_design_type& /*design*/)
        {
            throw std::runtime_error("Design not recognized.");
        } // operator ()(...)

        void operator ()(const adaptive_sprt_design<value_type>& design)
        {
            std::vector<value_type> null_thresholds {};
            std::vector<value_type> alt_thresholds {};
            this->m_init.make_thresholds(this->m_threshold_count, this->m_threshold_spacing, null_thresholds, alt_thresholds);

            switch (design.flavor())
            {
                case adaptive_sprt_flavor::simple:
                {
                    asprt_simple_type& rule_simple = this->m_asprt_simple_rules.emplace_back(design);
                    rule_simple.initialize(
                        this->m_model,
                        this->m_init.anticipated_run_length(),
                        this->m_log_likelihood_scale,
                        null_thresholds,
                        alt_thresholds);
					break;
                }
                case adaptive_sprt_flavor::general:
                {
                    asprt_general_type& rule_general = this->m_asprt_general_rules.emplace_back(design);
                    rule_general.initialize(
                        this->m_model,
                        this->m_init.anticipated_run_length(),
                        this->m_log_likelihood_scale,
                        null_thresholds,
                        alt_thresholds);
					break;
                }
                case adaptive_sprt_flavor::unconstrained:
                {
                    asprt_unconstrained_type& rule_unconstrained = this->m_asprt_unconstrained_rules.emplace_back(design);
                    rule_unconstrained.initialize(
                        this->m_model,
                        this->m_init.anticipated_run_length(),
                        this->m_log_likelihood_scale,
                        null_thresholds,
                        alt_thresholds);
					break;
                }
                default:
                    throw std::runtime_error("Adaptive SPRT flavor not recognized.");
            } // switch (...)
        } // operator ()(...)

        void operator ()(const generalized_sprt_design<value_type>& design)
        {
            std::vector<value_type> null_thresholds {};
            std::vector<value_type> alt_thresholds {};
            this->m_init.make_thresholds(this->m_threshold_count, this->m_threshold_spacing, null_thresholds, alt_thresholds);

            switch (design.flavor())
            {
                case generalized_sprt_flavor::cutoff:
                {
                    gsprt_cutoff_type& rule_cutoff = this->m_gsprt_cutoff_rules.emplace_back(design);
                    rule_cutoff.initialize(
                        this->m_model,
                        this->m_init.anticipated_run_length(),
                        this->m_log_likelihood_scale,
                        null_thresholds,
                        alt_thresholds);
					break;
                }
                case generalized_sprt_flavor::general:
                {
                    gsprt_general_type& rule_general = this->m_gsprt_general_rules.emplace_back(design);
                    rule_general.initialize(
                        this->m_model,
                        this->m_init.anticipated_run_length(),
                        this->m_log_likelihood_scale,
                        null_thresholds,
                        alt_thresholds);
					break;
                }
                default:
                    throw std::runtime_error("Generalized SPRT flavor not recognized.");
            } // switch (...)
        } // operator ()(...)

        void operator ()(const double_sprt_design<value_type>& design)
        {
            std::vector<value_type> null_thresholds {};
            std::vector<value_type> alt_thresholds {};
            this->m_init.make_thresholds(this->m_threshold_count, this->m_threshold_spacing, null_thresholds, alt_thresholds);

            dsprt_type& rule = this->m_dsprt_rules.emplace_back(design);
            rule.initialize(
                this->m_model,
                this->m_init.anticipated_run_length(),
                this->m_log_likelihood_scale,
                null_thresholds,
                alt_thresholds);
        } // operator ()(...)
    }; // struct sprt_factory
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_SPRT_FACTORY_HPP_INCLUDED
