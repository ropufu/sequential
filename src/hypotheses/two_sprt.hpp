
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED

#include <ropufu/algebra.hpp>  // aftermath::algebra::matrix
#include <ropufu/on_error.hpp> // aftermath::detail::on_error
#include "../draft/algebra/numbers.hpp"

#include "likelihood.hpp"
#include "model.hpp"
#include "moment_statistic.hpp"
#include "observer.hpp"
#include "process.hpp"

#include <algorithm>   // std::sort
#include <cmath>       // std::exp
#include <cstddef>     // std::size_t
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc
#include <type_traits> // std::is_same
#include <vector>      // std::vector

namespace ropufu::sequential::hypotheses
{
    /** @brief Base class for various versions of 2-SPRT based rules.
     *  @remark The inheriting type must friend the base class \c two_sprt<...>.
     *  @remark The inheriting type is required to implement the following protected functions:
     *          void on_initialized() noexcept
     *          void on_reset_override() noexcept
     *          void on_tic_override(const process<t_signal_type, t_noise_type>&) noexcept
     *  @remark The inheriting type is required to implement the following public functions:
     *          std::string to_path_string(std::size_t) const noexcept
     *          bool do_decide_null(typename t_noise_type::value_type) const noexcept
     *          bool do_decide_alt(typename t_noise_type::value_type) const noexcept
     */
    template <typename t_derived_type, typename t_signal_type, typename t_noise_type, bool t_sync_check = true>
    struct two_sprt : public observer<
        two_sprt<t_derived_type, t_signal_type, t_noise_type, t_sync_check>,
        t_signal_type, t_noise_type, t_sync_check>
    {
        using type = two_sprt<t_derived_type, t_signal_type, t_noise_type, t_sync_check>;
        using base_type = observer<type, t_signal_type, t_noise_type, t_sync_check>;
        friend base_type;

        using signal_type = typename base_type::signal_type;
        using noise_type = typename base_type::noise_type;
        using process_type = typename base_type::process_type;
        using value_type = typename base_type::value_type;
        using model_type = hypotheses::model<value_type>;
        using likelihood_type = hypotheses::likelihood<t_signal_type, t_noise_type, t_sync_check>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        using statistic_type = moment_statistic<matrix_t<value_type>>;

    private:
        using derived_type = t_derived_type;

        // ~~ Fundamental members ~~
        std::size_t m_id = 0;
        likelihood_type m_likelihood = {}; // Reset with each \c toc().
        value_type m_analyzed_mu = 0; // The signal "strength" conrresponding to what measure we want to analyze.
        value_type m_anticipated_run_length = 0; // An auxiliary quantity to improve accuracy of statistics.
        std::vector<value_type> m_unscaled_null_thresholds = {}; // Size-m vector of null thresholds.
        std::vector<value_type> m_unscaled_alt_thresholds = {};  // Size-n vector of alt thresholds.

        // ~~ Members reset with each \c \reset(), but persisting across \c toc() calls ~~
        bool m_is_initialized = false;
        statistic_type m_errors = {}; // Track erroneous decisions made by the stopping time.
        statistic_type m_run_lengths = {}; // Track run lengths of the stopping time.

        // ~~ Members reset with each \c toc() ~~
        bool m_is_listening = false;
        std::size_t m_count = false; // Number of collected observations.
        matrix_t<bool> m_has_decided_null = {}; // m-by-n matrix indicating if the procedure has decided in favor of the null hypothesis.
        matrix_t<bool> m_has_decided_alt = {};  // m-by-n matrix indicating if the procedure has decided in favor of the alternative hypothesis.
        matrix_t<std::size_t> m_counts = {};    // m-by-n matrix counting the number of observations prior to stopping.
        std::size_t m_first_uncrossed_null_index = 0; // Keep track of the first uncrossed null index: there is no need to re-check the smaller ones.
        std::size_t m_first_uncrossed_alt_index = 0;  // Keep track of the first uncrossed alt index: there is no need to re-check the smaller ones.

    private:
        /** @brief Resets the time to zero, but keeps the statistics. */
        void soft_reset() noexcept
        {
            this->m_is_listening = this->m_is_initialized;
            this->m_count = 0;
            this->m_likelihood.reset();

            this->m_has_decided_null.wipe();
            this->m_has_decided_alt.wipe();
            this->m_counts.wipe();
            this->m_first_uncrossed_null_index = 0;
            this->m_first_uncrossed_alt_index = 0;
        } // soft_reset(...)
        
        /** @brief Resets the statistics. */
        void hard_reset() noexcept
        {
            this->soft_reset();
            this->m_is_initialized = false;
            this->m_is_listening = false;

            this->m_unscaled_null_thresholds = std::vector<value_type>(this->m_unscaled_null_thresholds.size());
            this->m_unscaled_alt_thresholds = std::vector<value_type>(this->m_unscaled_alt_thresholds.size());

            this->m_errors.clear();
            this->m_run_lengths.clear();
        } // hard_reset(...)

    protected:
        bool validate(std::error_code& ec) const noexcept
        {
            if (modules::is_nan(this->m_analyzed_mu) || modules::is_infinite(this->m_analyzed_mu)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Analyzed mu has to be a number.", false);
            if (modules::is_nan(this->m_anticipated_run_length) || modules::is_infinite(this->m_anticipated_run_length)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Anticipated run length has to be a number.", false);
            return true;
        } // validate(...)

        void coerce() noexcept
        {
            if (modules::is_nan(this->m_analyzed_mu) || modules::is_infinite(this->m_analyzed_mu)) this->m_analyzed_mu = 0;
            if (modules::is_nan(this->m_anticipated_run_length) || modules::is_infinite(this->m_anticipated_run_length)) this->m_anticipated_run_length = 0;
        } // coerce(...)

    protected:
        two_sprt() noexcept { }
        
        /** Set the signal "strength" conrresponding to what measure we want to analyze and an auxiliary quantity to improve accuracy of statistics. */
        explicit two_sprt(std::size_t id) noexcept : m_id(id) { }

        /** @brief Indicates if the choice of thresholds does not affect other design parameters. */
        bool is_design_threshold_independent() const noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::is_design_threshold_independent),
                decltype(&type::is_design_threshold_independent)>::value;
            static_assert(is_overwritten, "static polymorphic function <is_design_threshold_independent> was not overwritten.");

            const derived_type* that = static_cast<const derived_type*>(this);
            return that->is_design_threshold_independent();
        } // is_design_threshold_independent(...)

        /** @brief Auxiliary function to be executed right after the \c initialize() call. */
        void on_initialized() noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_initialized),
                decltype(&type::on_initialized)>::value;
            static_assert(is_overwritten, "static polymorphic function <on_initialized> was not overwritten.");

