
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_HPP_INCLUDED

#include <nlohmann/json.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief Class for reading and writing configurtation setting.
                *  @remark Singleton structure taken from https://stackoverflow.com/questions/11711920
                */
            struct config
            {
                typedef config type;

            private:
                bool m_has_changed = false;
                std::string m_filename = "";
                nlohmann::json m_json = { };

            protected:
                // Always try to read the default configuration on construction.
                config() noexcept { this->read(); }
                // Always try to save the configuration on exit.
                ~config() noexcept { this->write(); }

            public:
                /** Output configuration parameters. */
                friend std::ostream& operator <<(std::ostream& os, const type& self)
                {
                    os << self.m_json;
                    return os;
                }

                /** Read the configuration from a file. */
                bool read(std::string filename = "./hypotheses.config") noexcept
                {
                    std::ifstream i(filename); // Try to open the file for reading.
                    if (!i.good()) return false; // Stop on failure.

                    this->m_filename = filename; // Remember the filename.
                    i >> this->m_json;
                    return true;
                }

                /** Write the configuration to a file. */
                bool write() noexcept
                {
                    if (!this->m_has_changed) return true;
                    std::ofstream o(this->m_filename); // Try to open the file for writing.
                    if (!o.good()) return false; // Stop on failure.

                    o << std::setw(4) << this->m_json << std::endl;
                    this->m_has_changed = false;
                    return true;
                }

                /** Expose all the settings. */
                const nlohmann::json& json() const noexcept { return this->m_json;  }

                /** Expose specific settings. */
                const nlohmann::json& operator [](const std::string& parameter_name) const noexcept { return this->m_json[parameter_name];  }

                /** The only instance of this type. */
                static type& instance()
                {
                    // Since it's a static variable, if the class has already been created, it won't be created again.
                    // Note: it is thread-safe in C++11.
                    static type s_instance;
    
                    // Return a reference to our instance.
                    return s_instance;
                }
    
                // ~~ Delete copy and move constructors and assign operators ~~
                config(type const&) = delete; // Copy constructor.
                config(type&&)      = delete; // Move constructor.
                type& operator =(type const&) = delete; // Copy assign.
                type& operator =(type&&)      = delete; // Move assign.
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_CONFIG_HPP_INCLUDED
