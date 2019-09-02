
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_CONFIG_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_CONFIG_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include "../hypotheses/signals.hpp"
#include "../hypotheses/noises.hpp"
#include "../hypotheses/simple_process.hpp"
#include "../hypotheses/rules.hpp"

#include "homedir.hpp"
#include "run.hpp"

#include <cstddef>      // std::size_t
#include <iostream>     // std::ostream
#include <filesystem>   // std::filesystem::path
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string
#include <system_error> // std::error_code, std::errc
#include <variant>      // std::variant
#include <vector>       // std::vector

namespace ropufu::sequential::hypotheses
{
    /** @brief Class for reading and writing configurtation setting. */
    template <typename t_engine_type, typename t_value_type>
    struct config
    {
        using type = config<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using signal_type = hypotheses::constant_signal<value_type>;
        using noise_type = hypotheses::white_noise<engine_type, value_type>;
        using process_type = hypotheses::simple_process<engine_type, value_type>;

        using design_variant_type = hypotheses::rule_design_variant<value_type>;
        using run_type = hypotheses::run<value_type>;

        // ~~ Json names ~~
        static constexpr char jstr_mat_output_path[] = "mat output";
        static constexpr char jstr_count_simulations[] = "simulations";
        static constexpr char jstr_count_threads[] = "threads";
        static constexpr char jstr_signal[] = "signal";
        static constexpr char jstr_noise[] = "noise";
        static constexpr char jstr_disable_oc_pass[] = "disable oc pass";
        static constexpr char jstr_rule_designs[] = "rules";
        static constexpr char jstr_runs[] = "runs";

    private:
        std::filesystem::path m_mat_output_path = "./mat/";
        bool m_disable_oc_pass = false;
        std::size_t m_count_simulations = 1'000;
        std::size_t m_count_threads = 1;
        signal_type m_signal = {};
        noise_type m_noise = {};
        std::vector<design_variant_type> m_rule_designs = {};
        std::vector<run_type> m_runs = {};

    public:
        config(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Populate default values.
            std::string mat_output_path = this->m_mat_output_path.string();
            std::size_t count_simulations = this->m_count_simulations;
            std::size_t count_threads = this->m_count_threads;
            signal_type signal = this->m_signal;
            noise_type noise = this->m_noise;
            bool disable_oc_pass = this->m_disable_oc_pass;
            std::vector<design_variant_type> rule_designs = this->m_rule_designs;
            std::vector<run_type> runs = this->m_runs;
            
            // Parse json entries.
            aftermath::noexcept_json::optional(j, type::jstr_mat_output_path, mat_output_path, ec);
            aftermath::noexcept_json::optional(j, type::jstr_count_simulations, count_simulations, ec);
            aftermath::noexcept_json::optional(j, type::jstr_count_threads, count_threads, ec);
            aftermath::noexcept_json::required(j, type::jstr_signal, signal, ec);
            aftermath::noexcept_json::required(j, type::jstr_noise, noise, ec);
            aftermath::noexcept_json::optional(j, type::jstr_disable_oc_pass, disable_oc_pass, ec);
            aftermath::noexcept_json::required(j, type::jstr_rule_designs, rule_designs, ec);
            aftermath::noexcept_json::required(j, type::jstr_runs, runs, ec);
            if (ec.value() != 0) return;

            // Populate values.
            this->m_mat_output_path = detail::format_homedir_path(mat_output_path);
            this->m_count_simulations = count_simulations;
            this->m_count_threads = count_threads;
            this->m_signal = signal;
            this->m_noise = noise;
            this->m_disable_oc_pass = disable_oc_pass;
            this->m_rule_designs = rule_designs;
            this->m_runs = runs;
        } // config(...)

        const std::filesystem::path& mat_output_path() const noexcept { return this->m_mat_output_path; }
        //void set_mat_output_path(const std::string& value) noexcept { this->m_mat_output_path = value; this->m_has_changed = true; }

        std::size_t count_simulations() const noexcept { return this->m_count_simulations; }
        //void set_simulation_count(std::size_t value) noexcept { this->m_count_simulations = value; this->m_has_changed = true; }

        std::size_t count_threads() const noexcept { return this->m_count_threads; }
        //void set_threads(std::size_t value) noexcept { this->m_count_threads = value; this->m_has_changed = true; }

        const signal_type& signal() const noexcept { return this->m_signal; }

        const noise_type& noise() const noexcept { return this->m_noise; }
        
        bool disable_oc_pass() const noexcept { return this->m_disable_oc_pass; }
        //void set_disable_oc_pass(bool value) noexcept { this->m_disable_oc_pass = value; this->m_has_changed = true; }

        const std::vector<design_variant_type>& rule_designs() const noexcept { return this->m_rule_designs; }

        const design_variant_type& rule_design_by_id(std::size_t id) const
        {
            for (const design_variant_type& v : this->m_rule_designs) if (v.id() == id) return v;
            throw std::runtime_error("Rule design with id " + std::to_string(id) + " not found.");
        } // rule_design_by_id(...)

        bool has_rule_design(std::size_t id) const noexcept
        {
            for (const design_variant_type& v : this->m_rule_designs) if (v.id() == id) return true;
            return false;
        } // has_rule_design(...)

        const std::vector<run_type>& runs() const noexcept { return this->m_runs; }

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct config

    // ~~ Json name definitions ~~
    template <typename t_engine_type, typename t_value_type> constexpr char config<t_engine_type, t_value_type>::jstr_mat_output_path[];
    template <typename t_engine_type, typename t_value_type> constexpr char config<t_engine_type, t_value_type>::jstr_count_simulations[];
    template <typename t_engine_type, typename t_value_type> constexpr char config<t_engine_type, t_value_type>::jstr_count_threads[];
    template <typename t_engine_type, typename t_value_type> constexpr char config<t_engine_type, t_value_type>::jstr_signal[];
    template <typename t_engine_type, typename t_value_type> constexpr char config<t_engine_type, t_value_type>::jstr_noise[];
    template <typename t_engine_type, typename t_value_type> constexpr char config<t_engine_type, t_value_type>::jstr_disable_oc_pass[];
    template <typename t_engine_type, typename t_value_type> constexpr char config<t_engine_type, t_value_type>::jstr_rule_designs[];
    template <typename t_engine_type, typename t_value_type> constexpr char config<t_engine_type, t_value_type>::jstr_runs[];
    
    template <typename t_engine_type, typename t_value_type>
    void to_json(nlohmann::json& j, const config<t_engine_type, t_value_type>& x) noexcept
    {
        using type = config<t_engine_type, t_value_type>;

        j = nlohmann::json{
            {type::jstr_mat_output_path, x.mat_output_path()},
            {type::jstr_count_simulations, x.count_simulations()},
            {type::jstr_count_threads, x.count_threads()},
            {type::jstr_signal, x.signal()},
            {type::jstr_noise, x.noise()},
            {type::jstr_disable_oc_pass, x.disable_oc_pass()},
            {type::jstr_rule_designs, x.rule_designs()},
            {type::jstr_runs, x.runs()}
        };
    } // to_json(...)

    template <typename t_engine_type, typename t_value_type>
    void from_json(const nlohmann::json& j, config<t_engine_type, t_value_type>& x)
    {
        using type = config<t_engine_type, t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <config> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_CONFIG_INCLUDED
