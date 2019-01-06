
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_KNOWN_SPRTS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_KNOWN_SPRTS_HPP_INCLUDED

#include <ropufu/algebra.hpp>
#include <ropufu/on_error.hpp> // aftermath::detail::on_error

#include "../hypotheses/model.hpp"
#include "../hypotheses/moment_statistic.hpp"
#include "../hypotheses/observer.hpp"
#include "../hypotheses/process.hpp"
#include "../hypotheses/rules.hpp"

#include "hypothesis_pair.hpp"
#include "init_info.hpp"
#include "operating_characteristic.hpp"
#include "simulation_pair.hpp"
#include "writer.hpp"

#include <cstddef> // std::size_t
#include <string>  // std::string
#include <system_error> // std::error_code, std::errc
#include <variant> // std::variant, std::get_if
#include <vector>  // std::vector

namespace ropufu::sequential::hypotheses
{
    /** Represents a collection of SPRT-based rule observers. */
    template <typename t_signal_type, typename t_noise_type>
    struct known_sprts;

    template <typename t_process_type>
    using known_sprts_t = known_sprts<typename t_process_type::signal_type, typename t_process_type::noise_type>;

    /** Represents a collection of SPRT-based rule observers. */
    template <typename t_signal_type, typename t_noise_type>
    struct known_sprts : public observer<
        known_sprts<t_signal_type, t_noise_type>,
        t_signal_type, t_noise_type>
    {
        using type = known_sprts<t_signal_type, t_noise_type>;
        using base_type = observer<type, t_signal_type, t_noise_type>;
        friend base_type;

        using signal_type = typename base_type::signal_type;
        using noise_type = typename base_type::noise_type;
        using process_type = typename base_type::process_type;
        using value_type = typename base_type::value_type;
        using init_type = init_info<value_type>;
        using model_type = model<value_type>;
        using moment_statistic_type = moment_statistic<aftermath::algebra::matrix<value_type>>;
        using oc_statistic_type = oc_array_t<moment_statistic_type>;

        // ~~ Supported 2-SPRT types ~~
        using type_0 = adaptive_sprt_a<t_signal_type, t_noise_type>;
        using type_1 = adaptive_sprt_b<t_signal_type, t_noise_type>;
        using type_2 = double_sprt<t_signal_type, t_noise_type>;
        using type_3 = generalized_sprt_a<t_signal_type, t_noise_type>;
        using type_4 = generalized_sprt_b<t_signal_type, t_noise_type>;

    private:
        std::vector<type_0> m_rules_0 = {};
        std::vector<type_1> m_rules_1 = {};
        std::vector<type_2> m_rules_2 = {};
        std::vector<type_3> m_rules_3 = {};
        std::vector<type_4> m_rules_4 = {};
        std::vector<oc_statistic_type> m_oc_statistics = {};

        template <typename t_rule_type>
        static void copy(
            const std::vector<t_rule_type>& from_rules,
            std::vector<t_rule_type>& to_rules,
            const std::vector<init_type>& init_list) noexcept
        {
            for (const t_rule_type& r : from_rules)
            {
                for (const init_type& init : init_list)
                {
                    if (init.rule_id() != r.id()) continue;
                    to_rules.push_back(r);
                    break;
                } // for (...)
            } // for (...)
        } // copy(...)

        template <typename t_rule_type>
        static void initialize(std::vector<t_rule_type>& rules, const init_type& init,
            const model_type& model, value_type analyzed_mu, value_type log_likelihood_scale,
            const std::vector<value_type>& null_thresholds,
            const std::vector<value_type>& alt_thresholds,
            std::error_code& ec) noexcept
        {
            for (t_rule_type& r : rules)
            {
                if (init.rule_id() != r.id()) continue;
                r.initialize(model,
                    analyzed_mu,
                    init.anticipated_run_length(),
                    log_likelihood_scale, 
                    null_thresholds, alt_thresholds, ec);
                return;
            } // for (...)
        } // initialize(...)

    protected:
        /** @brief Auxiliary function to be executed right before the \c reset() call.
         *  @remark \c this->m_oc_statistics is deliberately not reset.
         */
        void on_reset() noexcept
        {
            for (type_0& r : this->m_rules_0) r.reset();
            for (type_1& r : this->m_rules_1) r.reset();
            for (type_2& r : this->m_rules_2) r.reset();
            for (type_3& r : this->m_rules_3) r.reset();
            for (type_4& r : this->m_rules_4) r.reset();
        } // on_reset(...)

        /** @brief Auxiliary function to be executed right after the \c tic() call. */
        void on_tic(const process_type& proc, std::error_code& ec) noexcept
        {
            for (type_0& r : this->m_rules_0) r.tic(proc, ec);
            for (type_1& r : this->m_rules_1) r.tic(proc, ec);
            for (type_2& r : this->m_rules_2) r.tic(proc, ec);
            for (type_3& r : this->m_rules_3) r.tic(proc, ec);
            for (type_4& r : this->m_rules_4) r.tic(proc, ec);
        } // on_tic(...)
        
        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& proc, std::error_code& ec) noexcept
        {
            for (type_0& r : this->m_rules_0) r.toc(proc, ec);
            for (type_1& r : this->m_rules_1) r.toc(proc, ec);
            for (type_2& r : this->m_rules_2) r.toc(proc, ec);
            for (type_3& r : this->m_rules_3) r.toc(proc, ec);
            for (type_4& r : this->m_rules_4) r.toc(proc, ec);
        } // on_toc(...)

