
#ifndef ROPUFU_SETTLERS_ONLINE_JSON_HPP_INCLUDED
#define ROPUFU_SETTLERS_ONLINE_JSON_HPP_INCLUDED

#include <nlohmann/json.hpp>

#include <string> // std::string

namespace ropufu
{
    /** Helper structure for parsing \c nlohmann::json. */
    struct quiet_json
    {
        using type = quiet_json;

    private:
        bool m_is_good = true; // Indicates if no errors have been encountered.
        std::string m_message = ""; // Error/warning message (if any).
        nlohmann::json m_container = { }; // Local copy of the JSON entry.

        /** @brief Checks if a required element is missing in json.
         *  @return True if the element is missing.
         *  @exception not_an_error::runtime_error This error is pushed to \c quiet_error if the element is required but missing.
         */
        bool is_missing(const std::string& key, bool is_optional) noexcept
        {
            if (this->m_container.count(key) > 0) return false;
            if (!is_optional) // Indicate erroneous state if key was required.
            {
                this->m_message = std::string("Missing required value for ") + key + std::string(".");
                this->m_is_good = false;
            } // if (...)
            return true;
        } // is_missing(...)

    public:
        explicit quiet_json(const nlohmann::json& container) noexcept
            : m_container(container)
        {
        } // quiet_json(...)

        bool good() const noexcept { return this->m_is_good; }

        const std::string& message() const noexcept { return this->m_message; }

        //bool has(const std::string& key) const noexcept { return this->m_container.count(key) > 0; }

        /** @brief Tries to interprets json object as \p value. */
        template <typename t_value_type>
        bool interpret_as(t_value_type& value) noexcept
        {
            if (!this->m_is_good) return false; // Don't do anything if errors have already beed encountered.
            try
            {
                t_value_type maybe = this->m_container; // Let Niels Lohmann handle everything.
                value = maybe; // Parsing has succeeded if we've reached this far.
                return true;
            } // try
            catch (...)
            {
                this->m_message = std::string("JSON interpretation failed.");
                this->m_is_good = false;
                return false;
            } // catch (...)
        } // interpret_as(...)

        /** @brief Tries to read a required or optional record from json.
         *  @return True if the record was acceptable.
         *  @remark \p value is overwritten only if the record was found and was properly formed.
         */
        template <typename t_value_type>
        bool required(const std::string& key, t_value_type& value, bool is_optional = false) noexcept
        {
            if (!this->m_is_good) return false; // Don't do anything if errors have already beed encountered.
            if (this->is_missing(key, is_optional)) return is_optional; // Allow missing keys for optional values.

            try
            {
                t_value_type maybe = this->m_container.at(key); // Let Niels Lohmann handle everything.
                value = maybe; // Parsing has succeeded if we've reached this far.
                return true;
            } // try
            catch (...)
            {
                this->m_message = std::string("JSON representation for '") + key + std::string("' malformed.");
                this->m_is_good = false;
                return false;
            } // catch (...)
        } // required(...)
        
        /** @brief Tries to read an optional record from json.
         *  @return True if the record was acceptable.
         *  @remark \p value is overwritten only if the record was found and was properly formed.
         */
        template <typename t_value_type>
        bool optional(const std::string& key, t_value_type& value) noexcept
        {
            if (!this->m_is_good) return false; // Don't do anything if errors have already beed encountered.
            return this->required(key, value, true);
        } // optional(...)

        // ~~ Delete copy and move constructors and assign operators ~~
        quiet_json(const type&) = delete; // Copy constructor.
        quiet_json(type&&)      = delete; // Move constructor.
        type& operator =(const type&) = delete; // Copy assign.
        type& operator =(type&&)      = delete; // Move assign.
    }; // struct quiet_json
} // namespace ropufu

#endif // ROPUFU_SETTLERS_ONLINE_JSON_HPP_INCLUDED
