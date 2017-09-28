
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_MATLAB_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_MATLAB_HPP_INCLUDED

#include <aftermath/not_an_error.hpp>

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            struct matlab
            {
                template <typename t_transform_type, typename t_inverse_transform_type>
                static std::vector<double> space(double from, double to, std::size_t count, const t_transform_type& f, const t_inverse_transform_type& f_inverse) noexcept
                {
                    if (count == 0) return std::vector<double>(0);
                    if (count == 1) return { from };
                    if (count == 2) return { from, to };

                    std::vector<double> result(count);
                    result[0] = from;
                    result[count - 1] = to;

                    from = f(from);
                    to = f(to);
                    double range = to - from;
                    for (std::size_t i = 1; i < count - 1; i++)
                    {
                        double step = (i * range) / (count - 1);
                        result[i] = f_inverse(from + step);
                    }
                    result.shrink_to_fit();
                    return result;
                }

                static std::vector<double> linspace(double from, double to, std::size_t count) noexcept
                {
                    return matlab::space(from, to, count, [](double x) { return x; }, [](double x) { return x; });
                }

                static std::vector<double> logspace(double from, double to, std::size_t count) noexcept
                {
                    return matlab::space(from, to, count, [](double x) { return std::log10(x); }, [](double x) { return std::pow(10, x); });
                }

                static std::vector<double> expspace(double from, double to, std::size_t count) noexcept
                {
                    return matlab::space(from, to, count, [](double x) { return std::pow(10, x); }, [](double x) { return std::log10(x); });
                }

                static std::vector<double> parse_space(const std::string& name, double from, double to, std::size_t count) noexcept
                {
                    if (name == "lin") return matlab::linspace(from, to, count);
                    if (name == "log") return matlab::logspace(from, to, count);
                    if (name == "exp") return matlab::expspace(from, to, count);
                    
                    aftermath::quiet_error::instance().push(aftermath::not_an_error::all_good, "Threshold spacing not recognized. Defaulting to linear.", __FUNCTION__, __LINE__);
                    return matlab::linspace(from, to, count);
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_MATLAB_HPP_INCLUDED
