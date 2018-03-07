
#ifndef ROPUFU_AFTERMATH_MATLAB_HPP_INCLUDED
#define ROPUFU_AFTERMATH_MATLAB_HPP_INCLUDED

#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include <cmath>   // std::log10, std::pow
#include <cstddef> // std::size_t
#include <initializer_list> // std::initializer_list
#include <string>  // std::string
#include <vector>  // std::vector

namespace ropufu
{
    namespace aftermath
    {
        namespace detail
        {
            template <typename t_data_type, typename t_container_type>
            struct matlab_container
            {
                using type = matlab_container<t_data_type, t_container_type>;
                using data_type = t_data_type;
                using container_type = t_container_type;

                static container_type make_empty(std::size_t count = 0) noexcept;
                static container_type make_init(std::initializer_list<data_type>) noexcept;
                static void shrink(container_type& container) noexcept;
            }; // struct matlab_container

            template <typename t_data_type>
            struct matlab_container<t_data_type, std::vector<t_data_type>>
            {
                using type = matlab_container<t_data_type, std::vector<t_data_type>>;
                using data_type = t_data_type;
                using container_type = std::vector<t_data_type>;

                static container_type make_empty(std::size_t count = 0) noexcept { return container_type(count); }
                static container_type make_init(std::initializer_list<data_type> data) noexcept { return container_type(data); }
                static void shrink(container_type& container) noexcept { container.shrink_to_fit(); };
            }; // struct matlab_container
        } // namespace detail

        template <typename t_data_type, typename t_container_type = std::vector<t_data_type>>
        struct matlab
        {
            using type = matlab<t_data_type, t_container_type>;
            using data_type = t_data_type;
            using container_type = t_container_type;

        private:
            using helper_type = detail::matlab_container<t_data_type, t_container_type>;

        public:
            template <typename t_transform_type, typename t_inverse_transform_type>
            static container_type space(data_type from, data_type to, std::size_t count, const t_transform_type& f, const t_inverse_transform_type& f_inverse) noexcept
            {
                if (count == 0) return helper_type::make_empty();
                if (count == 1) return helper_type::make_init({ from });
                if (count == 2) return helper_type::make_init({ from, to });

                container_type result = helper_type::make_empty(count);

                data_type f_from = f(from);
                data_type f_to = f(to);
                data_type f_range = f_to - f_from;

                std::size_t i = 0;
                bool is_first = true;
                bool is_last = false;
                for (data_type& x : result)
                {
                    is_last = (i == count - 1);
                    if (is_first) x = from;
                    else if (is_last) x = to;
                    else
                    {
                        data_type f_step = (i * f_range) / (count - 1);
                        x = f_inverse(f_from + f_step);
                    }
                    ++i;
                    is_first = false;
                }
                helper_type::shrink(result);
                return result;
            } // space(...)

            static container_type linspace(data_type from, data_type to, std::size_t count) noexcept
            {
                return type::space(from, to, count, [] (data_type x) { return x; }, [] (data_type x) { return x; });
            } // linspace(...)

            static container_type logspace(data_type from, data_type to, std::size_t count) noexcept
            {
                return type::space(from, to, count, [] (data_type x) { return std::log10(x); }, [] (data_type x) { return std::pow(10, x); });
            } // logspace(...)

            static container_type expspace(data_type from, data_type to, std::size_t count) noexcept
            {
                return matlab::space(from, to, count, [] (data_type x) { return std::pow(10, x); }, [] (data_type x) { return std::log10(x); });
            } // expspace(...)

            static container_type parse_space(const std::string& name, data_type from, data_type to, std::size_t count) noexcept
            {
                if (name == "lin") return matlab::linspace(from, to, count);
                if (name == "log") return matlab::logspace(from, to, count);
                if (name == "exp") return matlab::expspace(from, to, count);
                
                aftermath::quiet_error::instance().push(
                    aftermath::not_an_error::all_good,
                    aftermath::severity_level::not_at_all,
                    "Threshold spacing not recognized. Defaulting to linear.", __FUNCTION__, __LINE__);
                return matlab::linspace(from, to, count);
            } // parse_space(...)
        }; // struct matlab
    } // namespace aftermath
} // namespace ropufu

#endif // ROPUFU_AFTERMATH_MATLAB_HPP_INCLUDED
