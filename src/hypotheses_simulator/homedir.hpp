
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_HOMEDIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_HOMEDIR_HPP_INCLUDED

#include <cstddef> // std::size_t
#include <cstdlib> // std::getenv
#include <string>  // std::string

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                /** Retrieves the environment variable named \p key. */
                static std::string get_environment_variable(const std::string& key) noexcept
                {
                    char* value = std::getenv(key.c_str());
                    return value == nullptr ? "" : std::string(value);
                } 
                
                /** Trims the provided path and replaces the leading ~ with $HOME environment variable. */
                static std::string format_homedir_path(const std::string& path) noexcept
                {
                    std::string result = path;
                    // First, trim the path.
                    std::size_t first_index = path.find_first_not_of(' ');
                    if (first_index != std::string::npos)
                    {
                        std::size_t last_index = path.find_last_not_of(' ');
                        result = path.substr(first_index, last_index - first_index + 1);
                    } // if (...)

                    // Return empty string if the path is empty.
                    if (result.empty()) return result;
                    // Return the trimmed path if it does not start with '~'.
                    if (result.front() != '~') return result;

                    std::string home_path = detail::get_environment_variable("HOME");
                    if (home_path == "") home_path = detail::get_environment_variable("USERPROFILE");
                    if (home_path == "") home_path = "~";

                    return (home_path + path.substr(1));
                } // format_homedir_path(...)
            } // namespace detail
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_HOMEDIR_HPP_INCLUDED
