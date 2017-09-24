
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_BUNCH_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_BUNCH_HPP_INCLUDED

#include <nlohmann/json.hpp>

#include "config.hpp"
#include "model.hpp"
#include "not_an_error.hpp"
#include "process.hpp"
#include "stopping_time_observer.hpp"

#include "adaptive_sprt_star.hpp"
#include "adaptive_sprt.hpp"
#include "generalized_sprt.hpp"
#include "generalized_sprt_star.hpp"

#include <cstddef>
#include <fstream>
#include <iomanip>  // For std::setprecision
#include <iostream> // For std::cout.
#include <string>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                struct matlab
                {
                    static std::vector<double> linspace(double from, double to, std::size_t count) noexcept
                    {
                        if (count == 0) return std::vector<double>(0);
                        if (count == 1) return { from };

                        double range = to - from;
                        std::vector<double> result(count);
                        result[0] = from;
                        result[count - 1] = to;
                        for (std::size_t i = 1; i < count - 1; i++)
                        {
                            double step = (i * range) / (count - 1);
                            result[i] = from + step;
                        }
                        result.shrink_to_fit();
                        return result;
                    }
                };
            }

            /** A collection of any number of observers for the pre-defined types of stopping times. */
            template <typename t_signal_type>
            struct observer_bunch
            {
                typedef observer_bunch<t_signal_type> type;
                typedef t_signal_type signal_type;
                typedef model<signal_type> model_type;
                typedef process<signal_type> process_type;

                typedef adaptive_sprt_star<signal_type>    fsprt_type;
                typedef adaptive_sprt<signal_type>         asprt_type;
                typedef generalized_sprt<signal_type>      gsprt_type;
                typedef generalized_sprt_star<signal_type> hsprt_type;

            private:
                model_type m_model; // Hypotheses testing model descriptor.
                double m_desired_mu; // Signal "strength" to analyse.
                std::vector<stopping_time_observer<fsprt_type>> m_fsprts;
                std::vector<stopping_time_observer<asprt_type>> m_asprts;
                std::vector<stopping_time_observer<gsprt_type>> m_gsprts;
                std::vector<stopping_time_observer<hsprt_type>> m_hsprts;

                template <typename t_stopping_type>
                std::ostream& write(const stopping_time_observer<t_stopping_type>& observer, std::ostream& os)
                {
                    typedef typename stopping_time_observer<t_stopping_type>::empirical_measure_type empirical_measure_type;
                    os << observer << std::endl; // A brief summary.
                    // A more detailed tab-separated summary.
                    os << std::setprecision(16);
                    os << "\tNull Threshold" << "\tAlt Threshold" << "\tESS" << "\tESS stdev" << "\tP[error]" << "\tP[error] stdev" << std::endl;
                    const std::vector<empirical_measure_type>& run_lengths = observer.run_lengths();
                    const std::vector<empirical_measure_type>& errors = observer.errors();
                    const std::vector<double>& null_thresholds = observer.stopping_time().null_thresholds();
                    const std::vector<double>& alt_thresholds = observer.stopping_time().alt_thresholds();
                    for (std::size_t i = 0; i < run_lengths.size(); i++)
                    {
                        os << '\t' << null_thresholds[i];
                        os << '\t' << alt_thresholds[i];
                        os << '\t' << run_lengths[i].mean();
                        os << '\t' << run_lengths[i].compute_standard_deviation();
                        os << '\t' << errors[i].mean();
                        os << '\t' << errors[i].compute_standard_deviation();
                        os << std::endl;
                    }
                    return os;
                }
            
            public:
                friend std::ostream& operator <<(std::ostream& os, const type& self)
                {
                    for (const auto& p : self.m_fsprts) os << '\t' << p << std::endl;
                    for (const auto& p : self.m_asprts) os << '\t' << p << std::endl;
                    for (const auto& p : self.m_gsprts) os << '\t' << p << std::endl;
                    for (const auto& p : self.m_hsprts) os << '\t' << p << std::endl;
                    return os;
                }

                bool clear_log(std::string filename = "./hypotheses.log") noexcept
                {
                    std::ofstream o(filename); // Try to open the file for writing.
                    if (!o.good()) return false; // Stop on failure.
                    return true;
                }

                /** Write the configuration to a file. */
                template <typename t_caption_type>
                bool log_caption(const t_caption_type& caption, std::string filename = "./hypotheses.log") noexcept
                {
                    std::ofstream o(filename, std::ios_base::app); // Try to open the file for writing.
                    if (!o.good()) return false; // Stop on failure.
                    o << caption << std::endl;
                    o << this->m_model << std::endl;
                    return true;
                }

                /** Write the configuration to a file. */
                bool log(std::string filename = "./hypotheses.log") noexcept
                {
                    std::ofstream o(filename, std::ios_base::app); // Try to open the file for writing.
                    if (!o.good()) return false; // Stop on failure.

                    for (const auto& p : this->m_fsprts) this->write(p, o);
                    for (const auto& p : this->m_asprts) this->write(p, o);
                    for (const auto& p : this->m_gsprts) this->write(p, o);
                    for (const auto& p : this->m_hsprts) this->write(p, o);
                    o << std::endl;

                    return true;
                }
                
                explicit observer_bunch(const model_type& model) noexcept
                    : m_model(model), m_desired_mu(model.mu_under_null())
                {
                }

                /** Clears all the observations and resets the underlying stopping time. */
                void clear() noexcept
                {
                    for (auto& p : this->m_fsprts) p.clear();
                    for (auto& p : this->m_asprts) p.clear();
                    for (auto& p : this->m_gsprts) p.clear();
                    for (auto& p : this->m_hsprts) p.clear();
                }

                /** The signal "strength" conrresponding to what measure we want to analyze. */
                void look_for(double desired_mu) noexcept
                {
                    for (auto& p : this->m_fsprts) p.look_for(desired_mu);
                    for (auto& p : this->m_asprts) p.look_for(desired_mu);
                    for (auto& p : this->m_gsprts) p.look_for(desired_mu);
                    for (auto& p : this->m_hsprts) p.look_for(desired_mu);
                    this->m_desired_mu = desired_mu;
                }

                /** Signal "strength" to analyse. */
                double desired_mu() const noexcept { return this->m_desired_mu; }

                /** Indicates if at least one of the underlying stopping times is still running. */
                bool is_running() const noexcept 
                {
                    for (const auto& p : this->m_fsprts) if (p.is_running()) return true;
                    for (const auto& p : this->m_asprts) if (p.is_running()) return true;
                    for (const auto& p : this->m_gsprts) if (p.is_running()) return true;
                    for (const auto& p : this->m_hsprts) if (p.is_running()) return true;
                    return false;
                }

                /** @todo Check for quiet errors. */
                quiet_return<void> tic(const process_type& proc) noexcept 
                {
                    for (auto& p : this->m_fsprts) p.tic(proc);
                    for (auto& p : this->m_asprts) p.tic(proc);
                    for (auto& p : this->m_gsprts) p.tic(proc);
                    for (auto& p : this->m_hsprts) p.tic(proc);
                    return not_an_error::all_good;
                }
            
                /** @todo Check for quiet errors. */
                quiet_return<void> toc(const process_type& proc) noexcept
                {
                    for (auto& p : this->m_fsprts) p.toc(proc);
                    for (auto& p : this->m_asprts) p.toc(proc);
                    for (auto& p : this->m_gsprts) p.toc(proc);
                    for (auto& p : this->m_hsprts) p.toc(proc);
                    return not_an_error::all_good;
                }
                
                /** Parses \p json entry to construct and add another stopping time observer to the collection. */
                quiet_return<void> try_parse(const nlohmann::json& json) noexcept
                {
                    std::string proc_name = json["name"];
                    std::string type_name = json["type"];
                    //std::string threshold_spacing = json["threshold spacing"];
                    std::size_t threshold_count = json["threshold count"];
                    std::vector<double> threshold_range_null = json["threshold range null"];
                    std::vector<double> threshold_range_alt = json["threshold range alt"];

                    if (threshold_range_null.size() != 2 || threshold_range_alt.size() != 2) return not_an_error::runtime_error;
                    std::vector<double> thresholds_null = detail::matlab::linspace(threshold_range_null[0], threshold_range_null[1], threshold_count);
                    std::vector<double> thresholds_alt = detail::matlab::linspace(threshold_range_alt[0], threshold_range_alt[1], threshold_count);
            
                    if (type_name == "adaptive_sprt_star")
                    {
                        double guess_mu_null = json["mu guess for null"]; // Initial guess for signal "strength" for adaptive LLR under the null hypothesis.
                        double guess_mu_alt = json["mu guess for alt"]; // Initial guess for signal "strength" for adaptive LLR under the alternative hypothesis.
            
                        stopping_time_observer<fsprt_type> o(
                            this->m_model,
                            guess_mu_null, guess_mu_alt);
                        o.look_for(this->m_desired_mu);
                        o.set_name(proc_name);
                        not_an_error result = o.set_thresholds(thresholds_null, thresholds_alt);
            
                        this->m_fsprts.push_back(o);
                        return result;
                    }
                    if (type_name == "adaptive_sprt")
                    {
                        double guess_mu_null = json["mu guess for null"];
                        double guess_mu_alt = json["mu guess for alt"];
            
                        stopping_time_observer<asprt_type> o(
                            this->m_model,
                            guess_mu_null, guess_mu_alt);
                        o.look_for(this->m_desired_mu);
                        o.set_name(proc_name);
                        not_an_error result = o.set_thresholds(thresholds_null, thresholds_alt);
            
                        this->m_asprts.push_back(o);
                        return result;
                    }
                    if (type_name == "generalized_sprt")
                    {
                        stopping_time_observer<gsprt_type> o(
                            this->m_model);
                        o.look_for(this->m_desired_mu);
                        o.set_name(proc_name);
                        not_an_error result = o.set_thresholds(thresholds_null, thresholds_alt);
            
                        this->m_gsprts.push_back(o);
                        return result;
                    }
                    if (type_name == "generalized_sprt_star")
                    {
                        stopping_time_observer<hsprt_type> o(
                            this->m_model);
                        o.look_for(this->m_desired_mu);
                        o.set_name(proc_name);
                        not_an_error result = o.set_thresholds(thresholds_null, thresholds_alt);
            
                        this->m_hsprts.push_back(o);
                        return result;
                    }
                    return not_an_error::runtime_error;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_BUNCH_HPP_INCLUDED
