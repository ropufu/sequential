
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED

#include "model.hpp"
#include "not_an_error.hpp"
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
                typedef two_sprt<t_derived_type, t_signal_type> type;
                typedef t_derived_type derived_type;
                typedef t_signal_type signal_type;
                typedef model<signal_type> model_type;
                typedef process<signal_type> process_type;

            private:
                model_type m_model;
                std::vector<std::size_t> m_counts = { 0 }; // Counts the number of observations prior to crossing either of the thresholds.
                std::vector<double> m_null_thresholds = { 0 };
                std::vector<double> m_alt_thresholds = { 0 };
                std::vector<bool> m_have_crossed_null = { false }; // Indicates if the null thresholds have been crossed.
                std::vector<bool> m_have_crossed_alt = { false }; // Indicates if the alt thresholds have been crossed.
                std::size_t m_first_uncrossed_index = 0; // Index of the first uncrossed threshold (either null or alt).

                void post_process_thresholds(std::vector<double>& null_thresholds, std::vector<double>& alt_thresholds) noexcept
                {
                    constexpr bool is_overwritten = std::is_same<
                        decltype(&derived_type::post_process_thresholds), 
                        decltype(&type::post_process_thresholds)>::value;
                    static_assert(!is_overwritten, "static polymorphic function <post_process_thresholds> was not overwritten.");
                    derived_type* that = static_cast<derived_type*>(this);
                    that->post_process_thresholds(null_thresholds, alt_thresholds);
                }

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
                    std::size_t size = this->m_counts.size();
                    for (std::size_t k = 0; k < size; k++)
                    {
                        this->m_counts[k] = 0;
                        this->m_have_crossed_null[k] = false;
                        this->m_have_crossed_alt[k] = false;
                    }
                    this->m_first_uncrossed_index = 0;
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
                quiet_return<void> set_thresholds(const std::vector<double>& null_thresholds, const std::vector<double>& alt_thresholds) noexcept
                {
                    if (this->m_counts.back() != 0) return not_an_error::logic_error; // Thresholds have to be set prior to first observation.
                    if (null_thresholds.size() != alt_thresholds.size()) return not_an_error::invalid_argument; // Threshold sizes have to match.
                    if (null_thresholds.empty()) return not_an_error::logic_error; // At least one threshold has to be specified.
                    if (alt_thresholds.empty()) return not_an_error::logic_error; // At least one threshold has to be specified.

                    this->m_counts.resize(null_thresholds.size(), 0); // Align observation counts to thresholds.
                    this->m_have_crossed_null.resize(null_thresholds.size(), false); // Align cross indicators to thresholds.
                    this->m_have_crossed_alt.resize(null_thresholds.size(), false); // Align cross indicators to thresholds.

                    this->m_null_thresholds = null_thresholds;
                    this->m_alt_thresholds = alt_thresholds;
                    // ~~ Sort the thresholds ~~
                    std::sort(this->m_null_thresholds.begin(), this->m_null_thresholds.end());
                    std::sort(this->m_alt_thresholds.begin(), this->m_alt_thresholds.end());
                    // ~~ Rescale ~~
                    this->post_process_thresholds(this->m_null_thresholds, this->m_alt_thresholds);

                    return not_an_error::all_good;
                }

                const std::vector<double>& null_thresholds() const noexcept { return this->m_null_thresholds; }
                const std::vector<double>& alt_thresholds() const noexcept { return this->m_alt_thresholds; }

                const model_type& model() const noexcept { return this->m_model; }
                const std::vector<std::size_t>& run_lengths() const noexcept { return this->m_counts; }
                const std::vector<bool>& have_crossed_null() const noexcept { return this->m_have_crossed_null; }
                const std::vector<bool>& have_crossed_alt() const noexcept { return this->m_have_crossed_alt; }
                bool has_stopped() const noexcept { return this->m_have_crossed_null.back() || this->m_have_crossed_alt.back(); }
                bool is_running() const noexcept { return !this->has_stopped(); }

                quiet_return<void> observe(const process_type& proc) noexcept
                {
                    if (proc.empty()) return not_an_error::all_good; // Do nothing if the process hasn't collected any observations.
                    if (this->has_stopped()) return not_an_error::all_good; // Do nothing if the process hasn't collected any observations.

                    std::size_t count = proc.count();
                    std::size_t time_index = count - 1;
                    if (time_index != this->m_counts.back()) return not_an_error::logic_error; // Make sure counts are in sync! Otherwise, quietly do nothing.

                    this->observe_unchecked(proc);

                    bool could_cross = true;
                    for (std::size_t k = this->m_first_uncrossed_index; k < this->m_counts.size(); k++) 
                    {
                        this->m_counts[k]++;
                        bool has_crossed_null = could_cross && this->do_decide_null(this->m_null_thresholds[k]);
                        bool has_crossed_alt = could_cross && this->do_decide_alt(this->m_alt_thresholds[k]);
                        this->m_have_crossed_null[k] = has_crossed_null;
                        this->m_have_crossed_alt[k] = has_crossed_alt;

                        // If the current two thresholds were not crossed, then the following (higher) thresholds surely will not be crossed.
                        could_cross = has_crossed_null || has_crossed_alt;
                        if (could_cross) this->m_first_uncrossed_index++;
                    }

                    return not_an_error::all_good;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TWO_SPRT_HPP_INCLUDED
