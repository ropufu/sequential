
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

#include <cstddef>     // std::size_t
#include <filesystem>  // std::filesystem::path
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <variant>     // std::variant, std::visit
#include <vector>      // std::vector

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
        static constexpr std::string_view jstr_mat_output_path = "mat output";
        static constexpr std::string_view jstr_count_simulations = "simulations";
        static constexpr std::string_view jstr_count_threads = "threads";
        static constexpr std::string_view jstr_signal = "signal";
        static constexpr std::string_view jstr_noise = "noise";
        static constexpr std::string_view jstr_disable_oc_pass = "disable oc pass";
        static constexpr std::string_view jstr_disable_gray_pass = "disable gray pass";
        static constexpr std::string_view jstr_do_limiting_distribution = "limiting distribution only";
        static constexpr std::string_view jstr_limiting_observations = "limiting observations";
        static constexpr std::string_view jstr_limiting_cutoff_time = "limiting cutoff time";
        static constexpr std::string_view jstr_rule_designs = "rules";
        static constexpr std::string_view jstr_runs = "runs";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        std::filesystem::path m_mat_output_path = "./mat/";
        std::size_t m_count_simulations = 1'000;
        std::size_t m_count_threads = 1;
        signal_type m_signal = {};
        noise_type m_noise = {};
        bool m_disable_oc_pass = false;
        bool m_disable_gray_pass = false;
        bool m_do_limiting_distribution = false;
        std::size_t m_limiting_observations = 1'000;
        std::size_t m_limiting_cutoff_time = 5'000;
        std::vector<design_variant_type> m_rule_designs = {};
        std::vector<run_type> m_runs = {};

    public:
        const std::filesystem::path& mat_output_path() const noexcept { return this->m_mat_output_path; }

        std::size_t count_simulations() const noexcept { return this->m_count_simulations; }

        std::size_t count_threads() const noexcept { return this->m_count_threads; }

        const signal_type& signal() const noexcept { return this->m_signal; }

        const noise_type& noise() const noexcept { return this->m_noise; }
        
        bool disable_oc_pass() const noexcept { return this->m_disable_oc_pass; }
        bool disable_gray_pass() const noexcept { return this->m_disable_gray_pass; }
        
        bool do_limiting_distribution() const noexcept { return this->m_do_limiting_distribution; }

        std::size_t limiting_observations() const noexcept { return this->m_limiting_observations; }
        std::size_t limiting_cutoff_time() const noexcept { return this->m_limiting_cutoff_time; }

        const std::vector<design_variant_type>& rule_designs() const noexcept { return this->m_rule_designs; }

        const design_variant_type& rule_design_by_id(std::size_t id) const
        {
            for (const design_variant_type& v : this->m_rule_designs)
            {
                std::size_t rule_id = std::visit([] (auto&& arg) { return arg.id(); }, v);
                if (rule_id == id) return v;
            } // for (...)
            throw std::runtime_error("Rule design with id " + std::to_string(id) + " not found.");
        } // rule_design_by_id(...)

        bool has_rule_design(std::size_t id) const noexcept
        {
            for (const design_variant_type& v : this->m_rule_designs)
            {
                std::size_t rule_id = std::visit([] (auto&& arg) { return arg.id(); }, v);
                if (rule_id == id) return true;
            } // for (...)
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
            {type::jstr_disable_gray_pass, x.disable_gray_pass()},
            {type::jstr_do_limiting_distribution, x.do_limiting_distribution()},
            {type::jstr_limiting_observations, x.limiting_observations()},
            {type::jstr_limiting_cutoff_time, x.limiting_cutoff_time()},
            {type::jstr_rule_designs, x.rule_designs()},
            {type::jstr_runs, x.runs()}
        };
    } // to_json(...)

    template <typename t_engine_type, typename t_value_type>
    void from_json(const nlohmann::json& j, config<t_engine_type, t_value_type>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <config> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_engine_type, typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::config<t_engine_type, t_value_type>>
    {
        using engine_type = t_engine_type;
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::config<t_engine_type, t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            std::string mat_output_path {};
            
            // Parse json entries.
            if (!noexcept_json::optional(j, result_type::jstr_mat_output_path, mat_output_path)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_count_simulations, x.m_count_simulations)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_count_threads, x.m_count_threads)) return false;
            if (!noexcept_json::required(j, result_type::jstr_signal, x.m_signal)) return false;
            if (!noexcept_json::required(j, result_type::jstr_noise, x.m_noise)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_disable_oc_pass, x.m_disable_oc_pass)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_disable_gray_pass, x.m_disable_gray_pass)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_do_limiting_distribution, x.m_do_limiting_distribution)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_limiting_observations, x.m_limiting_observations)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_limiting_cutoff_time, x.m_limiting_cutoff_time)) return false;
            if (!noexcept_json::required(j, result_type::jstr_rule_designs, x.m_rule_designs)) return false;
            if (!noexcept_json::required(j, result_type::jstr_runs, x.m_runs)) return false;

            // Populate values.
            x.m_mat_output_path = ropufu::sequential::hypotheses::detail::format_homedir_path(mat_output_path);
            
            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_CONFIG_INCLUDED
