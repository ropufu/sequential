
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_WRITER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_WRITER_HPP_INCLUDED

#include <aftermath/algebra.hpp>      // aftermath::algebra::matrix
#include <aftermath/format.hpp>       // aftermath::format::matstream<4>
#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include "../hypotheses/model.hpp"
#include "../hypotheses/moment_statistic.hpp"

#include "config.hpp"
#include "homedir.hpp"
#include "operating_characteristic.hpp"

#include <cstddef>      // std::size_t
#include <string>       // std::string
#include <system_error> // std::error_code

#if __cplusplus > 201703L || _MSVC_LANG > 201703L
#include <filesystem>   // std::filesystem::path, std::filesystem::exists, std::filesystem::create_directories, std::filesystem::copy_file
namespace fs = std::filesystem;
#else
#include <experimental/filesystem> // ...
namespace fs = std::experimental::filesystem::v1;
#endif

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** Takes care of writing .mat output. */
            template <typename t_value_type>
            struct writer
            {
                using type = writer<t_value_type>;
                using value_type = t_value_type;

                using model_type = hypotheses::model<value_type>;
                using matstream_type = aftermath::format::matstream<4>;

                template <typename t_data_type>
                using matrix_t = aftermath::algebra::matrix<t_data_type>;
                using statistic_type = hypotheses::moment_statistic<matrix_t<value_type>>;

                using congif_type = config<value_type>;

            private:
                bool m_is_good = false;
                fs::path m_root = "."; // Where to write output to.
                fs::path m_mat_subfolder = "."; // Path to subfolder, storing the mat files.
                fs::path m_config_path = "."; // Path to a copy of config file.

                /** @brief Generates prefix for .mat files, and copies the config file to the output folder.
                 *  @remark If prefix has been fixed, and has already been generated, does nothing.
                 */
                template <std::size_t t_prefix_size = 3>
                void initialize_subfolder(const model_type& model)
                {
                    static_assert(t_prefix_size >= 1 && t_prefix_size <= 8, "Prefix size has to be an integer between 1 and 8.");
                    std::error_code err { };

                    char letter_from = 'a';
                    char letter_to = 'z';

                    std::array<char, t_prefix_size> prefix { };
                    prefix.fill(letter_from);

                    err.clear();
                    while (true)
                    {
                        // Build subfolder name: "<prefix> space <string representation of model>".
                        std::string subfolder_name = "";
                        for (std::size_t i = 0; i < t_prefix_size; ++i) subfolder_name += prefix[i];
                        subfolder_name += " ";
                        subfolder_name += model.to_path_string();

                        // Build name for a copy of config file.
                        std::string config_name = subfolder_name + ".json";
                        this->m_mat_subfolder = this->m_root / subfolder_name;
                        this->m_config_path = this->m_root / config_name;

                        // Check that neither exists; otherwise generate another prefix.
                        if (!fs::exists(this->m_mat_subfolder, err) && !fs::exists(this->m_config_path, err)) break;
                        if (err.value() != 0) return;

                        // The prefix is already taken: switch to next one.
                        std::size_t index = t_prefix_size - 1;
                        ++prefix[index];
                        bool is_overflow = prefix[index] > letter_to;
                        while (is_overflow)
                        {
                            prefix[index] = letter_from;
                            if (index == 0) break;
                            --index;
                            ++prefix[index];
                            is_overflow = prefix[index] > letter_to;
                        } // while (...)

                        // Check if we've run out of options.
                        if (is_overflow)
                        {
                            aftermath::quiet_error::instance().push(
                                aftermath::not_an_error::runtime_error,
                                aftermath::severity_level::major,
                                "Name pool exhausted!", __FUNCTION__, __LINE__);
                            return;
                        } // if (...)
                    } // while (...)
                    if (err.value() != 0) return;

                    // Create required directory structure.
                    fs::create_directories(this->m_mat_subfolder, err);
                    if (err.value() != 0) return;

                    // Copy the current configuration to output folder.
                    fs::copy_file(congif_type::instance().filename(), this->m_config_path, err);
                    if (err.value() != 0) return;

                    this->m_is_good = true;
                } // initialize_subfolder(...)

                bool enforce_good(std::string&& caller_function_name, std::size_t caller_line_number) const noexcept
                {
                    if (this->m_is_good) return true;
                    aftermath::quiet_error::instance().push(
                        aftermath::not_an_error::runtime_error,
                        aftermath::severity_level::major,
                        "Mat writer has not been successfully initialized.", caller_function_name, caller_line_number);
                    return false;
                }
            
            public:
                explicit writer(const std::string& output_path, const model_type& model) noexcept
                    : m_root(detail::format_homedir_path(output_path))
                {
                    this->initialize_subfolder(model);
                } // writer(...)

                bool good() const noexcept { return this->m_is_good; }

                template <typename t_rule_type>
                void write_mat(const t_rule_type& rule, const oc_array<typename t_rule_type::statistic_type>& oc)
                {
                    if (!this->enforce_good(__FUNCTION__, __LINE__)) return; // Make sure the writer has been successfully initialized.

                    std::string mat_name = rule.to_path_string() + " oc.mat";
                    fs::path mat_path = this->m_mat_subfolder / mat_name;

                    // @todo Implement a range-based for loop in <oc_array>.
                    const statistic_type& fa = oc[operating_characteristic::probability_of_false_alarm];
                    const statistic_type& ms = oc[operating_characteristic::probability_of_missed_signal];
                    const statistic_type& ss_null = oc[operating_characteristic::ess_under_null];
                    const statistic_type& ss_alt = oc[operating_characteristic::ess_under_alt];

                    std::string mat_path_str = mat_path.string();
                    matstream_type mat(mat_path_str);
                    // mat.clear(); // Clear the existing contents.

                    // @todo Loop through <oc_array>; implement .mat var name function for <operating_characteristic> distinct from the existing std::to_string.
                    mat << "b_null" << rule.unscaled_null_thresholds()
                        << "b_alt" << rule.unscaled_alt_thresholds()
                        << "pfa" << fa.mean() << "vfa" << fa.variance()
                        << "pms" << ms.mean() << "vms" << ms.variance()
                        << "ess_null" << ss_null.mean() << "vss_null" << ss_null.variance()
                        << "ess_alt" << ss_alt.mean() << "vss_alt" << ss_alt.variance();
                }

                template <typename t_rule_type>
                void write_mat(const t_rule_type& rule, const simulation_pair<value_type>& mu_pair)
                {
                    if (!this->enforce_good(__FUNCTION__, __LINE__)) return; // Make sure the writer has been successfully initialized.

                    std::string mat_name = rule.to_path_string() + " more " + mu_pair.to_path_string() + ".mat";
                    fs::path mat_path = this->m_mat_subfolder / mat_name;

                    matrix_t<value_type> analyzed_mu(1, 1, mu_pair.analyzed_mu());
                    matrix_t<value_type> simulated_mu(1, 1, mu_pair.simulated_mu());
                    const statistic_type& errors = rule.errors();
                    const statistic_type& run_lengths = rule.run_lengths();

                    std::string mat_path_str = mat_path.string();
                    matstream_type mat(mat_path_str);
                    // mat.clear(); // Clear the existing contents.

                    mat << "analyzed_mu" << analyzed_mu
                        << "simulated_mu" << simulated_mu
                        << "b_null" << rule.unscaled_null_thresholds()
                        << "b_alt" << rule.unscaled_alt_thresholds()
                        << "perror" << errors.mean() << "verror" << errors.variance()
                        << "ess" << run_lengths.mean() << "vss" << run_lengths.variance();
                }
            }; // struct writer
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_WRITER_HPP_INCLUDED