    public:
        known_sprts() noexcept { }

        template <typename... t_design_types>
        known_sprts(const std::vector<std::variant<t_design_types...>>& rule_designs, std::error_code& ec) noexcept
        {
            for (const std::variant<t_design_types...>& v : rule_designs)
            {
                std::size_t sz = this->size() + 1;

                if (const typename type_0::design_type* design_ptr = std::get_if<typename type_0::design_type>(&v)) this->m_rules_0.emplace_back(*design_ptr);
                if (const typename type_1::design_type* design_ptr = std::get_if<typename type_1::design_type>(&v)) this->m_rules_1.emplace_back(*design_ptr);
                if (const typename type_2::design_type* design_ptr = std::get_if<typename type_2::design_type>(&v)) this->m_rules_2.emplace_back(*design_ptr);
                if (const typename type_3::design_type* design_ptr = std::get_if<typename type_3::design_type>(&v)) this->m_rules_3.emplace_back(*design_ptr);
                if (const typename type_4::design_type* design_ptr = std::get_if<typename type_4::design_type>(&v)) this->m_rules_4.emplace_back(*design_ptr);

                if (sz != this->size()) aftermath::detail::on_error(ec, std::errc::invalid_argument, "Rule design not recognized.");
            } // for (...)
            this->m_oc_statistics.resize(this->size()); // One default-initialized <oc_array_t> for each rule.
        } // known_sprts(...)

        type filter(const std::vector<init_type>& init_list) const noexcept
        {
            type result {};

            type::copy(this->m_rules_0, result.m_rules_0, init_list);
            type::copy(this->m_rules_1, result.m_rules_1, init_list);
            type::copy(this->m_rules_2, result.m_rules_2, init_list);
            type::copy(this->m_rules_3, result.m_rules_3, init_list);
            type::copy(this->m_rules_4, result.m_rules_4, init_list);

            result.m_oc_statistics.resize(result.size()); // One default-initialized <oc_array_t> for each rule.
            return result;
        } // filter(...)

        const std::vector<oc_statistic_type>& oc_statistics() const noexcept { return this->m_oc_statistics; }

        void initialize(const init_type& init,
            const model_type& model, value_type analyzed_mu, value_type log_likelihood_scale,
            const std::vector<value_type>& null_thresholds,
            const std::vector<value_type>& alt_thresholds,
            std::error_code& ec) noexcept
        {
            type::initialize(this->m_rules_0, init, model, analyzed_mu, log_likelihood_scale, null_thresholds, alt_thresholds, ec);
            type::initialize(this->m_rules_1, init, model, analyzed_mu, log_likelihood_scale, null_thresholds, alt_thresholds, ec);
            type::initialize(this->m_rules_2, init, model, analyzed_mu, log_likelihood_scale, null_thresholds, alt_thresholds, ec);
            type::initialize(this->m_rules_3, init, model, analyzed_mu, log_likelihood_scale, null_thresholds, alt_thresholds, ec);
            type::initialize(this->m_rules_4, init, model, analyzed_mu, log_likelihood_scale, null_thresholds, alt_thresholds, ec);
        } // filter(...)

        /** @brief Reports the result of simulations.
         *  @param handle Has to implement operator ()(size_type id, const moment_statistic& run_lengths, const moment_statistic& errors) -> void.
         */
        template <typename t_statistics_handler_type>
        void report(t_statistics_handler_type&& handle) const noexcept
        {
            for (const type_0& r : this->m_rules_0) handle(r.id(), r.run_lengths(), r.errors());
            for (const type_1& r : this->m_rules_1) handle(r.id(), r.run_lengths(), r.errors());
            for (const type_2& r : this->m_rules_2) handle(r.id(), r.run_lengths(), r.errors());
            for (const type_3& r : this->m_rules_3) handle(r.id(), r.run_lengths(), r.errors());
            for (const type_4& r : this->m_rules_4) handle(r.id(), r.run_lengths(), r.errors());
        } // report(...)

