
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/number_traits.hpp>

#include "../../draft/algebra/matrix_mask.hpp"
#include "../../draft/probability/moment_statistic.hpp"

#include "../likelihood.hpp"
#include "../model.hpp"
#include "../observer.hpp"
#include "../simple_process.hpp"
#include "../change_of_measure.hpp"

#include <algorithm>   // std::sort
#include <cmath>       // std::exp
#include <cstddef>     // std::size_t
#include <stdexcept>    // std::logic_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc
#include <type_traits> // std::is_same
#include <vector>      // std::vector

namespace ropufu::sequential::hypotheses
{
    enum struct two_sprt_state : char
    {
        uninitialized = 0, // Waiting to be initialized.
        listening = 1, // Collecting process observations.
        decided = 2, // Has arrived at the decision.
        finalized = 3 // Has completed a simulation cycle.
    }; // struct two_sprt_state

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
    template <typename t_derived_type, typename t_engine_type, typename t_value_type>
    struct two_sprt : observer<t_engine_type, t_value_type>
    {
        using type = two_sprt<t_derived_type, t_engine_type, t_value_type>;
        using derived_type = t_derived_type;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using model_type = model<value_type>;
        using likelihood_type = likelihood<value_type>;
        using process_type = simple_process<engine_type, value_type>;
        using change_of_measure_type = change_of_measure<value_type>;

        using matrix_mask_type = aftermath::algebra::matrix_mask<std::size_t>;
        using matrix_cell_type = aftermath::algebra::sparse_matrix_cell<std::size_t, bool>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        using moment_statistic_type = aftermath::probability::moment_statistic<matrix_t<value_type>>;

    private:
        // ~~ Timer-related ~~
        two_sprt_state m_state = two_sprt_state::uninitialized;
        std::size_t m_count_tics = 0; // Number of tics up to this moment.

        // ~~ Fundamental members ~~
        std::size_t m_id = 0;
        value_type m_anticipated_run_length = 0; // An auxiliary quantity to improve accuracy of statistics.
        std::vector<value_type> m_unscaled_null_thresholds = {}; // Size-m vector of null thresholds.
        std::vector<value_type> m_unscaled_alt_thresholds = {};  // Size-n vector of alt thresholds.

        // ~~ Members reset with each \c \reset(), but persisting across \c toc() calls ~~
        moment_statistic_type m_decision_errors = {}; // Track erroneous decisions made by the stopping time.
        moment_statistic_type m_run_lengths = {}; // Track run lengths of the stopping time.

        // ~~ Members reset with each \c toc() ~~
        matrix_mask_type m_thresholds_mask = {}; // List of thresholds where the decision has not yet been made.
        matrix_t<bool> m_has_decided_null = {};  // m-by-n matrix indicating if the procedure has decided in favor of the null hypothesis.
        matrix_t<bool> m_has_decided_alt = {};   // m-by-n matrix indicating if the procedure has decided in favor of the alternative hypothesis.
        matrix_t<std::size_t> m_run_length = {}; // m-by-n matrix counting the number of observations prior to stopping.
        std::size_t m_first_uncrossed_null_index = 0; // Keep track of the first uncrossed null index: there is no need to re-check the smaller ones.
        std::size_t m_first_uncrossed_alt_index = 0;  // Keep track of the first uncrossed alt index: there is no need to re-check the smaller ones.

        /** @brief Resets the timer to zero, but keeps the run length & error statistics. */
        void reset_timer() noexcept
        {
            this->m_count_tics = 0;
            this->m_thresholds_mask.wipe();

            this->m_has_decided_null.wipe();
            this->m_has_decided_alt.wipe();
            this->m_run_length.wipe();
            this->m_first_uncrossed_null_index = 0;
            this->m_first_uncrossed_alt_index = 0;
        } // reset_timer(...)

