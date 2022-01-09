
#include <iostream> // std::cout, std::endl

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <ropufu/random/monte_carlo.hpp>

#include "aggregator.hpp"
#include "config.hpp"
#include "model.hpp"
#include "simulator.hpp"
#include "xsprt.hpp"

#include <array>        // std::array
#include <chrono>       // std::chrono::steady_clock, std::chrono::duration_cast
#include <cmath>        // std::sqrt, std::log10
#include <cstddef>      // std::size_t
#include <filesystem>   // std::filesystem::path
#include <fstream>      // std::ifstream
#include <iomanip>      // std::setw
#include <ios>          // std::ios_base::failure
#include <iostream>     // std::cout, std::endl
#include <random>       // std::mt19937_64

enum struct execution_result : int
{
    all_good = 0,
    failed_to_read_config_file = 1,
    failed_to_parse_config_file = 7
}; // struct execution_result

void separator()
{
    std::cout << "======================================================================" << std::endl;
} // separator(...)

template <typename t_moment_statistic_type, typename t_transform_type>
void cat(const t_moment_statistic_type& stat, t_transform_type&& transform) noexcept
{
    const auto& mat = stat.mean();
    std::size_t m = mat.height();
    std::size_t n = mat.width();
    if (m == 0 || n == 0) return;

    std::cout << std::left;
    std::cout <<
        std::setw(10) << transform(mat(0, 0)) <<
        std::setw(10) << " --- " <<
        std::setw(10) << transform(mat(0, n - 1)) << std::endl;
    std::cout <<
        std::setw(10) << "     " <<
        std::setw(10) << " ... " << std::endl;
    std::cout <<
        std::setw(10) << transform(mat(m - 1, 0)) <<
        std::setw(10) << " --- " <<
        std::setw(10) << transform(mat(m - 1, n - 1)) << std::endl;

    const auto& var = stat.variance();
    auto max_var = *(var.cbegin());
    for (auto x : var) if (x > max_var) max_var = x;
    max_var /= stat.count();
    std::cout << "SE = " << std::sqrt(max_var) << std::endl;
} // cat(...)

template <typename t_moment_statistic_type>
void cat(const t_moment_statistic_type& stat) noexcept
{
    ::cat(stat, [] (auto x) { return x; });
} // cat(...)

template <typename t_value_type, typename t_engine_type, std::size_t t_count_threads>
struct program
{
    using type = program<t_value_type, t_engine_type, t_count_threads>;
    using value_type = t_value_type;
    using engine_type = t_engine_type;
    static constexpr std::size_t count_threads = t_count_threads;

    using simulator_type = ropufu::sequential::gaussian_mean_hypotheses::simulator<value_type, engine_type>;
    using aggregator_type = ropufu::sequential::gaussian_mean_hypotheses::aggregator<value_type>;

    using config_type = ropufu::sequential::gaussian_mean_hypotheses::config<value_type>;
    using statistic_type = typename simulator_type::statistic_type;
    using model_type = typename statistic_type::model_type;

    using monte_carlo_type = ropufu::aftermath::random::monte_carlo<simulator_type, aggregator_type, count_threads>;

    static bool try_read_json(const std::filesystem::path& path, nlohmann::json& j) noexcept
    {
        try
        {
            std::ifstream filestream{path}; // Try to open the file for reading.
            if (filestream.fail()) return false; // Stop on failure.
            filestream >> j;
            return true;
        } // try
        catch (const std::ios_base::failure& /*e*/)
        {
            return false;
        } // catch(...)
    } // try_read_json(...)