            derived_type* that = static_cast<derived_type*>(this);
            that->on_initialized();
        } // on_initialized(...)

        /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
        void on_reset_override() noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_reset_override),
                decltype(&type::on_reset_override)>::value;
            static_assert(is_overwritten, "static polymorphic function <on_reset_override> was not overwritten.");

            derived_type* that = static_cast<derived_type*>(this);
            that->on_reset_override();
        } // on_reset_override(...)

        /** @brief Auxiliary function to be executed right after the \c tic(), but before the \c on_tic() call. */
        void on_tic_override(const process_type& proc) noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_tic_override),
                decltype(&type::on_tic_override)>::value;
            static_assert(is_overwritten, "static polymorphic function <on_tic_override> was not overwritten.");
            
            derived_type* that = static_cast<derived_type*>(this);
            that->on_tic_override(proc);
        } // on_tic_override(...)

        /** @brief Auxiliary function to be executed right before the \c on_toc() call. */
        void on_toc_override(const process_type& proc) noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_toc_override),
                decltype(&type::on_toc_override)>::value;
            static_assert(is_overwritten, "static polymorphic function <on_toc_override> was not overwritten.");
            
            derived_type* that = static_cast<derived_type*>(this);
            that->on_toc_override(proc);
        } // on_toc_override(...)

        /** @brief Auxiliary function to be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            this->on_reset_override();
            this->hard_reset();
        } // on_reset(...)

        /** @brief Auxiliary function to be executed right after the \c tic() call.
         *  @todo Add documentation.
         */
        void on_tic(const process_type& proc, std::error_code& ec) noexcept
        {
            if (proc.empty()) return; // Do nothing if the process hasn't collected any observations.
            if (this->has_stopped()) return; // Do nothing if the process hasn't collected any observations.

            if (!this->m_is_initialized) aftermath::detail::on_error(ec, std::errc::function_not_supported, "Decision rule has not been initialized.");
            if (proc.time() != this->m_count) aftermath::detail::on_error(ec, std::errc::function_not_supported, "Process and stopping time are out of sync.");
            if (ec.value() != 0) return;

            this->m_likelihood.tic(proc, ec);
            this->on_tic_override(proc);  // Make sure the override goes before the \c do_decide_null(...) and \c do_decide_alt(...) are called.
            this->m_count = proc.count();

            // Null thresholds: a.
            // Alt thresholds: b.
            //
            //         |  0    1   ...   n-1    | b (alt) 
            // --------|------------------------|         
            //     0   |           ...          |         
            //     1   |           ...          |         
            //    ...  |           ...          |         
            //    m-1  |           ...          |         
            // ----------------------------------         
            //  a (null)                                  
            //
            
            std::size_t m = this->m_unscaled_null_thresholds.size(); // Height of the threshold matrix.
            std::size_t n = this->m_unscaled_alt_thresholds.size(); // Width of the threshold matrix.

            // Optimize when the design parameters are threshold-independent.
            if (this->is_design_threshold_independent())
            {
                // Traverse null thresholds (vertical).
                std::size_t next_uncrossed_null_index = this->m_first_uncrossed_null_index;
                for (std::size_t i = this->m_first_uncrossed_null_index; i < m; ++i)
                {
                    value_type a = this->m_unscaled_null_thresholds[i];
                    if (!this->do_decide_null(a)) break; // Break the loop the first time the null hypothesis is not accepted.
                    else
                    {
                        next_uncrossed_null_index = i + 1;
                        for (std::size_t j = this->m_first_uncrossed_alt_index; j < n; ++j)
                        {
                            this->m_has_decided_null(i, j) = true; // Mark threshold as crossed.
                            this->m_counts(i, j) = this->m_count; // Record the freshly stopped times.
                        } // for (...)
                    } // else (...)
                } // for (...)

                // Traverse alt thresholds (horizontal).
                std::size_t next_uncrossed_alt_index = this->m_first_uncrossed_alt_index;
                for (std::size_t j = this->m_first_uncrossed_alt_index; j < n; ++j)
                {
                    value_type b = this->m_unscaled_alt_thresholds[j];
                    if (!this->do_decide_alt(b)) break; // Break the loop the first time the alternative hypothesis is not accepted.
                    else
                    {
                        next_uncrossed_alt_index = j + 1;
                        for (std::size_t i = this->m_first_uncrossed_null_index; i < m; ++i)
                        {
                            this->m_has_decided_alt(i, j) = true; // Mark threshold as crossed.
                            this->m_counts(i, j) = this->m_count; // Record the freshly stopped times.
                        } // for (...)
                    } // else (...)
                } // for (...)

                this->m_first_uncrossed_null_index = next_uncrossed_null_index;
                this->m_first_uncrossed_alt_index = next_uncrossed_alt_index;
                this->m_is_listening = (this->m_first_uncrossed_null_index < m) && (this->m_first_uncrossed_alt_index < n);
            } // if (...)
            else
            {

            } // else (...)
        } // on_tic(...)

        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& proc, std::error_code& ec) noexcept
        {
            this->on_toc_override(proc);

            const matrix_t<std::size_t>& run_lengths = this->m_counts;
            const matrix_t<bool>& have_crossed_null = this->m_has_decided_null;
            const matrix_t<bool>& have_crossed_alt = this->m_has_decided_alt;

            bool is_null_true = this->m_likelihood.model().is_null(this->m_analyzed_mu);
            bool is_alt_true = this->m_likelihood.model().is_alt(this->m_analyzed_mu);

            // Count the thresholds.
            std::size_t m = run_lengths.height();
            std::size_t n = run_lengths.width();

            matrix_t<value_type> corrected_run_lengths(m, n);
            matrix_t<value_type> corrected_errors(m, n);
            for (std::size_t i = 0; i < m; ++i)
            {
                for (std::size_t j = 0; j < n; ++j)
                {
                    bool has_crossed_null = have_crossed_null(i, j);
                    bool has_crossed_alt = have_crossed_alt(i, j);
                    // Check if the stopping time has stopped!
                    if (!has_crossed_null && !has_crossed_alt)
                    {
                        aftermath::detail::on_error(ec, std::errc::function_not_supported, "The procedure has not stopped! Resetting.");
                        this->soft_reset(); // If not, reset time anyway (but keep the statistics).
                        return;
                    } // if (...)
            
                    std::size_t run_length = run_lengths(i, j);
                    std::size_t error = 0;
                    if (has_crossed_null && has_crossed_alt) error = 1;
                    if (has_crossed_null && is_alt_true) error = 1;
                    if (has_crossed_alt && is_null_true) error = 1;
                    //std::size_t error_relaxed = has_crossed_null ? (is_null_true ? 0 : 1) : (is_null_true ? 1 : 0);
            
                    /* @todo Check that \c run_length is not zero. */
                    value_type correction = std::exp(proc.unscaled_log_likelihood_between(proc.actual_mu(), this->m_analyzed_mu, run_length - 1) / proc.log_likelihood_scale());
                    value_type t = run_length / correction;
                    value_type e = error / correction;
                    corrected_run_lengths(i, j) = t;
                    corrected_errors(i, j) = e;
                } // for(...)
            } // for(...)

            this->m_run_lengths.observe(corrected_run_lengths);
            this->m_errors.observe(corrected_errors);
            this->soft_reset(); // Reset time, but keep the statistics.
        } // on_toc(...)

        const matrix_t<std::size_t>& counts() const noexcept { return this->m_counts; }

        const matrix_t<bool>& has_decided_null() const noexcept { return this->m_has_decided_null; }

        const matrix_t<bool>& has_decided_alt() const noexcept { return this->m_has_decided_alt; }

        void set_id(std::size_t value) noexcept { this->m_id = value; }

    public:
        std::size_t id() const noexcept { return this->m_id; }

        const likelihood_type& likelihood() const noexcept { return this->m_likelihood; }

        bool is_listening() const noexcept { return this->m_is_listening; }
        bool has_stopped() const noexcept { return !this->m_is_listening; }
        
        /** Signal "strength" conrresponding to what measure we want to analyze. */
        value_type analyzed_mu() const noexcept { return this->m_analyzed_mu; }

        /** An auxiliary quantity to improve accuracy of statistics. */
        value_type anticipated_run_length() const noexcept { return this->m_anticipated_run_length; }

        /** Track erroneous decisions made by the stopping time. */
        const statistic_type& errors() const noexcept { return this->m_errors; }

        /** Track run lengths of the stopping time. */
        const statistic_type& run_lengths() const noexcept { return this->m_run_lengths; }

        const std::vector<value_type>& unscaled_null_thresholds() const noexcept { return this->m_unscaled_null_thresholds; }
        const std::vector<value_type>& unscaled_alt_thresholds() const noexcept { return this->m_unscaled_alt_thresholds; }

        std::string to_path_string(std::size_t decimal_places) const noexcept
        {
            constexpr bool is_overwritten = !std::is_same<decltype(&derived_type::to_path_string), decltype(&type::to_path_string)>::value;
            static_assert(is_overwritten, "static polymorphic function <to_path_string> was not overwritten.");
            
            const derived_type* that = static_cast<const derived_type*>(this);
            return that->to_path_string(decimal_places);
        } // do_decide_alt(...)

        bool do_decide_null(value_type threshold) const noexcept
        {
            constexpr bool is_overwritten = !std::is_same<decltype(&derived_type::do_decide_null), decltype(&type::do_decide_null)>::value;
            static_assert(is_overwritten, "static polymorphic function <do_decide_null> was not overwritten.");

            const derived_type* that = static_cast<const derived_type*>(this);
            return that->do_decide_null(threshold);
        } // do_decide_null(...)

        bool do_decide_alt(value_type threshold) const noexcept
        {
            constexpr bool is_overwritten = !std::is_same<decltype(&derived_type::do_decide_alt), decltype(&type::do_decide_alt)>::value;
            static_assert(is_overwritten, "static polymorphic function <do_decide_alt> was not overwritten.");
            
            const derived_type* that = static_cast<const derived_type*>(this);
            return that->do_decide_alt(threshold);
        } // do_decide_alt(...)

        /** @remark Thresholds have to be of the same size; they are independently(!) sorted and then paired up. */
        void initialize(const model_type& model, value_type analyzed_mu, value_type anticipated_run_length,
            const process_type& proc,
            const std::vector<value_type>& null_thresholds,
            const std::vector<value_type>& alt_thresholds,
            std::error_code& ec) noexcept
        {
            if (this->m_is_initialized)  aftermath::detail::on_error(ec, std::errc::function_not_supported, "Thresholds have to be set prior to first observation.");
            if (null_thresholds.empty()) aftermath::detail::on_error(ec, std::errc::function_not_supported, "At least one null threshold has to be specified.");
            if (alt_thresholds.empty())  aftermath::detail::on_error(ec, std::errc::function_not_supported, "At least one alt threshold has to be specified.");

            bool are_null_bad = false;
            bool are_alt_bad = false;
            for (const value_type& a : null_thresholds) if (modules::is_nan(a) || modules::is_infinite(a)) are_null_bad = true;
            for (const value_type& b : alt_thresholds) if (modules::is_nan(b) || modules::is_infinite(b)) are_alt_bad = true;
            if (are_null_bad) aftermath::detail::on_error(ec, std::errc::function_not_supported, "Null thresholds must be finite numbers.");
            if (are_alt_bad) aftermath::detail::on_error(ec, std::errc::function_not_supported, "Alt thresholds must be finite numbers.");

            if (ec.value() != 0) return; // Return on error.

            this->m_likelihood = likelihood_type(model);
            this->m_analyzed_mu = analyzed_mu;
            this->m_anticipated_run_length = anticipated_run_length;
            if (!this->validate(ec)) this->coerce();

            std::size_t m = null_thresholds.size(); // Height of the threshold matrix.
            std::size_t n = alt_thresholds.size(); // Width of the threshold matrix.

            this->m_has_decided_null = matrix_t<bool>(m, n); // Align cross indicators to thresholds.
            this->m_has_decided_alt = matrix_t<bool>(m, n); // Align cross indicators to thresholds.
            this->m_counts = matrix_t<size_t>(m, n); // Align observation counts to thresholds.

            this->m_unscaled_null_thresholds = null_thresholds;
            this->m_unscaled_alt_thresholds = alt_thresholds;
            // ~~ Sort the thresholds ~~
            std::sort(this->m_unscaled_null_thresholds.begin(), this->m_unscaled_null_thresholds.end());
            std::sort(this->m_unscaled_alt_thresholds.begin(), this->m_unscaled_alt_thresholds.end());
            // ~~ Rescale ~~
            value_type factor = proc.log_likelihood_scale(); // @todo Think, can we get rid of \c proc, and move the scale elsewhere?
            for (value_type& a : this->m_unscaled_null_thresholds) a *= factor;
            for (value_type& b : this->m_unscaled_alt_thresholds) b *= factor;
            
            // Now that the stopping time has verified the threshold structure, go on and resize the empirical measures accordingly.
            matrix_t<value_type> zero(m, n);
            matrix_t<value_type> anticipated_mean(m, n, this->m_anticipated_run_length);

            this->m_errors = statistic_type(zero, zero); // Align cross indicators to match threshold size.
            this->m_run_lengths = statistic_type(zero, anticipated_mean); // Align observation counts to match threshold size.

            // Finish up.
            if (ec.value() != 0) return;
            this->on_initialized();
            this->m_is_initialized = true;
            this->m_is_listening = true;
        } // initialize(...)
    }; // struct two_sprt
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
