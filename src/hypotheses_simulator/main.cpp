
#include <nlohmann/json.hpp>

#include "config.hpp"
#include "automator.hpp"

#include <chrono>   // std::chrono::steady_clock, std::chrono::duration_cast
#include <cstddef>  // std::size_t
#include <filesystem> // std::filesystem::path, std::filesystem::remove
#include <fstream>  // std::ifstream, std::ofstream
#include <iomanip>  // std::setw
#include <ios>        // std::ios_base::failure
#include <iostream> // std::cout, std::endl
#include <random>   // std::mt19937
#include <system_error> // std::error_code

bool try_read_json(const std::filesystem::path& path, nlohmann::json& j)
{
    try
    {
        std::ifstream filestream {path}; // Try to open the file for reading.
        if (filestream.fail()) return false; // Stop on failure.
        filestream >> j;
        return true;
    } // try
    catch (const std::ios_base::failure& /*e*/)
    {
        return false;
    } // catch(...)
} // try_read_json(...)

bool try_write_json(const std::filesystem::path& path, const nlohmann::json& j)
{
    try
    {
        std::ofstream filestream {path}; // Try to open the file for writing.
        if (filestream.fail()) return false; // Stop on failure.
        filestream << std::setw(4) << j << std::endl;
        return true;
    } // try
    catch (const std::ios_base::failure& /*e*/)
    {
        return false;
    } // catch(...)
} // try_write_json(...)

int main()
{
    using engine_type = std::mt19937;
    using value_type = double;
    using config_type = ropufu::sequential::hypotheses::config<engine_type, value_type>;
    using automator_type = ropufu::sequential::hypotheses::automator<engine_type, value_type>;

    const std::filesystem::path config_path = "./config.json";

    std::chrono::steady_clock::time_point start {};
    std::chrono::steady_clock::time_point end {};

    nlohmann::json config_json {};
    bool is_okay = ::try_read_json(config_path, config_json);
    if (!is_okay)
    {
        std::cout << "Failed to read config file." << std::endl;
        return 1729;
    } // if (...)

    std::error_code ec {};
    config_type config {config_json, ec};
    if (ec.value() != 0)
    {
        std::cout << "Config file could not be parsed." << std::endl;
        return 87539319;
    } // if (...)

    std::cout << "Initialization completed." << std::endl;
    std::cout << "-- " << config.rule_designs().size() << " rules." << std::endl;
    std::cout << "-- " << config.runs().size() << " runs." << std::endl;

    start = std::chrono::steady_clock::now();

    engine_type engine {};
    int time_seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
    std::seed_seq sequence { 1, 1, 2, 3, 5, 8, 1729, time_seed };
    engine.seed(sequence);

    automator_type automator {config, config_path};
    automator.execute(engine);

    end = std::chrono::steady_clock::now();

    double elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / static_cast<double>(1'000);
    std::cout << "Total elapsed time: " << elapsed_seconds << "s." << std::endl;
    
    return 0;
} // main(...)
