
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_INCLUDED

#include <nlohmann/json.hpp>

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/rules.hpp"

#include "run.hpp"

#include <fstream>  // std::ifstream, std::ofstream
#include <iostream> // std::ostream
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string, std::to_string
#include <system_error> // std::error_code, std::make_error_code, std::errc
#include <utility>  // std::pair
#include <variant>  // std::variant, std::visit
#include <vector>   // std::vector

namespace ropufu::sequential::hypotheses
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
            /*transitionary_signal<t_value_type, 2>*/>;
        using noise_variant_type = std::variant<
            white_noise<t_value_type>,
            auto_regressive_noise<t_value_type, 1>
            /*auto_regressive_noise<t_value_type, 2>*/>;
        using design_variant_type = std::variant<
            adaptive_sprt_a_design<t_value_type>,
            adaptive_sprt_b_design<t_value_type>,
            double_sprt_design<t_value_type>,
            generalized_sprt_a_design<t_value_type>,
            generalized_sprt_b_design<t_value_type>>;

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
        nlohmann::json m_json = {}; // Raw configuration json.
        std::vector<std::string> m_logger = {};
        // ~~ Specific properties ~~
        std::string m_mat_output_path = "./mat/";
        std::size_t m_count_simulations = 10'000;
        std::size_t m_count_threads = 1;
        std::size_t m_count_interpolated_runs = 0;
        signal_variant_type m_signal = {};
        noise_variant_type m_noise = {};
        std::vector<design_variant_type> m_rules = {};
        std::vector<run<value_type>> m_runs = {};

    public:
        // Always try to read the default configuration on construction.
        explicit config(const std::string& filename) noexcept { this->read(filename); }
        
        // Always try to save the configuration on exit.
        ~config() noexcept { this->write(); }

        /** @brief Output configuration parameters. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            os << self.m_json;
            return os;
        } // operator <<(...)

        bool good() const noexcept { return this->m_is_good; }

        const std::vector<std::string>& log() const noexcept { return this->m_logger; }
        void clear_log() noexcept { this->m_logger.clear(); }

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

        const std::vector<design_variant_type>& rules() const noexcept { return this->m_rules; }

        const std::vector<run<value_type>>& runs() const noexcept { return this->m_runs; }

        /** Read the configuration from a file. */
        bool read(const std::string& filename) noexcept
        {
            std::ifstream i(filename); // Try to open the file for reading.
            if (!i.good()) return false; // Stop on failure.

            try
            {
                this->m_filename = filename; // Remember the filename.

                i >> this->m_json;
                this->m_is_good = false;
                const nlohmann::json& j = this->m_json;
                std::error_code ec {};

                // Populate default values.
                std::string mat_output_path = this->m_mat_output_path;
                std::size_t count_simulations = this->m_count_simulations;
                std::size_t count_threads = this->m_count_threads;
                std::size_t count_interpolated_runs = this->m_count_interpolated_runs;
                std::vector<design_variant_type> rules = this->m_rules;
                std::vector<run<value_type>> runs = this->m_runs;
                
                // Parse json entries.
                aftermath::noexcept_json::optional(j, type::jstr_mat_output_path, mat_output_path, ec);
                aftermath::noexcept_json::optional(j, type::jstr_count_simulations, count_simulations, ec);
                aftermath::noexcept_json::optional(j, type::jstr_count_threads, count_threads, ec);
                aftermath::noexcept_json::optional(j, type::jstr_count_interpolated_runs, count_interpolated_runs, ec);
                aftermath::noexcept_json::required(j, type::jstr_runs, runs, ec);

                signal_variant_type signal = this->m_signal;
                noise_variant_type noise = this->m_noise;
                
                if (j.count(type::jstr_signal) == 0) { this->m_logger.push_back("Signal descriptor missing."); return false; }
                if (j.count(type::jstr_noise) == 0) { this->m_logger.push_back("Noise descriptor missing."); return false; }
                if (j.count(type::jstr_rules) == 0) { this->m_logger.push_back("Rules descriptor missing."); return false; }

                const nlohmann::json& signal_json = j[type::jstr_signal];
                const nlohmann::json& noise_json = j[type::jstr_noise];
                const nlohmann::json& rules_json = j[type::jstr_rules];

                // Reconstruct the object.
                if (ec.value() != 0)
                {
                    this->m_logger.push_back(ec.message());
                    return false;
                } // if (...)

                // Signal discriminator.
                if (!hypotheses::try_discriminate_signal(signal_json, signal))
                {
                    this->m_logger.push_back(std::string("Signal not recognized: ") + signal_json.dump() + std::string("."));
                    return false;
                } // if (...)
                // Noise discriminator.
                if (!hypotheses::try_discriminate_noise(noise_json, noise))
                {
                    this->m_logger.push_back(std::string("Noise not recognized: ") + noise_json.dump() + std::string("."));
                    return false;
                } // if (...)
                
                if (!rules_json.is_array()) { this->m_logger.push_back("Rules descriptor must be an array."); return false; }
                for (const nlohmann::json& r_json : rules_json)
                {
                    design_variant_type rule {};
                    if (!hypotheses::try_discriminate_rule(r_json, rule))
                    {
                        this->m_logger.push_back(std::string("Rule not recognized: ") + r_json.dump() + std::string("."));
                        return false;
                    } // if (...)
                    rules.push_back(rule);
                } // for (...)

                this->m_mat_output_path = mat_output_path;
                this->m_count_simulations = count_simulations;
                this->m_count_threads = count_threads;
                this->m_count_interpolated_runs = count_interpolated_runs;
                this->m_signal = signal;
                this->m_noise = noise;
                this->m_rules = rules;
                this->m_runs = runs;

                this->m_is_good = true;
                return true;
            } // try
            catch (...)
            {
                this->m_is_good = false;
                this->m_json = {};
                return false;
            } // catch (...)
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
            nlohmann::json rules_json = nlohmann::json::array();
            for (const design_variant_type& rule : this->m_rules)
            {
                std::visit([&] (auto&& arg) { nlohmann::json k = arg; rules_json.push_back(k); }, rule);
            } // for (...)
            j[type::jstr_rules] = rules_json;
            j[type::jstr_runs] = this->m_runs;

            try
            {
                o << std::setw(4) << this->m_json << std::endl;
                this->m_has_changed = false;
                return true;
            } // try
            catch (...)
            {
                return false;
            } // catch (...)
        } // write(...)
    }; // struct config

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char config<t_value_type>::jstr_mat_output_path[];
    template <typename t_value_type> constexpr char config<t_value_type>::jstr_count_simulations[];
    template <typename t_value_type> constexpr char config<t_value_type>::jstr_count_threads[];
    template <typename t_value_type> constexpr char config<t_value_type>::jstr_count_interpolated_runs[];
    template <typename t_value_type> constexpr char config<t_value_type>::jstr_signal[];
    template <typename t_value_type> constexpr char config<t_value_type>::jstr_noise[];
    template <typename t_value_type> constexpr char config<t_value_type>::jstr_rules[];
    template <typename t_value_type> constexpr char config<t_value_type>::jstr_runs[];
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_INCLUDED
