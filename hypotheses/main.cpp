
#include <aftermath/probability/empirical_measure.hpp>
#include <aftermath/algebra/matrix.hpp>
#include <aftermath/format/matstream.hpp>
#include <aftermath/format/matstream_v4.hpp>
#include <nlohmann/json.hpp>

#include "config.hpp"
#include "model.hpp"
#include "not_an_error.hpp"
#include "process.hpp"
#include "signals.hpp"
#include "observer_bunch.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

typedef ropufu::sequential::hypotheses::config config_type;
typedef ropufu::sequential::hypotheses::constant_signal constant_signal_type;
typedef ropufu::sequential::hypotheses::not_an_error not_an_error_type;
typedef ropufu::aftermath::probability::empirical_measure<double, std::size_t, double> empirical_measure_type;
typedef ropufu::aftermath::format::matstream<4> matstream_type;
typedef ropufu::aftermath::algebra::matrix<double> matrix_type;

template <typename t_return_type>
using quiet_return_t = ropufu::sequential::hypotheses::quiet_return<t_return_type>;
template <typename t_stopping_time_type>
using observer_t = ropufu::sequential::hypotheses::stopping_time_observer<t_stopping_time_type>;
template <typename t_signal_type>
using model_t = ropufu::sequential::hypotheses::model<t_signal_type>;
template <typename t_signal_type>
using process_t = ropufu::sequential::hypotheses::process<t_signal_type>;
template <typename t_signal_type>
using bunch_t = ropufu::sequential::hypotheses::observer_bunch<t_signal_type>;

template <typename t_function_type>
void benchmark(t_function_type&& action)
{
    typedef std::chrono::high_resolution_clock clock_type;
    auto tic = clock_type::now();
    action();
    auto toc = clock_type::now();
    auto dt = toc.time_since_epoch() - tic.time_since_epoch();
    auto seconds = static_cast<double>(dt.count() * clock_type::period::num) / clock_type::period::den;

    std::cout << "Elapsed time " << seconds << " s." << std::endl << std::flush;
}

template <typename t_return_type>
bool check_for_error(const quiet_return_t<t_return_type>& x)
{
    if (x.error() == not_an_error_type::all_good) return false;
    std::cout << "Quiet error: " << static_cast<std::size_t>(x.error()) << std::endl;
    return true;
}

template <typename t_signal_type>
void run_length(process_t<t_signal_type>& process, bunch_t<t_signal_type>& bunch, std::size_t m)
{
    std::ostringstream caption_stream;

    // Brief caption.
    caption_stream <<
        "Monte Carlo [" << m << "]" << " under " <<
        (process.model().is_null(bunch.desired_mu()) ? "null" : "alt") <<
        " [" << bunch.desired_mu() << "]";
    if (process.actual_mu() != bunch.desired_mu()) caption_stream << " w/ change of measure: " << process.actual_mu() << " -> " << bunch.desired_mu();

    bunch.log_caption(caption_stream.str());
    std::cout << caption_stream.str() << std::endl;

    // ~~ Clean up ~~
    process.reset(); // Make sure the process starts from scratch.
    bunch.clear(); // Make sure the observers don't have any lingering data.

    //empirical_measure_type adjusted_process = { };
    for (std::size_t i = 0; i < m; i++)
    {
        while (bunch.is_running())
        {
            process.tic();
            //adjusted_process.observe(process.adjusted_process(process.count() - 1));
            if (check_for_error(bunch.tic(process))) return;
        }
        if (check_for_error(bunch.toc(process))) return;
        process.reset();
    }
    //std::cout << "adjusted process: " << adjusted_process.mean() << " pm " << adjusted_process.compute_standard_deviation() << std::endl;
    std::cout << bunch;

    // ~~ Clean up ~~
    bunch.log();
    bunch.clear(); // Make sure the observed data does not escape the scope of this function.
}

double a_element_at(std::size_t i, std::size_t j) { return 10 * (i + 1) + (j + 1); }
double b_element_at(std::size_t i, std::size_t j) { return 10 * (i + 1) - (j + 1); }

void test_write_mat()
{
    matrix_type a(5, 3);
    matrix_type b(2, 4);

    for (std::size_t i = 0; i < a.height(); i++) for (std::size_t j = 0; j < a.width(); j++) a(i, j) = a_element_at(i, j);
    for (std::size_t i = 0; i < b.height(); i++) for (std::size_t j = 0; j < b.width(); j++) b(i, j) = b_element_at(i, j);

    matstream_type mat("./hypotheses.mat");
    mat.clear();
    mat << "baka" << a;
    mat << "greg" << b;
}

bool test_read_mat()
{
    matrix_type a;
    matrix_type b;
    std::string name1;
    std::string name2;
    matstream_type mat("./hypotheses.mat");
    mat.load(name1, a);
    mat.load(name2, b);
    std::cout << name1 << " is a " << a.height() << " by " << a.width() << " matrix." << std::endl;
    std::cout << name2 << " is a " << b.height() << " by " << b.width() << " matrix." << std::endl;
    
    for (std::size_t i = 0; i < a.height(); i++) for (std::size_t j = 0; j < a.width(); j++) if (a(i, j) != a_element_at(i, j)) return false;
    for (std::size_t i = 0; i < b.height(); i++) for (std::size_t j = 0; j < b.width(); j++) if (b(i, j) != b_element_at(i, j)) return false;
    return true;
}

std::int32_t main(std::int32_t argc, char* argv[], char* envp[])
{
    if (argc > 1) std::cout << argv[1] << std::endl;
    test_write_mat();
    std::cout << "Matrix test: " << (test_read_mat() ? "passed" : "failed") << std::endl;
    return 0;

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
    //bool do_change_of_measure = analyzed_mu != simulated_mu;

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
        bunch.look_for(analyzed_mu);
        bunch.clear_log();
        // ~~ Contains information relevant to construction of stopping times and decision makers ~~
        for (const nlohmann::json& desc : config["procedures"])
        {
            quiet_return_t<void> q = bunch.try_parse(desc);
            if (q.error() != not_an_error_type::all_good) std::cout << "Failed to parse " << desc << std::endl;
        }

        benchmark([&](){
            run_length(process, bunch, monte_carlo_count);
            std::cout << std::endl;
        });
    }

    //std::cout << "Press any key to continue . . ." << std::endl;
    return 0;
}
