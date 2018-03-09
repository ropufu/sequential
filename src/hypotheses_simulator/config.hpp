
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_INCLUDED

#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include <nlohmann/json.hpp>
#include "../hypotheses/json.hpp"

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/rules.hpp"

#include "run.hpp"

#include <fstream>  // std::ifstream, std::ofstream
#include <iostream> // std::ostream
#include <string>   // std::string
#include <utility>  // std::pair
#include <variant>  // std::variant, std::visit
#include <vector>   // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Class for reading and writing configurtation setting. */
            template <typename t_value_type>
            struct config
            {
                using type = config<t_value_type>;
                using value_type = t_value_type;
                using signal_variant_type = std::variant<
                    unit_signal<t_value_type>,
                    constant_signal<t_value_type>,
                    transitionary_signal<t_value_type, 1>
                    /**transitionary_signal<t_value_type, 2>*/>;
                using noise_variant_type = std::variant<
                    white_noise<t_value_type>,
                    auto_regressive_noise<t_value_type, 1>
                    /**auto_regressive_noise<t_value_type, 2>*/>;
                using rule_type = hypotheses::xsprt<no_signal_t<value_type>, no_noise_t<value_type>, false>;

                // ~~ Json names ~~
                static constexpr char jstr_mat_output_path[] = "mat output";
                static constexpr char jstr_count_simulations[] = "simulations";
                static constexpr char jstr_count_threads[] = "threads";
                static constexpr char jstr_count_interpolated_runs[] = "interpolated runs";
                static constexpr char jstr_signal[] = "signal";
                static constexpr char jstr_noise[] = "noise";
                static constexpr char jstr_rules[] = "rules";
                static constexpr char jstr_runs[] = "runs";

            private:
                // ~~ General configuration ~~
                bool m_is_good = false;
                bool m_has_changed = false;
                std::string m_filename = ""; // Where the configuration was loaded from.
                nlohmann::json m_json = { }; // Raw configuration json.
                // ~~ Specific properties ~~
                std::string m_mat_output_path = "./mat/";
                std::size_t m_count_simulations = 10'000;
                std::size_t m_count_threads = 1;
                std::size_t m_count_interpolated_runs = 0;
                signal_variant_type m_signal = { };
                noise_variant_type m_noise = { };
                std::vector<rule_type> m_rules = { };
                std::vector<run<value_type>> m_runs = { };

            protected:
                // Always try to read the default configuration on construction.
                config() noexcept { this->read(); }
                // Always try to save the configuration on exit.
                ~config() noexcept { this->write(); }

            public:
                /** @brief Output configuration parameters. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    os << self.m_json;
                    return os;
                } // operator <<(...)

                bool good() const noexcept { return this->m_is_good; }

                const std::string& filename() const noexcept { return this->m_filename; }

                const std::string& mat_output_path() const noexcept { return this->m_mat_output_path; }
                void set_mat_output_path(const std::string& value) noexcept { this->m_mat_output_path = value; this->m_has_changed = true; }
                
                std::size_t simulation_count() const noexcept { return this->m_count_simulations; }
                void set_simulation_count(std::size_t value) noexcept { this->m_count_simulations = value; this->m_has_changed = true; }

                std::size_t threads() const noexcept { return this->m_count_threads; }
                void set_threads(std::size_t value) noexcept { this->m_count_threads = value; this->m_has_changed = true; }

                std::size_t interpolated_runs() const noexcept { return this->m_count_interpolated_runs; }
                void set_interpolated_runs(std::size_t value) noexcept { this->m_count_interpolated_runs = value; this->m_has_changed = true; }

                const signal_variant_type& signal() const noexcept { return this->m_signal; }

                const noise_variant_type& noise() const noexcept { return this->m_noise; }

                const std::vector<rule_type>& rules() const noexcept { return this->m_rules; }

                const std::vector<run<value_type>>& runs() const noexcept { return this->m_runs; }

                /** Read the configuration from a file. */
                bool read(const std::string& filename = "./simulator.config") noexcept
                {
                    std::ifstream i(filename); // Try to open the file for reading.
                    if (!i.good()) return false; // Stop on failure.

                    try
                    {
                        this->m_filename = filename; // Remember the filename.

                        i >> this->m_json;
                        this->m_is_good = true;
                        const nlohmann::json& j = this->m_json;

                        // Populate default values.
                        std::string mat_output_path = this->m_mat_output_path;
                        std::size_t count_simulations = this->m_count_simulations;
                        std::size_t count_threads = this->m_count_threads;
                        std::size_t count_interpolated_runs = this->m_count_interpolated_runs;
                        signal_variant_type signal = this->m_signal;
                        noise_variant_type noise = this->m_noise;
                        std::vector<rule_type> rules = this->m_rules;
                        std::vector<run<value_type>> runs = this->m_runs;
                        
                        // Parse json entries.
                        if (!quiet_json::optional(j, type::jstr_mat_output_path, mat_output_path)) return false;
                        if (!quiet_json::optional(j, type::jstr_count_simulations, count_simulations)) return false;
                        if (!quiet_json::optional(j, type::jstr_count_threads, count_threads)) return false;
                        if (!quiet_json::optional(j, type::jstr_count_interpolated_runs, count_interpolated_runs)) return false;
                        if (quiet_json::is_missing(j, type::jstr_signal)) return false;
                        if (quiet_json::is_missing(j, type::jstr_noise)) return false;
                        // Signal discriminator.
                        if (!hypotheses::try_discriminate_signal(j[type::jstr_signal], signal))
                        {
                            aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "Signal not recognized.", __FUNCTION__, __LINE__);
                            return false;
                        }
                        // Noise discriminator.
                        if (!hypotheses::try_discriminate_noise(j[type::jstr_noise], noise))
                        {
                            aftermath::quiet_error::instance().push(aftermath::not_an_error::logic_error, aftermath::severity_level::major, "Noise not recognized.", __FUNCTION__, __LINE__);
                            return false;
                        }
                        // Read rules.
                        if (quiet_json::is_missing(j, type::jstr_rules)) return false;
                        rules.clear();
                        for (const nlohmann::json& k : j[type::jstr_rules])
                        {
                            rule_type x = k;
                            rules.push_back(x);
                        }
                        // Read runs.
                        if (quiet_json::is_missing(j, type::jstr_runs)) return false;
                        runs.clear();
                        for (const nlohmann::json& k : j[type::jstr_runs])
                        {
                            run<value_type> x = k;
                            runs.push_back(x);
                        }

                        this->m_mat_output_path = mat_output_path;
                        this->m_count_simulations = count_simulations;
                        this->m_count_threads = count_threads;
                        this->m_count_interpolated_runs = count_interpolated_runs;
                        this->m_signal = signal;
                        this->m_noise = noise;
                        this->m_rules = rules;
                        this->m_runs = runs;

                        return true;
                    }
                    catch (...)
                    {
                        this->m_is_good = false;
                        this->m_json = { };
                        return false;
                    }
                } // read(...)
                
                /** Write the configuration to a file. */
                bool write() noexcept { return this->write(this->m_filename); }

                /** Write the configuration to a file. */
                bool write(const std::string& filename) noexcept
                {
                    if (!this->m_has_changed) return true;

                    std::ofstream o(filename); // Try to open the file for writing.
                    if (!o.good()) return false; // Stop on failure.
                    nlohmann::json& j = this->m_json;

                    j[type::jstr_mat_output_path] = this->m_mat_output_path;
                    j[type::jstr_count_simulations] = this->m_count_simulations;
                    j[type::jstr_count_threads] = this->m_count_threads;
                    j[type::jstr_count_interpolated_runs] = this->m_count_interpolated_runs;
                    std::visit([&] (auto&& arg) { j[type::jstr_signal] = arg; }, this->m_signal);
                    std::visit([&] (auto&& arg) { j[type::jstr_noise] = arg; }, this->m_noise);
                    j[type::jstr_rules] = this->m_rules;
                    j[type::jstr_runs] = this->m_runs;

                    try
                    {
                        o << std::setw(4) << this->m_json << std::endl;
                        this->m_has_changed = false;
                        return true;
                    }
                    catch (...)
                    {
                        return false;
                    }
                } // write(...)

                /** The only instance of this type. */
                static type& instance() noexcept
                {
                    // Since it's a static variable, if the class has already been created, it won't be created again.
                    // Note: it is thread-safe in C++11.
                    static type s_instance { };
                    // Return a reference to our instance.
                    return s_instance;
                } // instance(...)
    
                // ~~ Delete copy and move constructors and assign operators ~~
                config(const type&) = delete; // Copy constructor.
                config(type&&)      = delete; // Move constructor.
                type& operator =(const type&) = delete; // Copy assign.
                type& operator =(type&&)      = delete; // Move assign.
            }; // config

            // ~~ Json name definitions ~~
            template <typename t_value_type> constexpr char config<t_value_type>::jstr_mat_output_path[];
            template <typename t_value_type> constexpr char config<t_value_type>::jstr_count_simulations[];
            template <typename t_value_type> constexpr char config<t_value_type>::jstr_count_threads[];
            template <typename t_value_type> constexpr char config<t_value_type>::jstr_count_interpolated_runs[];
            template <typename t_value_type> constexpr char config<t_value_type>::jstr_signal[];
            template <typename t_value_type> constexpr char config<t_value_type>::jstr_noise[];
            template <typename t_value_type> constexpr char config<t_value_type>::jstr_rules[];
            template <typename t_value_type> constexpr char config<t_value_type>::jstr_runs[];
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_INCLUDED
