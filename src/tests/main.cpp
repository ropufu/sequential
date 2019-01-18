
#include "signal_test.hpp"
#include "noise_test.hpp"
#include "process_test.hpp"
#include "rule_test.hpp"
#include "monte_carlo_test.hpp"
#include "moment_statistic_test.hpp"

#include <chrono>    // std::chrono::steady_clock, std::chrono::duration_cast
#include <cstddef>   // std::size_t
#include <cstdint>   // std::int32_t
#include <exception> // std::exception
#include <iostream>  // std::cout, std::endl
#include <string>    // std::string

using signal_test = ropufu::sequential::hypotheses_test::signal_test;
using noise_test = ropufu::sequential::hypotheses_test::noise_test;
using process_test = ropufu::sequential::hypotheses_test::process_test;
using rule_test = ropufu::sequential::hypotheses_test::rule_test;
using monte_carlo_test = ropufu::sequential::hypotheses_test::monte_carlo_test;
using moment_statistic_test = ropufu::sequential::hypotheses_test::moment_statistic_test;

template <typename t_test_type>
bool run_test(t_test_type test, std::string&& name)
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    bool result = test();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    double elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1'000.0;

    std::cout
        << (result ? "test passed: " : "test failed: ")
        << name << ". Elapsed time: "
        << elapsed_seconds << "s." << std::endl;

    return result;
} // run_test(...)

std::int32_t main()
{
    try
    {
        //run_test([] () { return false; });
        // ~~ Moment statistic tests ~~
        run_test(moment_statistic_test::test_scalar<std::size_t, double, 0>, "<moment_statistic_test(0)> scalar");
        run_test(moment_statistic_test::test_scalar<float, double, 1>, "<moment_statistic_test(1)> scalar");
        run_test(moment_statistic_test::test_scalar<double, double, 3>, "<moment_statistic_test(3)> scalar");
        run_test(moment_statistic_test::test_scalar<float, float, 8>, "<moment_statistic_test(8)> scalar");
        run_test(moment_statistic_test::test_matrix<float, double, 0, 4, 5>, "<moment_statistic_test(0)> matrix");
        run_test(moment_statistic_test::test_matrix<std::size_t, double, 1, 4, 5>, "<moment_statistic_test(1)> matrix");
        run_test(moment_statistic_test::test_matrix<float, float, 3, 7, 3>, "<moment_statistic_test(3)> matrix");
        run_test(moment_statistic_test::test_matrix<double, double, 8, 2, 2>, "<moment_statistic_test(8)> matrix");
        // ~~ Signal tests ~~
        run_test(signal_test::test_constant, "<signal_test> constant");
        run_test(signal_test::test_transitionary<0>, "<signal_test> transitionary(0)");
        run_test(signal_test::test_transitionary<2>, "<signal_test> transitionary(2)");
        run_test(signal_test::test_transitionary<5>, "<signal_test> transitionary(5)");
        run_test(signal_test::print, "<signal_test> printer");
        // ~~ Noise tests ~~
        run_test(noise_test::test_white, "<noise_test> white");
        run_test(noise_test::test_ar<0>, "<noise_test> AR(0)");
        run_test(noise_test::test_ar<2>, "<noise_test> AR(2)");
        run_test(noise_test::test_ar<5>, "<noise_test> AR(5)");
        run_test(noise_test::print, "<noise_test> printer");
        // ~~ Process tests ~~
        run_test(process_test::test_constant_white, "<process_test> constant / white");
        run_test(process_test::test_constant_ar<0>, "<process_test> constant / AR(0)");
        run_test(process_test::test_constant_ar<2>, "<process_test> constant / AR(2)");
        run_test(process_test::test_constant_ar<5>, "<process_test> constant / AR(5)");
        run_test(process_test::test_transit_white<0>, "<process_test> transitionary(0) / white");
        run_test(process_test::test_transit_white<2>, "<process_test> transitionary(2) / white");
        run_test(process_test::test_transit_ar<0, 0>, "<process_test> transitionary(0) / AR(0)");
        run_test(process_test::test_transit_ar<2, 2>, "<process_test> transitionary(2) / AR(2)");
        run_test(process_test::test_transit_ar<2, 5>, "<process_test> transitionary(2) / AR(5)");
        run_test(process_test::test_transit_ar<5, 2>, "<process_test> transitionary(5) / AR(2)");
        run_test(process_test::print, "<process_test> printer");
        // ~~ Rule tests ~~
        run_test(rule_test::test_asprt, "<rule_test> A-2-SPRT");
        run_test(rule_test::test_bsprt, "<rule_test> A-2-SPRT*");
        run_test(rule_test::test_gsprt, "<rule_test> GSPRT");
        run_test(rule_test::test_hsprt, "<rule_test> GSPRT*");
        run_test(rule_test::print, "<rule_test> printer");
        // ~~ MC tests ~~
        run_test(monte_carlo_test::test_all, "<monte_carlo_test> all");    
    } // try
    catch (const std::exception& ex)
    {
        std::cout << "~~ Oh no! ~~" << std::endl;
        std::cout << ex.what() << std::endl;
    } // catch (...)

    return 0;
} // main(...)
