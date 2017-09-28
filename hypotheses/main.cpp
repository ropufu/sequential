
#include <aftermath/algebra.hpp>
#include <aftermath/format.hpp>
#include <aftermath/not_an_error.hpp>
#include <aftermath/probability.hpp>
#include <nlohmann/json.hpp>

#include "config.hpp"
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

typedef ropufu::sequential::hypotheses::config config_type;
typedef ropufu::sequential::hypotheses::constant_signal constant_signal_type;

typedef ropufu::aftermath::probability::empirical_measure<double, std::size_t, double> empirical_measure_type;
typedef ropufu::aftermath::format::matstream<4> matstream_type;
typedef ropufu::aftermath::algebra::matrix<double> matrix_type;
typedef ropufu::aftermath::not_an_error not_an_error_type;
typedef ropufu::aftermath::quiet_error quiet_error_type;

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
    typedef std::chrono::high_resolution_clock clock_type;
    auto tic = clock_type::now();
    action();
    auto toc = clock_type::now();
    auto dt = toc.time_since_epoch() - tic.time_since_epoch();
    auto seconds = static_cast<double>(dt.count() * clock_type::period::num) / clock_type::period::den;

    std::cout << "Elapsed time " << seconds << " s." << std::endl << std::flush;
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
    bunch.clear(); // Make sure the observed data does not escape the scope of this function.
    bunch.mat(); // Dump results to .mat files.
}

std::int32_t main(std::int32_t argc, char* argv[], char* envp[]) noexcept
{
    //if (argc > 1) std::cout << argv[1] << std::endl;

    quiet_error_type& quiet_error = quiet_error_type::instance();
    const config_type& config = config_type::instance();
    //std::cout << config << std::endl;
    // ~~ Noise ~~
    std::vector<double> ar_parameters = config["AR parameters"];
    double noise_sigma = config["noise sigma"];
    // ~~ Hypotheses ~~
    double null_mu = config["null mu"];
    double alt_mu = config["alt mu"];
    // ~~ Monte Carlo ~~
    std::size_t monte_carlo_count = config["monte carlo"];
    double analyzed_mu = config["analyzed mu"];
    double simulated_mu = config["simulated mu"];
    double expected_run_length = config["expected run length"];
    //bool do_change_of_measure = analyzed_mu != simulated_mu;
    std::string log_filename = config["log file"];
    std::string mat_folder = config["mat output"];

    // ~~ Signal ~~
    std::string signal_type = config["signal"]["type"];
    if (signal_type == "const")
    {
        double signal_level = config["signal"]["level"];
        constant_signal_type signal(signal_level);
        model_t<decltype(signal)> model(signal, null_mu, alt_mu, noise_sigma, ar_parameters);
        std::cout << model << std::endl;

        process_t<decltype(signal)> process(model, simulated_mu);
        bunch_t<decltype(signal)> bunch(model);
        bunch.set_output_to(log_filename, mat_folder);
        // ~~ Contains information relevant to construction of stopping times and decision makers ~~
        for (const nlohmann::json& desc : config["procedures"])
        {
            if (!bunch.try_parse(desc)) std::cout << "Failed to parse " << desc << std::endl;
        }
        bunch.look_for(analyzed_mu, expected_run_length);

        benchmark([&](){
            run_length(process, bunch, analyzed_mu, monte_carlo_count);
            std::cout << std::endl;
        });
    }

    if (quiet_error.good()) std::cout << "~~ Oh no! Errors encoutered: ~~" << std::endl;
    else if (quiet_error.empty()) std::cout << "~~ Something to keep in mind: ~~" << std::endl;
    while (!quiet_error.empty())
    {
        std::string message = "";
        std::string function_name = "";
        std::size_t line = 0;
        not_an_error_type err = quiet_error.pop(message, function_name, line);
        std::cout << '\t' << static_cast<std::size_t>(err) << " on line " << line << " of <" << function_name << ">:\t" << message << std::endl;
    }
    //std::cout << "Press any key to continue . . ." << std::endl;
    return 0;
}
