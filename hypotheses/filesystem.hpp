
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_FILESYSTEM_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_FILESYSTEM_HPP_INCLUDED

#include <aftermath/not_an_error.hpp>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            struct filesystem
            {
                /** Retrieves the environment variable named \p key. */
                static std::string get_environment_variable(const std::string& key) noexcept
                {
                    char* val = std::getenv(key.c_str());
                    return val == nullptr ? "" : std::string(val);
                } 
                
                /** Trims the provided path and replaces the leading ~ with $HOME environment variable. */
                static std::string format_path(const std::string& path) noexcept
                {
                    std::string result = path;
                    // First, trim the path.
                    std::size_t first_index = path.find_first_not_of(' ');
                    if (first_index != std::string::npos)
                    {
                        size_t last_index = path.find_last_not_of(' ');
                        result = path.substr(first_index, last_index - first_index + 1);
                    }

                    // Return empty string if the path is empty.
                    if (result.empty()) return result;
                    // Return the trimmed path if it does not start with '~'.
                    if (result.front() != '~') return result;

                    std::ostringstream path_stream;
                    path_stream << filesystem::get_environment_variable("HOME"); // Replace '~' with $HOME.
                    path_stream << path.substr(1); // Append the remaining path.
                    return path_stream.str();
                }

                /** Combines two paths. */
                static std::string path_combine(const std::string& folder_path, const std::string& other_path) noexcept
                {
                    if (folder_path.empty()) return other_path;

                    std::ostringstream path_stream;
                    path_stream << folder_path;
                    if (folder_path.back() != '/' && folder_path.back() != '\\') path_stream << '/';
                    path_stream << other_path;
                    return path_stream.str();
                }

                /** Checks if the specified file exists and is accessible for reading. */
                static bool does_file_exist(const std::string& file_path) noexcept
                {
                    if (file_path.empty()) return false;

                    std::ifstream i(filesystem::format_path(file_path));
                    return i.good();
                }

                /** @brief Checks if the specified file is accessible for writing.
                 *  @exception not_an_error::not_an_error This warning is pushed to \c quiet_error if the file is not accessible.
                 */
                static bool can_write_surely(const std::string& file_path, const std::string& caller_function_name, std::size_t line_number) noexcept
                {
                    std::ofstream o(filesystem::format_path(file_path), std::ios_base::app); // Try to open the file for writing.
                    if (!o.good())
                    {
                        aftermath::quiet_error::instance().push(aftermath::not_an_error::runtime_error, "Could not write to file.", caller_function_name, line_number);
                        return false; // Stop on failure.
                    }
                    return true;
                }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_FILESYSTEM_HPP_INCLUDED
