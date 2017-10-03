
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_BUNCH_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_BUNCH_HPP_INCLUDED

#include <aftermath/format.hpp>
#include <aftermath/not_an_error.hpp>
#include <nlohmann/json.hpp>

#include "config.hpp"
#include "filesystem.hpp"
#include "matlab.hpp"
#include "model.hpp"
#include "process.hpp"
#include "stopping_time_observer.hpp"

#include "adaptive_sprt_star.hpp"
#include "adaptive_sprt.hpp"
#include "generalized_sprt.hpp"
#include "generalized_sprt_star.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>  // For std::setprecision
#include <iostream> // For std::cout.
#include <sstream>  // For std::ostringstream.
#include <string>
#include <utility>  // For std::forward.
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                /** @brief Set the thresholds for the underlying stopping type.
                 *  @remark Forwards the arguments to the stopping time.
                 */
                template <typename t_stopping_type, typename... t_args>
                bool try_push_back(
                    std::vector<stopping_time_observer<t_stopping_type>>& collection,
                    const std::string& type_name,
                    const std::string& proc_name,
                    const std::vector<double>& thresholds_null,
                    const std::vector<double>& thresholds_alt,
                    t_args&&... args) noexcept
                {
                    stopping_time_observer<t_stopping_type> o(std::forward<t_args>(args)...);
                    o.set_type_name(type_name);
                    o.set_name(proc_name);
                    o.set_thresholds(thresholds_null, thresholds_alt);
                    collection.push_back(o);
                    return true;
                }              
            }

            /** A collection of any number of observers for the pre-defined types of stopping times. */
            template <typename t_signal_type>
            struct observer_bunch
            {
                using type = observer_bunch<t_signal_type>;
                using signal_type = t_signal_type;
                using model_type = model<signal_type>;
                using process_type = process<signal_type>;
                using matstream_type = aftermath::format::matstream<4>;

                using fsprt_type = adaptive_sprt_star<signal_type>;
                using asprt_type = adaptive_sprt<signal_type>;
                using gsprt_type = generalized_sprt<signal_type>;
                using hsprt_type = generalized_sprt_star<signal_type>;

            private:
                model_type m_model; // Hypotheses testing model descriptor.
                bool m_has_parsed = false;
                bool m_has_been_initialized = false;
                bool m_do_fix_mat_prefix = false;
                std::string m_mat_prefix = "";
                std::string m_error_var_name;
                std::string m_run_length_var_name;
                std::string m_log_filename;
                std::string m_mat_folder;
                std::vector<stopping_time_observer<fsprt_type>> m_fsprts;
                std::vector<stopping_time_observer<asprt_type>> m_asprts;
                std::vector<stopping_time_observer<gsprt_type>> m_gsprts;
                std::vector<stopping_time_observer<hsprt_type>> m_hsprts;

                /** @brief Generates prefix for .mat files, and copies the config file to the output folder.
                 *  @remark If prefix has been fixed, and has already been generated, does nothing.
                 */
                void generate_mat_prefix()
                {
                    // If the prefix has been fixed, and has already been generated, do nothing.
                    if (this->m_do_fix_mat_prefix && this->m_mat_prefix != "") return;

                    constexpr std::size_t prefix_size = 3;
                    char letter_from = 'a';
                    char letter_to = 'z';

                    std::array<char, prefix_size> prefix;
                    prefix.fill(letter_from);

                    std::string mat_prefix = "";
                    std::string file_path = "";
                    while (true)
                    {
                        // Build the current .json file name:
                        // [0] [1] ... [prefix_size - 1].json
                        std::ostringstream prefix_stream;
                        for (std::size_t i = 0; i < prefix_size; i++) prefix_stream << prefix[i];
                        this->m_model.mat_prefix(prefix_stream);
                        mat_prefix = prefix_stream.str();
                        prefix_stream << ".json";
                        // Build a path to current .json file. 
                        file_path = filesystem::path_combine(this->m_mat_folder, prefix_stream.str());

                        if (!filesystem::does_file_exist(file_path)) break; // If the path does not exist all is well; proceed with current prefix.

                        // The prefix is already taken: switch to next one.
                        std::size_t index = prefix_size - 1;
                        prefix[index]++;
                        bool is_overflow = prefix[index] > letter_to;
                        while (is_overflow)
                        {
                            prefix[index] = letter_from;
                            if (index == 0) break;
                            index--;
                            prefix[index]++;
                            is_overflow = prefix[index] > letter_to;
                        }

                        // Check if we've run out of options.
                        if (is_overflow)
                        {
                            aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Name pool exhausted!", __FUNCTION__, __LINE__);
                            return;
                        }
                    }
                    // Copy the current configuration to output folder.
                    config::instance().write(file_path);
                    this->m_mat_prefix = mat_prefix;
                }

                template <typename t_stopping_type>
                void write_mat(const stopping_time_observer<t_stopping_type>& observer)
                {
                    typedef typename stopping_time_observer<t_stopping_type>::statistic_type statistic_type;
                    typedef t_stopping_type stopping_type;

                    if (this->m_mat_prefix == "")
                    {
                        aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Mat prefix has not been generated.", __FUNCTION__, __LINE__);
                        return;
                    }
                    if (this->m_error_var_name == "" || this->m_run_length_var_name == "")
                    {
                        aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Variable names have not been set.", __FUNCTION__, __LINE__);
                        return;
                    }

                    std::ostringstream mat_name_stream;
                    mat_name_stream << this->m_mat_prefix << "_" << observer.type_name();
                    mat_name_stream << ".mat";

                    std::string mat_filename = filesystem::path_combine(this->m_mat_folder, mat_name_stream.str());
                    if (!filesystem::can_write_surely(mat_filename, __FUNCTION__, __LINE__)) return;

                    const stopping_type& stopping_time = observer.stopping_time();
                    const statistic_type& errors = observer.errors();
                    const statistic_type& run_lengths = observer.run_lengths();
                    matstream_type mat(mat_filename);
                    // mat.clear(); // Clear the existing contents.
                    
                    mat << "b_null" << stopping_time.unscaled_null_thresholds();
                    mat << "b_alt" << stopping_time.unscaled_alt_thresholds();
                    mat << this->m_error_var_name << "_mean" << errors.mean();
                    mat << this->m_error_var_name << "_var" << errors.variance();
                    mat << this->m_run_length_var_name << "_mean" << run_lengths.mean();
                    mat << this->m_run_length_var_name << "_var" << run_lengths.variance();
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

                void clear_log() noexcept
                {
                    std::ofstream o(this->m_log_filename); // Try to open the file for writing and erase current contents..
                    if (!o.good()) aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Log file could not be cleared.", __FUNCTION__, __LINE__);
                }

                /** Writes the caption to a log file. */
                template <typename t_caption_type>
                void log_caption(const t_caption_type& caption) noexcept
                {
                    std::ofstream o(this->m_log_filename, std::ios_base::app); // Try to open the file for writing.
                    if (!o.good())
                    {
                        aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Could not write to log file.", __FUNCTION__, __LINE__);
                        return; // Stop on failure.
                    }
                    o << caption << std::endl;
                    o << this->m_model << std::endl;
                }

                /** Write the brief summary to a log file. */
                void log() noexcept
                {
                    std::ofstream o(this->m_log_filename, std::ios_base::app); // Try to open the file for writing.
                    if (!o.good())
                    {
                        aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Could not write to log file.", __FUNCTION__, __LINE__);
                        return; // Stop on failure.
                    }

                    for (const auto& p : this->m_fsprts) o << p << std::endl;
                    for (const auto& p : this->m_asprts) o << p << std::endl;
                    for (const auto& p : this->m_gsprts) o << p << std::endl;
                    for (const auto& p : this->m_hsprts) o << p << std::endl;
                }

                /** Write the results to a .mat file. */
                void mat() noexcept
                {
                    this->generate_mat_prefix();
                    if (!aftermath::quiet_error::instance().good()) return;

                    for (const auto& p : this->m_fsprts) this->write_mat(p);
                    for (const auto& p : this->m_asprts) this->write_mat(p);
                    for (const auto& p : this->m_gsprts) this->write_mat(p);
                    for (const auto& p : this->m_hsprts) this->write_mat(p);
                }
                
                explicit observer_bunch(const model_type& model) noexcept
                    : m_model(model)
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

                /** Fixes the output file, so that results of several simulations are stored in the same place. */
                void fix_mat_prefix() noexcept { this->m_do_fix_mat_prefix = true; }

                void set_var_names(const std::string& error_var_name, const std::string& run_length_var_name, const std::string& suffix) noexcept
                {
                    this->m_error_var_name = error_var_name + "_" + suffix;
                    this->m_run_length_var_name = run_length_var_name + "_" + suffix;
                }

                void set_output_to(const std::string& log_filename, const std::string& mat_folder) noexcept
                {
                    if (log_filename.empty()) aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Log filename cannot be empty.", __FUNCTION__, __LINE__);
                    if (mat_folder.empty()) aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Mat folder path cannot be empty.", __FUNCTION__, __LINE__);
                    if (!aftermath::quiet_error::instance().good()) return;

                    // Now that the trivial checks have been made, see if the paths are accessible.
                    if (!filesystem::can_write_surely(log_filename, __FUNCTION__, __LINE__)) return;
                    if (!filesystem::can_write_surely(filesystem::path_combine(mat_folder, "dummy.mat"), __FUNCTION__, __LINE__)) return;

                    this->m_log_filename = log_filename;
                    this->m_mat_folder = mat_folder;
                }

                /** Set the signal "strength" conrresponding to what measure we want to analyze and an auxiliary quantity to improve accuracy of statistics. */
                void look_for(double analyzed_mu, double expected_run_length) noexcept
                {
                    if (!this->m_has_parsed)
                    {
                        aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, "Parsing has to be done prior to initialization.", __FUNCTION__, __LINE__);
                        return;
                    }
                    this->m_has_been_initialized = true;

                    for (auto& p : this->m_fsprts) p.look_for(analyzed_mu, expected_run_length);
                    for (auto& p : this->m_asprts) p.look_for(analyzed_mu, expected_run_length);
                    for (auto& p : this->m_gsprts) p.look_for(analyzed_mu, expected_run_length);
                    for (auto& p : this->m_hsprts) p.look_for(analyzed_mu, expected_run_length);
                }

                /** Indicates if at least one of the underlying stopping times is still running. */
                bool is_running() const noexcept 
                {
                    for (const auto& p : this->m_fsprts) if (p.is_running()) return true;
                    for (const auto& p : this->m_asprts) if (p.is_running()) return true;
                    for (const auto& p : this->m_gsprts) if (p.is_running()) return true;
                    for (const auto& p : this->m_hsprts) if (p.is_running()) return true;
                    return false;
                }

                /** Listens to new observations from \p proc. */
                void tic(const process_type& proc) noexcept 
                {
                    for (auto& p : this->m_fsprts) p.tic(proc);
                    for (auto& p : this->m_asprts) p.tic(proc);
                    for (auto& p : this->m_gsprts) p.tic(proc);
                    for (auto& p : this->m_hsprts) p.tic(proc);
                }
            
                /** Completes the sequence of observations from \p proc. */
                void toc(const process_type& proc) noexcept
                {
                    for (auto& p : this->m_fsprts) p.toc(proc);
                    for (auto& p : this->m_asprts) p.toc(proc);
                    for (auto& p : this->m_gsprts) p.toc(proc);
                    for (auto& p : this->m_hsprts) p.toc(proc);
                }
                
                /** Parses \p json entry to construct and add another stopping time observer to the collection. */
                bool try_parse(const nlohmann::json& json) noexcept
                {
                    if (this->m_has_been_initialized)
                    {
                        aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, "Parsing has to be done before initialization is completed.", __FUNCTION__, __LINE__);
                        return false;
                    }
                    this->m_has_parsed = true;

                    std::string proc_name = json["name"];
                    std::string type_name = json["type"];
                    std::string threshold_spacing = json["threshold spacing"];
                    std::size_t threshold_count = json["threshold count"];
                    std::vector<double> threshold_range_null = json["threshold range null"];
                    std::vector<double> threshold_range_alt = json["threshold range alt"];

                    if (threshold_range_null.size() != 2 || threshold_range_alt.size() != 2)
                    {
                        aftermath::quiet_error::instance().push(aftermath::not_an_error::all_good, "Threshold range should be a vector with two entries.", __FUNCTION__, __LINE__);
                        return false;
                    }

                    std::vector<double> thresholds_null = matlab::parse_space(threshold_spacing, threshold_range_null[0], threshold_range_null[1], threshold_count);
                    std::vector<double> thresholds_alt = matlab::parse_space(threshold_spacing, threshold_range_alt[0], threshold_range_alt[1], threshold_count);
            
                    if (type_name == "adaptive_sprt_star")
                    {
                        double relative_guess_mu_null = json["relative mu guess for null"]; // Initial guess for signal "strength" for adaptive LLR under the null hypothesis.
                        double relative_guess_mu_alt = json["relative mu guess for alt"]; // Initial guess for signal "strength" for adaptive LLR under the alternative hypothesis.
                        return detail::try_push_back(this->m_fsprts, type_name, proc_name, thresholds_null, thresholds_alt, this->m_model, relative_guess_mu_null, relative_guess_mu_alt);
                    }
                    if (type_name == "adaptive_sprt")
                    {
                        double relative_guess_mu_null = json["relative mu guess for null"];
                        double relative_guess_mu_alt = json["relative mu guess for alt"];
                        return detail::try_push_back(this->m_asprts, type_name, proc_name, thresholds_null, thresholds_alt, this->m_model, relative_guess_mu_null, relative_guess_mu_alt);
                    }
                    if (type_name == "generalized_sprt")
                    {
                        return detail::try_push_back(this->m_gsprts, type_name, proc_name, thresholds_null, thresholds_alt, this->m_model);
                    }
                    if (type_name == "generalized_sprt_star")
                    {
                        double relative_mu_star = json["relative mu star"];
                        return detail::try_push_back(this->m_hsprts, type_name, proc_name, thresholds_null, thresholds_alt, this->m_model, relative_mu_star);
                    }
                    aftermath::quiet_error::instance().push(aftermath::not_an_error::all_good, "Type name not recognized.", __FUNCTION__, __LINE__);
                    return false;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_BUNCH_HPP_INCLUDED
