
#include <aftermath/algebra.hpp>
#include <aftermath/format.hpp>
#include <aftermath/not_an_error.hpp>
#include <aftermath/probability.hpp>
#include <nlohmann/json.hpp>

#include "config.hpp"
#include "filesystem.hpp"
#include "model.hpp"
#include "process.hpp"
#include "signals.hpp"
#include "observer_bunch.hpp"

#include "moment_statistic.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

using config_type = ropufu::sequential::hypotheses::config;
using constant_signal_type = ropufu::sequential::hypotheses::constant_signal;
using filesystem_type = ropufu::sequential::hypotheses::filesystem;

using matstream_type = ropufu::aftermath::format::matstream<4>;
using matrix_type = ropufu::aftermath::algebra::matrix<double>;
using not_an_error_type = ropufu::aftermath::not_an_error;
using quiet_error_type = ropufu::aftermath::quiet_error;

template <typename t_stopping_time_type>
using observer_t = ropufu::sequential::hypotheses::stopping_time_observer<t_stopping_time_type>;
template <typename t_signal_type>
using model_t = ropufu::sequential::hypotheses::model<t_signal_type>;
template <typename t_signal_type>
using process_t = ropufu::sequential::hypotheses::process<t_signal_type>;
template <typename t_signal_type>
using bunch_t = ropufu::sequential::hypotheses::observer_bunch<t_signal_type>;

template <typename t_function_type>
void benchmark(t_function_type&& action) noexcept
{
    if (!quiet_error_type::instance().good()) return;
    using clock_type = std::chrono::steady_clock;

    auto tic = clock_type::now();
    action();
    auto toc = clock_type::now();
    double elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic).count() / 1'000.0;

    std::cout << "Elapsed time " << elapsed_seconds << " s." << std::endl << std::flush;
}

template <typename t_signal_type>
void run_length(process_t<t_signal_type>& process, bunch_t<t_signal_type>& bunch, double analyzed_mu, std::size_t m) noexcept
{
    if (!quiet_error_type::instance().good()) return;
    std::ostringstream caption_stream;

    // Brief caption.
    caption_stream <<
        "Monte Carlo [" << m << "]" << " under " <<
        (process.model().is_null(analyzed_mu) ? "null" : "alt") <<
        " [" << analyzed_mu << "]";
    if (process.actual_mu() != analyzed_mu) caption_stream << " w/ change of measure: " << process.actual_mu() << " -> " << analyzed_mu;

    bunch.clear_log();
    bunch.log_caption(caption_stream.str());
    std::cout << caption_stream.str() << std::endl;

    // ~~ Clean up ~~
    process.reset(); // Make sure the process starts from scratch.
    bunch.clear(); // Make sure the observers don't have any lingering data.

    if (!quiet_error_type::instance().good()) return;
    for (std::size_t i = 0; i < m; i++)
    {
        while (bunch.is_running())
        {
            process.tic();
            bunch.tic(process);
            if (!quiet_error_type::instance().good()) return;
        }
        bunch.toc(process);
        process.reset();
        if (!quiet_error_type::instance().good()) return;
    }
    std::cout << bunch;

    // ~~ Clean up ~~
    bunch.log();
    bunch.mat(); // Dump results to .mat files.
    bunch.clear(); // Make sure the observed data does not escape the scope of this function.
}

template <typename t_signal_type>
void execute_model(const model_t<t_signal_type>& model) noexcept
{
    if (!quiet_error_type::instance().good()) return;
    std::cout << model << std::endl;

    //quiet_error_type& quiet_error = quiet_error_type::instance();
    const config_type& config = config_type::instance();

    // ~~ Monte Carlo ~~
    std::size_t monte_carlo_count = config["monte carlo"];
    // ~~
    std::vector<double> additional_analyzed_mu = config["more analyzed mu's"]; // Additional analyzed mu scenarios.
    std::vector<double> additional_simulated_mu = config["more simulated mu's"]; // Additional simulated mu scenarios.
    // ~~
    double expected_run_length = config["expected run length"];
    std::string log_filename = filesystem_type::format_path(config["log file"]);
    std::string mat_folder = filesystem_type::format_path(config["mat output"]);

    bunch_t<t_signal_type> bunch(model);
    bunch.set_output_to(log_filename, mat_folder);
    // ~~ Contains information relevant to construction of stopping times and decision makers ~~
    for (const nlohmann::json& desc : config["procedures"])
    {
        if (!bunch.try_parse(desc)) std::cout << "Failed to parse " << desc << std::endl;
    }

    double null_mu = model.mu_under_null();
    double alt_mu = model.mu_under_alt();
    bunch.fix_mat_prefix(); // Fixes the output file, so that results of several simulations are stored in the same place.
    for (double analyzed_mu : {null_mu, alt_mu})
    {
        for (double simulated_mu : {null_mu, alt_mu})
        {
            bool do_change_of_measure = analyzed_mu != simulated_mu;
            bool is_null = analyzed_mu == null_mu;
            bunch.set_var_names(
                is_null ? "false_alarm" : "missed_signal", 
                is_null ? "sample_size_h0" : "sample_size_h1", 
                do_change_of_measure ? "chg" : "noch");

            process_t<t_signal_type> process(model, simulated_mu);
            bunch.look_for(analyzed_mu, expected_run_length);

            benchmark([&](){
                run_length(process, bunch, analyzed_mu, monte_carlo_count);
                std::cout << std::endl;
            });
        }
    }
    
    // @todo Add additional simulated/analyzed mu's.
}

std::int32_t main(std::int32_t argc, char* argv[], char* envp[]) noexcept
{
    quiet_error_type& quiet_error = quiet_error_type::instance();
    const config_type& config = config_type::instance();
    //std::cout << config << std::endl;
    // ~~ Noise ~~
    std::vector<double> ar_parameters = config["AR parameters"];
    double noise_sigma = config["noise sigma"];
    // ~~ Hypotheses ~~
    std::vector<double> null_mus = config["null mu's"];
    std::vector<double> alt_mus = config["alt mu's"];
    if (null_mus.size() != alt_mus.size())
    {
        std::cout << "Null mu's and alt mu's should have the same number of elements." << std::endl;
        return 0;
    }
    std::size_t count_mus = null_mus.size();

    // ~~ Signal ~~
    std::string signal_type_name = config["signal"]["type"];
    if (signal_type_name == "const")
    {
        double signal_level = config["signal"]["level"];
        constant_signal_type signal(signal_level);

        for (std::size_t k = 0; k < count_mus; k++)
        {
            model_t<decltype(signal)> model(signal, null_mus[k], alt_mus[k], noise_sigma, ar_parameters);
            execute_model(model);
        }
    }
    else
    {
        std::cout << "Signal type not recognized: " << signal_type_name << std::endl;
        return 0;
    }

    if (!quiet_error.good()) std::cout << "~~ Oh no! Errors encoutered: ~~" << std::endl;
    else if (!quiet_error.empty()) std::cout << "~~ Something to keep in mind: ~~" << std::endl;
    while (!quiet_error.empty())
    {
        ropufu::aftermath::quiet_error_descriptor err = quiet_error.pop();
        std::cout << '\t' <<
            " level " << static_cast<std::size_t>(err.severity()) <<
            " error # " << static_cast<std::size_t>(err.error_code()) <<
            " on line " << err.caller_line_number() <<
            " of <" << err.caller_function_name() << ">:\t" << err.description() << std::endl;
    }
    
    return 0;
}
