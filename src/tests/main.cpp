
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#define JSON_USE_IMPLICIT_CONVERSIONS 0

// Hypotheses Tests:
#include "hypotheses/signals.hpp"
#include "hypotheses/noises.hpp"
#include "hypotheses/simple_process.hpp"
#include "hypotheses/change_of_measure.hpp"
#include "hypotheses/hypothesis_pair.hpp"
#include "hypotheses/observer.hpp"
#include "hypotheses/rules.hpp"
#include "hypotheses/monte_carlo.hpp"

// Hypotheses Simulator Tests:
#include "hypotheses_simulator/init_info.hpp"
#include "hypotheses_simulator/run.hpp"

#include <exception> // std::exception
#include <iostream>  // std::cout, std::endl

int main(int argc, char** argv, char** /*envp*/)
{
    doctest::Context context {};
    try
    {
        context.applyCommandLine(argc, argv);
        int res = context.run();
        if (context.shouldExit()) return res;  
    } // try
    catch (const std::exception& ex)
    {
        std::cout << "~~ Oh no! ~~" << std::endl;
        std::cout << ex.what() << std::endl;
    } // catch (...)

    return 0;
} // main(...)
