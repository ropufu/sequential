
#include <aftermath/not_an_error.hpp>

#include "config.hpp"
#include "automator.hpp"

#include <cstddef>  // std::size_t
#include <cstdint>  // std::int32_t
#include <iostream> // std::cout, std::endl
#include <string>   // std::string, std::to_string
#include <variant>  // std::visit

using config_type = ropufu::sequential::hypotheses::config<double>;
using error_manager_type = ropufu::aftermath::quiet_error;

struct sprt_visitor
{
    template <typename t_signal_type, typename t_noise_type>
    void operator ()(const t_signal_type& signal, const t_noise_type& noise) noexcept
    {
        std::cout << "Signal / Noise:" << std::endl;
        std::cout << '\t' << signal << std::endl;
        std::cout << '\t' << noise << std::endl;

        ropufu::sequential::hypotheses::automator<t_signal_type, t_noise_type, true> aut(signal, noise);
        aut.execute();
    }
}; // struct sprt_visitor

std::int32_t main()
{
    error_manager_type& err = error_manager_type::instance();
    const config_type& config = config_type::instance();

    if (!config.good()) std::cout << "Failed to read config file." << std::endl;
    else
    {
        std::cout << "Initialization completed." << std::endl;
        std::cout << "Rules:" << std::endl;
        for (const auto& x : config.rules()) std::cout << '\t' << x << std::endl;
        // std::cout << "Runs:" << std::endl;
        // for (const auto& x : config.runs()) std::cout << '\t' << x << std::endl;

        // std::cout << std::endl;
        std::visit(sprt_visitor{}, config.signal(), config.noise());
    }

    if (!err.good()) std::cout << "~~ Oh no! Errors encoutered: ~~" << std::endl;
    else if (!err.empty()) std::cout << "~~ Something to keep in mind: ~~" << std::endl;
    else std::cout << "~~ Simulation completed successfully. ~~" << std::endl;
    while (!err.empty())
    {
        ropufu::aftermath::quiet_error_descriptor desc = err.pop();
        std::cout << '\t' <<
            "" << std::to_string(desc.severity()) <<
            " \"" << std::to_string(desc.error_code()) << "\"" <<
            " on line " << desc.caller_line_number() <<
            " of <" << desc.caller_function_name() << ">:\t" << desc.description() << std::endl;
    }

    return 0;
} // main(...)
