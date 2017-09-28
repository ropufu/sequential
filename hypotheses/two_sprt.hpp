
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED

#include <aftermath/algebra.hpp>
#include <aftermath/not_an_error.hpp>

#include "model.hpp"
#include "process.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief 2-SPRT, intended as CRTP for static polymorhism.
             *  @remark For more information on CRTP see https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
             */
            template <typename t_derived_type, typename t_signal_type>
            struct two_sprt
            {
                using type = two_sprt<t_derived_type, t_signal_type>;
                using derived_type = t_derived_type;
                using signal_type = t_signal_type;
                using model_type = model<signal_type>;
                using process_type = process<signal_type>;

                template <typename t_data_type>
                using matrix_t = aftermath::algebra::matrix<t_data_type>;

            private:
                model_type m_model;
                bool m_is_initialized = false;
                bool m_is_running = false;
                matrix_t<double> m_unscaled_null_thresholds = { }; // m-by-1 vector of null thresholds.
                matrix_t<double> m_unscaled_alt_thresholds = { };  // 1-by-n vector of alt thresholds.
                matrix_t<bool> m_have_crossed_null = { }; // m-by-n matrix indicating if the null thresholds have been crossed.
                matrix_t<bool> m_have_crossed_alt = { };  // m-by-n matrix indicating if the alt thresholds have been crossed.
                matrix_t<std::size_t> m_counts = { };     // m-by-n matrix counting the number of observations prior to stopping.
                matrix_t<std::size_t> m_first_uncrossed_alt_index = { }; // m-by-1 vector of indices of the first uncrossed alt threshold for a fixed null threshold.

                void observe_unchecked(const process_type& proc) noexcept
                {
                    constexpr bool is_overwritten = std::is_same<
                        decltype(&derived_type::observe_unchecked), 
                        decltype(&type::observe_unchecked)>::value;
                    static_assert(!is_overwritten, "static polymorphic function <observe_unchecked> was not overwritten.");
                    derived_type* that = static_cast<derived_type*>(this);
                    that->observe_unchecked(proc);
                }

                void reset_unchecked() noexcept
                {
                    constexpr bool is_overwritten = std::is_same<
                        decltype(&derived_type::reset_unchecked), 
                        decltype(&type::reset_unchecked)>::value;
                    static_assert(!is_overwritten, "static polymorphic function <reset_unchecked> was not overwritten.");
                    derived_type* that = static_cast<derived_type*>(this);
                    that->reset_unchecked();
                }

            protected:
                explicit two_sprt(const model_type& model) noexcept
                    : m_model(model)
                {
                }

            public:
                void reset() noexcept
                {
                    this->m_have_crossed_null.erase();
                    this->m_have_crossed_alt.erase();
                    this->m_counts.erase();
                    this->m_first_uncrossed_alt_index.erase();
                    this->m_is_running = true;
                    
                    this->reset_unchecked();
                }

                bool do_decide_null(double threshold) const noexcept
                {
                    constexpr bool is_overwritten = std::is_same<decltype(&derived_type::do_decide_null), decltype(&type::do_decide_null)>::value;
                    static_assert(!is_overwritten, "static polymorphic function <do_decide_null> was not overwritten.");
                    const derived_type* that = static_cast<const derived_type*>(this);
                    return that->do_decide_null(threshold);
                }

                bool do_decide_alt(double threshold) const noexcept
                {
                    constexpr bool is_overwritten = std::is_same<decltype(&derived_type::do_decide_alt), decltype(&type::do_decide_alt)>::value;
                    static_assert(!is_overwritten, "static polymorphic function <do_decide_alt> was not overwritten.");
                    const derived_type* that = static_cast<const derived_type*>(this);
                    return that->do_decide_alt(threshold);
                }

                /** @remark Thresholds have to be of the same size; they are independently(!) sorted and then paired up. */
                void set_thresholds(const std::vector<double>& null_thresholds, const std::vector<double>& alt_thresholds) noexcept
                {
                    if (this->m_is_initialized)  aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, "Thresholds have to be set prior to first observation.", __FUNCTION__, __LINE__);
                    if (null_thresholds.empty()) aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, "At least one null threshold has to be specified.", __FUNCTION__, __LINE__);
                    if (alt_thresholds.empty())  aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, "At least one alt threshold has to be specified.", __FUNCTION__, __LINE__);
                    if (!aftermath::quiet_error::instance().good()) return;

                    this->m_is_initialized = true;
                    this->m_is_running = true;

                    std::size_t m = null_thresholds.size(); // Height of the threshold matrix.
                    std::size_t n = alt_thresholds.size(); // Width of the threshold matrix.

                    this->m_have_crossed_null = matrix_t<bool>(m, n); // Align cross indicators to thresholds.
                    this->m_have_crossed_alt = matrix_t<bool>(m, n); // Align cross indicators to thresholds.
                    this->m_counts = matrix_t<size_t>(m, n); // Align observation counts to thresholds.
                    this->m_first_uncrossed_alt_index = matrix_t<size_t>(m, 1); // Align indices of uncrossed alt thresholds.

                    std::vector<double> unscaled_null_thresholds = null_thresholds;
                    std::vector<double> unscaled_alt_thresholds = alt_thresholds;
                    // ~~ Sort the thresholds ~~
                    std::sort(unscaled_null_thresholds.begin(), unscaled_null_thresholds.end());
                    std::sort(unscaled_alt_thresholds.begin(), unscaled_alt_thresholds.end());
                    // ~~ Rescale ~~
                    double factor = this->m_model.log_likelihood_scale();
                    for (double& a : unscaled_null_thresholds) a *= factor;
                    for (double& a : unscaled_alt_thresholds) a *= factor;

                    this->m_unscaled_null_thresholds = unscaled_null_thresholds;
                    this->m_unscaled_alt_thresholds = unscaled_alt_thresholds;
                    this->m_unscaled_null_thresholds.reshape(m, 1);
                    this->m_unscaled_alt_thresholds.reshape(1, n);
                }

                const matrix_t<double>& unscaled_null_thresholds() const noexcept { return this->m_unscaled_null_thresholds; }
                const matrix_t<double>& unscaled_alt_thresholds() const noexcept { return this->m_unscaled_alt_thresholds; }

                const model_type& model() const noexcept { return this->m_model; }
                const matrix_t<std::size_t>& run_lengths() const noexcept { return this->m_counts; }
                const matrix_t<bool>& have_crossed_null() const noexcept { return this->m_have_crossed_null; }
                const matrix_t<bool>& have_crossed_alt() const noexcept { return this->m_have_crossed_alt; }
                bool has_stopped() const noexcept { return !this->m_is_running; }
                bool is_running() const noexcept { return this->m_is_running; }

                void observe(const process_type& proc) noexcept
                {
                    if (proc.empty()) return; // Do nothing if the process hasn't collected any observations.
                    if (this->has_stopped()) return; // Do nothing if the process hasn't collected any observations.

                    std::size_t time_index = proc.count() - 1;
                    if (time_index != this->m_counts.back()) aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, "Process and stopping time are out of sync.", __FUNCTION__, __LINE__);
                    if (!aftermath::quiet_error::instance().good()) return;

                    this->observe_unchecked(proc);

                    bool is_still_running = false;
                    std::size_t m = this->m_unscaled_null_thresholds.size(); // Height of the threshold matrix.
                    std::size_t n = this->m_unscaled_alt_thresholds.size(); // Width of the threshold matrix.
                    for (std::size_t i = 0; i < m; i++)
                    {
                        bool could_cross_next = true;
                        for (std::size_t j = this->m_first_uncrossed_alt_index.at(i, 0); j < n; j++)
                        {
                            this->m_counts.at(i, j)++;
                            // Next line can be moved outside the j-loop.
                            bool has_crossed_null = could_cross_next && this->do_decide_null(this->m_unscaled_null_thresholds.at(i, 0));
                            bool has_crossed_alt = could_cross_next && this->do_decide_alt(this->m_unscaled_alt_thresholds.at(0, j));
                            this->m_have_crossed_null.at(i, j) = has_crossed_null;
                            this->m_have_crossed_alt.at(i, j) = has_crossed_alt;

                            // If the current two thresholds were not crossed, then the following (higher) thresholds surely will not be crossed.
                            could_cross_next = has_crossed_null || has_crossed_alt;
                            if (could_cross_next) this->m_first_uncrossed_alt_index.at(i, 0)++;
                            else is_still_running = true;
                        }
                    }
                    this->m_is_running = is_still_running;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
