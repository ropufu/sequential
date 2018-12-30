
#include "config.hpp"
#include "automator.hpp"

#include <cstddef>  // std::size_t
#include <cstdint>  // std::int32_t
#include <iostream> // std::cout, std::endl
#include <string>   // std::string, std::to_string
#include <system_error> // std::error_code
#include <variant>  // std::visit

using config_type = ropufu::sequential::hypotheses::config<double>;
template <typename t_signal_type, typename t_noise_type>
using automator_t = ropufu::sequential::hypotheses::automator<t_signal_type, t_noise_type, true>;

static std::error_code s_error {};
static config_type s_config { "./simulator.config" };

struct sprt_visitor
{
    template <typename t_signal_type, typename t_noise_type>
    void operator ()(const t_signal_type& signal, const t_noise_type& noise) noexcept
    {
        std::cout << "Signal / Noise:" << std::endl;
        std::cout << '\t' << signal << std::endl;
        std::cout << '\t' << noise << std::endl;

        automator_t<t_signal_type, t_noise_type> aut(signal, noise, ::s_config, ::s_error);
        aut.execute(::s_config);
    } // operator ()(...)
}; // struct sprt_visitor

std::int32_t main()
{
    if (!::s_config.good())
    {
        std::cout << "Failed to read config file." << std::endl;
        for (const std::string& message : ::s_config.log()) std::cout << '\t' << "-- " << message << std::endl;
    } // if (...)
    else
    {
        std::cout << "Initialization completed." << std::endl;
        std::cout << "Rules:" << std::endl;
        for (const auto& x : ::s_config.rules()) std::cout << '\t' << x << std::endl;
        // std::cout << "Runs:" << std::endl;
        // for (const auto& x : config.runs()) std::cout << '\t' << x << std::endl;

        // std::cout << std::endl;
        std::visit(sprt_visitor{}, ::s_config.signal(), ::s_config.noise());
    } // else (...)

    if (::s_error.value() != 0) std::cout << "~~ Errors encountered. ~~" << std::endl;
    else std::cout << "~~ Simulation completed successfully. ~~" << std::endl;
    
    return 0;
} // main(...)