        /** @brief Resets the run length & error statistics. */
        void reset_statistics() noexcept
        {
            this->m_decision_errors.clear();
            this->m_run_lengths.clear();
        } // reset_statistics(...)

    protected:
        two_sprt() noexcept { }
        
        explicit two_sprt(std::size_t id) noexcept : m_id(id) { }

        /** @brief Indicates if the choice of thresholds does not affect other design parameters. */
        bool is_design_threshold_independent() const noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::is_design_threshold_independent),
                decltype(&type::is_design_threshold_independent)>::value;
            static_assert(is_overwritten, "is_design_threshold_independent() const noexcept -> bool was not overwritten.");

            const derived_type* that = static_cast<const derived_type*>(this);
            return that->is_design_threshold_independent();
        } // is_design_threshold_independent(...)

        /** @brief Auxiliary function to be executed right after the \c initialize() call. */
        void on_initialized(const model_type& model,
            const std::vector<value_type>& unscaled_null_thresholds,
            const std::vector<value_type>& unscaled_alt_thresholds) noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_initialized),
                decltype(&type::on_initialized)>::value;
            static_assert(is_overwritten, "on_initialized(...) noexcept -> void was not overwritten.");

            derived_type* that = static_cast<derived_type*>(this);
            that->on_initialized(model, unscaled_null_thresholds, unscaled_alt_thresholds);
        } // on_initialized(...)

        /** @brief Auxiliary function to be executed right before the \c reset() call. */
        void on_reset() noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_reset),
                decltype(&type::on_reset)>::value;
            static_assert(is_overwritten, "on_reset() noexcept -> void was not overwritten.");

            derived_type* that = static_cast<derived_type*>(this);
            that->on_reset();
        } // on_reset(...)

        /** @brief Auxiliary function to be executed right after the \c tic() call. */
        void on_tic(const process_type& proc, const likelihood_type& likelihood)
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_tic),
                decltype(&type::on_tic)>::value;
            static_assert(is_overwritten, "on_tic(...) -> void was not overwritten.");
            
            derived_type* that = static_cast<derived_type*>(this);
            that->on_tic(proc, likelihood);
        } // on_tic(...)

        /** @brief Auxiliary function to be executed right before the \c toc() call. */
        void on_toc(const process_type& proc, const likelihood_type& likelihood)
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::on_toc),
                decltype(&type::on_toc)>::value;
            static_assert(is_overwritten, "on_toc(...) -> void was not overwritten.");
            
            derived_type* that = static_cast<derived_type*>(this);
            that->on_toc(proc, likelihood);
        } // on_toc(...)

        bool do_decide_null(value_type threshold, std::size_t row_index, std::size_t column_index) const noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::do_decide_null),
                decltype(&type::do_decide_null)>::value;
            static_assert(is_overwritten, "do_decide_null(...) const -> bool was not overwritten.");

            const derived_type* that = static_cast<const derived_type*>(this);
            return that->do_decide_null(threshold, row_index, column_index);
        } // do_decide_null(...)

        bool do_decide_alt(value_type threshold, std::size_t row_index, std::size_t column_index) const noexcept
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::do_decide_alt),
                decltype(&type::do_decide_alt)>::value;
            static_assert(is_overwritten, "do_decide_alt(...) const noexcept -> bool was not overwritten.");
            
            const derived_type* that = static_cast<const derived_type*>(this);
            return that->do_decide_alt(threshold, row_index, column_index);
        } // do_decide_alt(...)

        const matrix_t<std::size_t>& run_length() const noexcept { return this->m_run_length; }

        const matrix_t<bool>& has_decided_null() const noexcept { return this->m_has_decided_null; }

        const matrix_t<bool>& has_decided_alt() const noexcept { return this->m_has_decided_alt; }

        void set_id(std::size_t value) noexcept { this->m_id = value; }

    public:
        // void uninitialize() noexcept
        // {
        //     this->reset_timer();
        //     this->reset_statistics();

        //     this->m_unscaled_null_thresholds.clear();
        //     this->m_unscaled_alt_thresholds.clear();

        //     this->m_state = two_sprt_state::uninitialized;
        // } // uninitialize(...)

        /** @remark Thresholds are independently sorted and then paired up to create a grid. */
        void initialize(const model_type& model, value_type anticipated_run_length,
            value_type log_likelihood_scale,
            const std::vector<value_type>& null_thresholds,
            const std::vector<value_type>& alt_thresholds)
        {
            if (this->m_state != two_sprt_state::uninitialized) throw std::logic_error("Initialization can only be performed once.");
            
            // ~~ Validate arguments ~~
            if (null_thresholds.empty()) throw std::invalid_argument("Null thresholds cannot be empty.");
            if (alt_thresholds.empty()) throw std::invalid_argument("Alt threshold cannot be empty.");
            
            if (!aftermath::is_finite(anticipated_run_length)) throw std::logic_error("Anticipated run length must be finite.");
            if (!aftermath::is_finite(log_likelihood_scale)) throw std::logic_error("Log-likelihood scale must be finite.");
            if (anticipated_run_length < 0) throw std::logic_error("Anticipated run length must be zero or positive.");
            if (log_likelihood_scale <= 0) throw std::logic_error("Log-likelihood scale must be positive.");

            for (value_type x : null_thresholds) if (!aftermath::is_finite(x)) throw std::logic_error("Null thresholds mu must be finite.");
            for (value_type x : alt_thresholds) if (!aftermath::is_finite(x)) throw std::logic_error("Alt thresholds mu must be finite.");

            // ~~ Store values ~~
            this->m_anticipated_run_length = anticipated_run_length;

            std::size_t m = null_thresholds.size(); // Height of the threshold matrix.
            std::size_t n = alt_thresholds.size(); // Width of the threshold matrix.

            this->m_has_decided_null = matrix_t<bool>(m, n); // Align cross indicators to thresholds.
            this->m_has_decided_alt = matrix_t<bool>(m, n); // Align cross indicators to thresholds.
            this->m_run_length = matrix_t<size_t>(m, n); // Align observation counts to thresholds.

            this->m_unscaled_null_thresholds = null_thresholds;
            this->m_unscaled_alt_thresholds = alt_thresholds;
            // ~~ Sort the thresholds ~~
            std::sort(this->m_unscaled_null_thresholds.begin(), this->m_unscaled_null_thresholds.end());
            std::sort(this->m_unscaled_alt_thresholds.begin(), this->m_unscaled_alt_thresholds.end());
            // ~~ Rescale ~~
            if (log_likelihood_scale != 1)
            {
                for (value_type& a : this->m_unscaled_null_thresholds) a *= log_likelihood_scale;
                for (value_type& b : this->m_unscaled_alt_thresholds) b *= log_likelihood_scale;
            } // if (...)
            this->m_thresholds_mask = matrix_mask_type(m, n);
            
            // Now that the stopping time has verified the threshold structure, go on and resize the empirical measures accordingly.
            matrix_t<value_type> zero {m, n};
            matrix_t<value_type> anticipated_mean {m, n, this->m_anticipated_run_length};

            this->m_decision_errors = moment_statistic_type(zero); // Align cross indicators to match threshold size.
            this->m_run_lengths = moment_statistic_type(anticipated_mean); // Align observation counts to match threshold size.

            // Finish up.
            this->on_initialized(model, this->m_unscaled_null_thresholds, this->m_unscaled_alt_thresholds);
            this->m_state = two_sprt_state::finalized;
        } // initialize(...)

        std::size_t id() const noexcept { return this->m_id; }

        two_sprt_state state() const noexcept { return this->m_state; }
        //bool is_listening() const noexcept { return this->m_state == two_sprt_state::listening; }
        
        /** An auxiliary quantity to improve accuracy of statistics. */
        value_type anticipated_run_length() const noexcept { return this->m_anticipated_run_length; }

        /** Track erroneous decisions made by the stopping time. */
        const moment_statistic_type& decision_errors() const noexcept override { return this->m_decision_errors; }

        /** Track run lengths of the stopping time. */
        const moment_statistic_type& run_lengths() const noexcept override { return this->m_run_lengths; }

        const std::vector<value_type>& unscaled_null_thresholds() const noexcept override { return this->m_unscaled_null_thresholds; }
        const std::vector<value_type>& unscaled_alt_thresholds() const noexcept override { return this->m_unscaled_alt_thresholds; }
        
        /** @brief Indicates if the observer is listening to tic's. */
        bool is_listening() const noexcept override
        {
            return this->m_state == two_sprt_state::listening;
        } // is_listening(...)

        /** @brief Resets the timer and all statistics. Should preceed any simulations. */
        void clean_up() override
        {
            if (this->m_state != two_sprt_state::finalized) throw std::logic_error("Decision rule must be in the finalized state.");
            this->reset_timer();
            this->reset_statistics();
            // this->m_state = two_sprt_state::finalized; // State stays finalized.
        } // clean_up(...)

        /** @brief Prepares the rule for another simulation pass to zero but keeps all statistics. */
        void reset() override
        {
            if (this->m_state != two_sprt_state::finalized) throw std::logic_error("Decision rule must be in the finalized state.");
            this->on_reset();
            this->reset_timer();
            this->m_state = two_sprt_state::listening;
        } // reset(...)

        /** @brief Updates the rule and statistics with the newest observation from \p proc. */
        void tic(const process_type& proc, const likelihood_type& likelihood) override
        {
            switch (this->m_state)
            {
                case two_sprt_state::listening: break; // This is an expected state: continue.
                case two_sprt_state::decided: return; // This is an expected state: leave, since the decision has already been made.
                default: throw std::logic_error("Decision rule must be in the listening or decided state.");
            } // (switch...)
            
            // ~~ Update timer ~~
            ++this->m_count_tics;
            if (this->m_count_tics != proc.count()) throw std::logic_error("Decision rule out of sync.");
            this->on_tic(proc, likelihood); // Make sure the override goes before the \c do_decide_null(...) and \c do_decide_alt(...) are called.

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
                    if (!this->do_decide_null(a, i, 0)) break; // Break the loop the first time the null hypothesis is not accepted.
                    else
                    {
                        next_uncrossed_null_index = i + 1;
                        for (std::size_t j = this->m_first_uncrossed_alt_index; j < n; ++j)
                        {
                            this->m_has_decided_null(i, j) = true; // Mark threshold as crossed.
                            this->m_run_length(i, j) = proc.count(); // Record the freshly stopped times.
                        } // for (...)
                    } // else (...)
                } // for (...)

                // Traverse alt thresholds (horizontal).
                std::size_t next_uncrossed_alt_index = this->m_first_uncrossed_alt_index;
                for (std::size_t j = this->m_first_uncrossed_alt_index; j < n; ++j)
                {
                    value_type b = this->m_unscaled_alt_thresholds[j];
                    if (!this->do_decide_alt(b, 0, j)) break; // Break the loop the first time the alternative hypothesis is not accepted.
                    else
                    {
                        next_uncrossed_alt_index = j + 1;
                        for (std::size_t i = this->m_first_uncrossed_null_index; i < m; ++i)
                        {
                            this->m_has_decided_alt(i, j) = true; // Mark threshold as crossed.
                            this->m_run_length(i, j) = proc.count(); // Record the freshly stopped times.
                        } // for (...)
                    } // else (...)
                } // for (...)

                this->m_first_uncrossed_null_index = next_uncrossed_null_index;
                this->m_first_uncrossed_alt_index = next_uncrossed_alt_index;
                
                bool is_still_listening = (this->m_first_uncrossed_null_index < m) && (this->m_first_uncrossed_alt_index < n);
                if (!is_still_listening) this->m_state = two_sprt_state::decided;
            } // if (...)
            else // Threshold-based design.
            {
                // Traverse all remaining thresholds (vertical and horizontal).
                for (matrix_cell_type& cell : this->m_thresholds_mask)
                {
                    std::size_t i = cell.row();
                    std::size_t j = cell.column();
                    value_type a = this->m_unscaled_null_thresholds[i];
                    value_type b = this->m_unscaled_alt_thresholds[j];
                    bool maybe_null = this->do_decide_null(a, i, j);
                    bool maybe_alt = this->do_decide_alt(b, i, j);
                    this->m_has_decided_null(i, j) = maybe_null;
                    this->m_has_decided_alt(i, j) = maybe_alt;
                    this->m_run_length(i, j) = proc.count();
                    
                    if (maybe_null || maybe_alt) cell.set(); // Decision has been made.
                } // for (...)
                this->m_thresholds_mask.commit();
                if (this->m_thresholds_mask.empty()) this->m_state = two_sprt_state::decided;
            } // else (...)
        } // tic(...)

        /** @brief Reset time, but keep the statistics. */
        void toc(const process_type& proc, const likelihood_type& likelihood, const change_of_measure_type& signal_strength) override
        {
            if (this->m_state != two_sprt_state::decided) throw std::logic_error("Decision rule must be in the decided state.");

            this->on_toc(proc, likelihood);

            const matrix_t<std::size_t>& run_lengths = this->m_run_length;
            const matrix_t<bool>& have_crossed_null = this->m_has_decided_null;
            const matrix_t<bool>& have_crossed_alt = this->m_has_decided_alt;

            bool is_null_true = likelihood.model().is_null(signal_strength.analyzed());
            bool is_alt_true = likelihood.model().is_alt(signal_strength.analyzed());

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
                    if (!has_crossed_null && !has_crossed_alt) throw std::logic_error("Apparently the procedure has not stopped.");
            
                    std::size_t run_length = run_lengths(i, j);
                    std::size_t error = 0;
                    if (has_crossed_null && has_crossed_alt) error = 1;
                    if (has_crossed_null && is_alt_true) error = 1;
                    if (has_crossed_alt && is_null_true) error = 1;
                    //std::size_t error_relaxed = has_crossed_null ? (is_null_true ? 0 : 1) : (is_null_true ? 1 : 0);
            
                    /* @todo Check that \c run_length is not zero. */
                    value_type t = static_cast<value_type>(run_length);
                    value_type e = static_cast<value_type>(error);
                    if (!signal_strength.is_identity())
                    {
                        value_type correction = std::exp(proc.unscaled_log_likelihood_between(
                            signal_strength.simulated(), signal_strength.analyzed(),
                            run_length - 1) / proc.log_likelihood_scale());
                        t /= correction;
                        e /= correction;
                    } // if (...)
                    corrected_run_lengths(i, j) = t;
                    corrected_errors(i, j) = e;
                } // for(...)
            } // for(...)

            this->m_run_lengths.observe(corrected_run_lengths);
            this->m_decision_errors.observe(corrected_errors);
            this->reset_timer(); // Reset time, but keep the statistics.
            this->m_state = two_sprt_state::finalized;
        } // toc(...)

        std::string to_path_string(std::size_t decimal_places) const noexcept override
        {
            constexpr bool is_overwritten = !std::is_same<
                decltype(&derived_type::to_path_string),
                decltype(&type::to_path_string)>::value;
            static_assert(is_overwritten, "to_path_string(...) const noexcept -> std::string was not overwritten.");
            
            const derived_type* that = static_cast<const derived_type*>(this);
            return that->to_path_string(decimal_places);
        } // do_decide_alt(...)
    }; // struct two_sprt
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
