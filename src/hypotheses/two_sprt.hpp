
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED

#include <aftermath/algebra.hpp>      // aftermath::algebra::matrix
#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include "likelihood.hpp"
#include "model.hpp"
#include "moment_statistic.hpp"
#include "observer.hpp"
#include "process.hpp"

#include <algorithm>   // std::sort
#include <cmath>       // std::exp, std::isnan, std::isinf
#include <cstddef>     // std::size_t
#include <type_traits> // std::is_same
#include <vector>      // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
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
            struct two_sprt : public observer<two_sprt<t_derived_type, t_signal_type, t_noise_type, t_sync_check>, t_signal_type, t_noise_type, t_sync_check>
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
                likelihood_type m_likelihood = { }; // Reset with each \c toc().
                value_type m_analyzed_mu = 0; // The signal "strength" conrresponding to what measure we want to analyze.
                value_type m_anticipated_run_length = 0; // An auxiliary quantity to improve accuracy of statistics.
                matrix_t<value_type> m_unscaled_null_thresholds = { }; // m-by-1 vector of null thresholds.
                matrix_t<value_type> m_unscaled_alt_thresholds = { };  // 1-by-n vector of alt thresholds.

                // ~~ Members reset with each \c \reset(), but persisting across \c toc() calls ~~
                bool m_is_initialized = false;
                statistic_type m_errors = { }; // Track erroneous decisions made by the stopping time.
                statistic_type m_run_lengths = { }; // Track run lengths of the stopping time.

                // ~~ Members reset with each \c toc() ~~
                bool m_is_listening = false;
                matrix_t<bool> m_have_crossed_null = { }; // m-by-n matrix indicating if the null thresholds have been crossed.
                matrix_t<bool> m_have_crossed_alt = { };  // m-by-n matrix indicating if the alt thresholds have been crossed.
                matrix_t<std::size_t> m_counts = { };     // m-by-n matrix counting the number of observations prior to stopping.
                matrix_t<std::size_t> m_first_uncrossed_alt_index = { }; // m-by-1 vector of indices of the first uncrossed alt threshold for a fixed null threshold.

            private:
                void coerce() noexcept
                {
                    if (std::isnan(this->m_analyzed_mu) || std::isinf(this->m_analyzed_mu))
                    {
                        this->m_analyzed_mu = 0;
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "Analyzed mu has to be a number. Coerced analyzed mu to zero.", __FUNCTION__, __LINE__);
                    }
                    if (std::isnan(this->m_anticipated_run_length) || std::isinf(this->m_anticipated_run_length) || m_anticipated_run_length < 0)
                    {
                        this->m_anticipated_run_length = 0;
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "Anticipated run length has to be a non-negative number. Coerced anticipated run length to zero.", __FUNCTION__, __LINE__);
                    }
                } // coerce(...)

                /** @brief Resets the time to zero, but keeps the statistics. */
                void soft_reset() noexcept
                {
                    this->m_is_listening = this->m_is_initialized;
                    this->m_likelihood.reset();

                    this->m_have_crossed_null.erase();
                    this->m_have_crossed_alt.erase();
                    this->m_counts.erase();
                    this->m_first_uncrossed_alt_index.erase();
                } // soft_reset(...)
                
                /** @brief Resets the statistics. */
                void hard_reset() noexcept
                {
                    this->soft_reset();
                    this->m_is_initialized = false;
                    this->m_is_listening = false;

                    this->m_unscaled_null_thresholds.erase();
                    this->m_unscaled_alt_thresholds.erase();

                    this->m_errors.clear();
                    this->m_run_lengths.clear();
                } // hard_reset(...)

            protected:
                two_sprt() noexcept { }
                
                /** Set the signal "strength" conrresponding to what measure we want to analyze and an auxiliary quantity to improve accuracy of statistics. */
                explicit two_sprt(std::size_t id) noexcept : m_id(id) { }

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

                /** @brief Auxiliary function to be executed right after the \c on_tic() call. */
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
                 *  @todo Rewrite to (i) run null/alt thresholds independently before going into joint tables; (ii) only update counts on stop rather than at every tick.
                 */
                void on_tic(const process_type& proc) noexcept
                {
                    if (proc.empty()) return; // Do nothing if the process hasn't collected any observations.
                    if (this->has_stopped()) return; // Do nothing if the process hasn't collected any observations.

                    std::size_t time_index = proc.time();
                    if (!this->m_is_initialized) aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "Decision rule has not been initialized.", __FUNCTION__, __LINE__);
                    if (time_index != this->m_counts.back()) aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "Process and stopping time are out of sync.", __FUNCTION__, __LINE__);
                    if (!aftermath::quiet_error::instance().good()) return;

                    // Null thresholds: a.
                    // Alt thresholds: b.
                    // The scanning goes horizontally, left to right.
                    // First uncrossed alt indices are marked with 'x'.
                    //
                    //         |  0    1    2    3    ...   n-1        | b (alt) 
                    // --------|---------------------------------------|         
                    //     0   |                x->   ...   -->    v   |         
                    //     1   |           x->  -->   ...   -->    v   |         
                    //    ...  |      ...             ...   -->    v   |         
                    //     ?   | x->  -->  -->  -->   ...         ...  |         
                    //    ?+1  |                                       |         
                    //    ...  |                                       |         
                    //    m-2  |                                       |         
                    //    m-1  |                                       |         
                    // -------------------------------------------------         
                    //  a (null)                                                 
                    //
                    bool is_still_running = false;
                    this->m_likelihood.tic(proc);
                    std::size_t m = this->m_unscaled_null_thresholds.size(); // Height of the threshold matrix.
                    std::size_t n = this->m_unscaled_alt_thresholds.size(); // Width of the threshold matrix.
                    for (std::size_t i = 0; i < m; ++i)
                    {
                        bool could_have_crossed_next = true;
                        // There is no need to update anything where the process has already stopped:
                        // start at the first uncrossed alt index instead.
                        for (std::size_t j = this->m_first_uncrossed_alt_index.at(i, 0); j < n; ++j)
                        {
                            ++this->m_counts.at(i, j);
                            // Next line can be moved outside the j-loop.
                            bool has_crossed_null = could_have_crossed_next && this->do_decide_null(this->m_unscaled_null_thresholds.at(i, 0));
                            bool has_crossed_alt = could_have_crossed_next && this->do_decide_alt(this->m_unscaled_alt_thresholds.at(0, j));
                            // @todo Keep running if both thresholds have been crossed.
                            this->m_have_crossed_null.at(i, j) = has_crossed_null;
                            this->m_have_crossed_alt.at(i, j) = has_crossed_alt;

                            // If the current two thresholds were not crossed, then the following (higher) thresholds surely have not be crossed.
                            //bool has_crossed_both = has_crossed_null && has_crossed_alt;
                            bool has_crossed_either = has_crossed_null || has_crossed_alt;
                            could_have_crossed_next = has_crossed_either;
                            
                            if (could_have_crossed_next) ++this->m_first_uncrossed_alt_index.at(i, 0);
                            else is_still_running = true;
                        } // for(...)
                    } // for(...)
                    this->m_is_listening = is_still_running;

                    this->on_tic_override(proc);
                } // on_tic(...)

                /** @brief Auxiliary function to be executed right before the \c toc() call. */
                void on_toc(const process_type& proc) noexcept
                {
                    this->on_toc_override(proc);

                    const matrix_t<std::size_t>& run_lengths = this->m_counts;
                    const matrix_t<bool>& have_crossed_null = this->m_have_crossed_null;
                    const matrix_t<bool>& have_crossed_alt = this->m_have_crossed_alt;

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
                            bool has_crossed_null = have_crossed_null.at(i, j);
                            bool has_crossed_alt = have_crossed_alt.at(i, j);
                            // Check if the stopping time has stopped!
                            if (!has_crossed_null && !has_crossed_alt)
                            {
                                aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "The procedure has not stopped! Resetting.", __FUNCTION__, __LINE__);
                                this->soft_reset(); // If not, reset time anyway (but keep the statistics).
                                return;
                            }
                    
                            std::size_t run_length = run_lengths.at(i, j);
                            std::size_t error = 0;
                            if (has_crossed_null && has_crossed_alt) error = 1;
                            if (has_crossed_null && is_alt_true) error = 1;
                            if (has_crossed_alt && is_null_true) error = 1;
                            //std::size_t error_relaxed = has_crossed_null ? (is_null_true ? 0 : 1) : (is_null_true ? 1 : 0);
                    
                            value_type correction = std::exp(proc.unscaled_log_likelihood_between(proc.actual_mu(), this->m_analyzed_mu, run_length) / proc.log_likelihood_scale());
                            value_type t = run_length / correction;
                            value_type e = error / correction;
                            corrected_run_lengths.at(i, j) = t;
                            corrected_errors.at(i, j) = e;
                        } // for(...)
                    } // for(...)

                    this->m_run_lengths.observe(corrected_run_lengths);
                    this->m_errors.observe(corrected_errors);
                    this->soft_reset(); // Reset time, but keep the statistics.
                } // on_toc(...)

            public:
                std::size_t id() const noexcept { return this->m_id; }

                const likelihood_type& likelihood() const noexcept { return this->m_likelihood; }

                const matrix_t<std::size_t>& counts() const noexcept { return this->m_counts; }

                const matrix_t<bool>& have_crossed_null() const noexcept { return this->m_have_crossed_null; }

                const matrix_t<bool>& have_crossed_alt() const noexcept { return this->m_have_crossed_alt; }
                
                bool has_stopped() const noexcept { return !this->m_is_listening; }
                bool is_listening() const noexcept { return this->m_is_listening; }
                
                /** Signal "strength" conrresponding to what measure we want to analyze. */
                value_type analyzed_mu() const noexcept { return this->m_analyzed_mu; }

                /** An auxiliary quantity to improve accuracy of statistics. */
                value_type anticipated_run_length() const noexcept { return this->m_anticipated_run_length; }

                /** Track erroneous decisions made by the stopping time. */
                const statistic_type& errors() const noexcept { return this->m_errors; }

                /** Track run lengths of the stopping time. */
                const statistic_type& run_lengths() const noexcept { return this->m_run_lengths; }

                const matrix_t<value_type>& unscaled_null_thresholds() const noexcept { return this->m_unscaled_null_thresholds; }
                const matrix_t<value_type>& unscaled_alt_thresholds() const noexcept { return this->m_unscaled_alt_thresholds; }

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
                    const std::vector<value_type>& alt_thresholds) noexcept
                {
                    if (this->m_is_initialized)  aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "Thresholds have to be set prior to first observation.", __FUNCTION__, __LINE__);
                    if (null_thresholds.empty()) aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "At least one null threshold has to be specified.", __FUNCTION__, __LINE__);
                    if (alt_thresholds.empty())  aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "At least one alt threshold has to be specified.", __FUNCTION__, __LINE__);

                    bool are_null_bad = false;
                    bool are_alt_bad = false;
                    for (const value_type& a : null_thresholds) if (std::isnan(a) || std::isinf(a)) are_null_bad = true;
                    for (const value_type& a : alt_thresholds) if (std::isnan(a) || std::isinf(a)) are_alt_bad = true;
                    if (are_null_bad) aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "Null thresholds must be finite numbers.", __FUNCTION__, __LINE__);
                    if (are_alt_bad) aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "Alt thresholds must be finite numbers.", __FUNCTION__, __LINE__);

                    if (!aftermath::quiet_error::instance().good()) return; // Return on error.

                    this->m_likelihood = likelihood_type(model);
                    this->m_analyzed_mu = analyzed_mu;
                    this->m_anticipated_run_length = anticipated_run_length;
                    this->coerce();

                    std::size_t m = null_thresholds.size(); // Height of the threshold matrix.
                    std::size_t n = alt_thresholds.size(); // Width of the threshold matrix.

                    this->m_have_crossed_null = matrix_t<bool>(m, n); // Align cross indicators to thresholds.
                    this->m_have_crossed_alt = matrix_t<bool>(m, n); // Align cross indicators to thresholds.
                    this->m_counts = matrix_t<size_t>(m, n); // Align observation counts to thresholds.
                    this->m_first_uncrossed_alt_index = matrix_t<size_t>(m, 1); // Align indices of uncrossed alt thresholds.

                    std::vector<value_type> unscaled_null_thresholds = null_thresholds;
                    std::vector<value_type> unscaled_alt_thresholds = alt_thresholds;
                    // ~~ Sort the thresholds ~~
                    std::sort(unscaled_null_thresholds.begin(), unscaled_null_thresholds.end());
                    std::sort(unscaled_alt_thresholds.begin(), unscaled_alt_thresholds.end());
                    // ~~ Rescale ~~
                    value_type factor = proc.log_likelihood_scale(); // @todo Think, can we get rid of \c proc, and move the scale elsewhere?
                    for (value_type& a : unscaled_null_thresholds) a *= factor;
                    for (value_type& a : unscaled_alt_thresholds) a *= factor;

                    this->m_unscaled_null_thresholds = unscaled_null_thresholds;
                    this->m_unscaled_alt_thresholds = unscaled_alt_thresholds;
                    this->m_unscaled_null_thresholds.reshape(m, 1);
                    this->m_unscaled_alt_thresholds.reshape(1, n);
                    
                    // Now that the stopping time has verified the threshold structure, go on and resize the empirical measures accordingly.
                    matrix_t<value_type> zero(m, n);
                    matrix_t<value_type> anticipated_mean(m, n, this->m_anticipated_run_length);

                    this->m_errors = statistic_type(zero, zero); // Align cross indicators to match threshold size.
                    this->m_run_lengths = statistic_type(zero, anticipated_mean); // Align observation counts to match threshold size.

                    // Finish up.
                    if (!aftermath::quiet_error::instance().good()) return;
                    this->on_initialized();
                    this->m_is_initialized = true;
                    this->m_is_listening = true;
                } // initialize(...)

                // friend std::ostream& operator <<(std::ostream& os, const type& self)
                // {
                //     auto ess = self.m_run_lengths.mean();
                //     auto perror = self.m_errors.mean();
                //     os <<
                //         self.m_name << " {" <<
                //         " ESS: " << ess.front() << "---" << ess.back() <<
                //         " pm " << std::sqrt(self.m_run_lengths.variance().back()) << ","
                //         " P[error]: (" << type::error_factor * perror.front() << "---" << type::error_factor * perror.back() <<
                //         " pm " << type::error_factor * std::sqrt(self.m_errors.variance().back()) <<
                //         ") e-" << type::error_suffix <<
                //         " }";
                //     return os;
                // } // operator <<(...)
            }; // struct two_sprt
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