    static void run(std::size_t count_simulations, const statistic_type& xsprt) noexcept
    {
        std::chrono::steady_clock::time_point start{};
        std::chrono::steady_clock::time_point end{};
        
        std::array<simulator_type, count_threads> simulators{};
        for (std::size_t i = 0; i < count_threads; ++i)
            simulators[i] = simulator_type(xsprt);

        int time_seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
        std::seed_seq main_sequence{ 1, 1, 2, 3, 5, 8, 1729, time_seed };
        engine_type seed_engine{main_sequence};
        for (std::size_t i = 0; i < count_threads; ++i)
        {
            std::seed_seq threaded_sequence{1, 7, 2, 9,
                static_cast<int>(seed_engine()),
                static_cast<int>(seed_engine())};
            simulators[i].seed(threaded_sequence);
        } // for (...)

        // ========================= Begin simulation ===============================
        start = std::chrono::steady_clock::now();
        ::separator();
        std::cout << "Simulations: " << count_simulations << std::endl;
        std::cout << "Simulated signal strength: " << xsprt.simulated_signal_strength() << std::endl;
        std::cout << "Change of measure signal strength: " << xsprt.change_of_measure_signal_strength() << std::endl;
        ::separator();

        monte_carlo_type mc{simulators};
        aggregator_type output = mc.execute_sync(count_simulations);
        
        std::cout << "ASPRT sample size:" << std::endl;
        ::cat(output.sample_size().adaptive_sprt);
        ::separator();
        std::cout << "GSPRT sample size:" << std::endl;
        ::cat(output.sample_size().generalized_sprt);
        ::separator();
        
        std::cout << "ASPRT direct error (log base 10):" << std::endl;
        ::cat(output.direct_error_indicator().adaptive_sprt, [] (auto x) { return -std::log10(x); });
        ::separator();
        std::cout << "GSPRT direct error (log base 10):" << std::endl;
        ::cat(output.direct_error_indicator().generalized_sprt, [] (auto x) { return -std::log10(x); });
        ::separator();
        
        std::cout << "ASPRT importance error (log base 10):" << std::endl;
        ::cat(output.importance_error_indicator().adaptive_sprt, [] (auto x) { return -std::log10(x); });
        ::separator();
        std::cout << "GSPRT importance error (log base 10):" << std::endl;
        ::cat(output.importance_error_indicator().generalized_sprt, [] (auto x) { return -std::log10(x); });
        ::separator();

        end = std::chrono::steady_clock::now();
        // ========================= End simulation =================================

        double elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / static_cast<double>(1'000);
        std::cout << "Total elapsed time: " << elapsed_seconds << " seconds." << std::endl;
        ::separator();
    } // run(...)

    static ::execution_result execute(const std::filesystem::path& config_path) noexcept
    {
        nlohmann::json j{};
        if (!type::try_read_json(config_path, j))
        {
            std::cout << "Failed to read config file." << std::endl;
            return ::execution_result::failed_to_read_config_file;
        } // if (...)

        config_type config{};
        if (!ropufu::noexcept_json::try_get(j, config))
        {
            std::cout << "Failed to parse config file." << std::endl;
            return ::execution_result::failed_to_parse_config_file;
        } // if (...)

        // First simulation: observations from \Pr_0, change of measure to \Pr_1.
        statistic_type xsprt_null{config.model, config.asprt_thresholds, config.gsprt_thresholds,
            0, config.model.weakest_signal_strength(), config.anticipated_sample_size.first};
        type::run(config.count_simulations, xsprt_null);
        
        // First simulation: observations from \Pr_1, change of measure to \Pr_0.
        statistic_type xsprt_alternative{config.model, config.asprt_thresholds, config.gsprt_thresholds,
            config.model.weakest_signal_strength(), 0, config.anticipated_sample_size.second};
        type::run(config.count_simulations, xsprt_alternative);

        return ::execution_result::all_good;
    } // execute(...)
}; // struct program

int main()
{
    using value_type = double;
    using engine_type = std::mt19937_64;
    constexpr std::size_t count_threads = 4;

    ::execution_result result = ::program<value_type, engine_type, count_threads>::execute("./config.json");
    return static_cast<int>(result);
} // main(...)