        /** Collect a portion of OC statistics. */
        void record_oc(operating_characteristic oc, const simulation_pair<value_type>& mu_pair, std::error_code& ec) noexcept
        {
            std::size_t k = 0;
            for (type_0& r : this->m_rules_0) { oc_statistic_type& s = this->m_oc_statistics[k++]; s[oc] = mu_pair.read_oc(oc, r, ec); }
            for (type_1& r : this->m_rules_1) { oc_statistic_type& s = this->m_oc_statistics[k++]; s[oc] = mu_pair.read_oc(oc, r, ec); }
            for (type_2& r : this->m_rules_2) { oc_statistic_type& s = this->m_oc_statistics[k++]; s[oc] = mu_pair.read_oc(oc, r, ec); }
            for (type_3& r : this->m_rules_3) { oc_statistic_type& s = this->m_oc_statistics[k++]; s[oc] = mu_pair.read_oc(oc, r, ec); }
            for (type_4& r : this->m_rules_4) { oc_statistic_type& s = this->m_oc_statistics[k++]; s[oc] = mu_pair.read_oc(oc, r, ec); }
        } // record_oc(...)

        void dump_oc(writer<value_type>& w, const model_type& model, std::error_code& ec) const noexcept
        {
            std::size_t k = 0;
            for (const type_0& r : this->m_rules_0) { const oc_statistic_type& s = this->m_oc_statistics[k++]; w.write_mat(model, r, s, ec); }
            for (const type_1& r : this->m_rules_1) { const oc_statistic_type& s = this->m_oc_statistics[k++]; w.write_mat(model, r, s, ec); }
            for (const type_2& r : this->m_rules_2) { const oc_statistic_type& s = this->m_oc_statistics[k++]; w.write_mat(model, r, s, ec); }
            for (const type_3& r : this->m_rules_3) { const oc_statistic_type& s = this->m_oc_statistics[k++]; w.write_mat(model, r, s, ec); }
            for (const type_4& r : this->m_rules_4) { const oc_statistic_type& s = this->m_oc_statistics[k++]; w.write_mat(model, r, s, ec); }
        } // dump_oc(...)

        void dump(writer<value_type>& w, const model_type& model, const simulation_pair<value_type>& mu_pair, std::error_code& ec) const noexcept
        {
            for (const type_0& r : this->m_rules_0) w.write_mat(model, r, mu_pair, ec);
            for (const type_1& r : this->m_rules_1) w.write_mat(model, r, mu_pair, ec);
            for (const type_2& r : this->m_rules_2) w.write_mat(model, r, mu_pair, ec);
            for (const type_3& r : this->m_rules_3) w.write_mat(model, r, mu_pair, ec);
            for (const type_4& r : this->m_rules_4) w.write_mat(model, r, mu_pair, ec);
        } // dump_oc(...)

        std::size_t size() const noexcept
        {
            return this->m_rules_0.size() +
                this->m_rules_1.size() +
                this->m_rules_2.size() +
                this->m_rules_3.size() +
                this->m_rules_4.size();
        } // size(...)

        /** Indicates if at least one SPRT in the collection is listening. */
        bool is_listening() const noexcept
        {
            for (const type_0& r : this->m_rules_0) if (r.is_listening()) return true;
            for (const type_1& r : this->m_rules_1) if (r.is_listening()) return true;
            for (const type_2& r : this->m_rules_2) if (r.is_listening()) return true;
            for (const type_3& r : this->m_rules_3) if (r.is_listening()) return true;
            for (const type_4& r : this->m_rules_4) if (r.is_listening()) return true;
            return false;
        } // is_listening(...)

        /** Indicates if all of the SPRT's in the collection have stopped. */
        bool has_stopped() const noexcept
        {
            for (const type_0& r : this->m_rules_0) if (!r.has_stopped()) return false;
            for (const type_1& r : this->m_rules_1) if (!r.has_stopped()) return false;
            for (const type_2& r : this->m_rules_2) if (!r.has_stopped()) return false;
            for (const type_3& r : this->m_rules_3) if (!r.has_stopped()) return false;
            for (const type_4& r : this->m_rules_4) if (!r.has_stopped()) return false;
            return true;
        } // has_stopped(...)
    }; // struct known_sprts
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_KNOWN_SPRTS_HPP_INCLUDED