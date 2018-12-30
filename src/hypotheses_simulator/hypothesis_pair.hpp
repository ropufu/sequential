
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct hypothesis_pair;
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const hypothesis_pair<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, hypothesis_pair<t_value_type>& x);

    /** @brief Describes a pair associated with a null-alternative hypotheses pair. */
    template <typename t_value_type>
    struct hypothesis_pair
    {
        using type = hypothesis_pair<t_value_type>;
        using value_type = t_value_type;

        // ~~ Json names ~~
        static constexpr char jstr_null[] = "null";
        static constexpr char jstr_alt[] = "alt";

    private:
        value_type m_null = {}; // Null hypothesis value.
        value_type m_alt = {}; // Alternative hypothesis value.

    public:
        hypothesis_pair() noexcept { }

        hypothesis_pair(const value_type& null_value, const value_type& alt_value) noexcept
            : m_null(null_value), m_alt(alt_value)
        {
        } // hypothesis_pair(...)

        hypothesis_pair(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Parse json entries.
            aftermath::noexcept_json::required(j, type::jstr_null, this->m_null, ec);
            aftermath::noexcept_json::required(j, type::jstr_alt, this->m_alt, ec);
        } // hypothesis_pair(...)

        /** @brief Signal "strength" under the null hypothesis. */
        const value_type& null() const noexcept { return this->m_null; }
        /** @brief Minimal signal "strength" under the alternative hypothesis. */
        const value_type& alt() const noexcept { return this->m_alt; }

        /** Checks if the two objects are equal. */
        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_null == other.m_null &&
                this->m_alt == other.m_alt;
        } // operator ==(...)

        /** Checks if the two objects are not equal. */
        bool operator !=(const type& other) const noexcept { return !this->operator ==(other); }

        /** @brief Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct hypothesis_pair

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char hypothesis_pair<t_value_type>::jstr_null[];
    template <typename t_value_type> constexpr char hypothesis_pair<t_value_type>::jstr_alt[];
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const hypothesis_pair<t_value_type>& x) noexcept
    {
        using type = hypothesis_pair<t_value_type>;

        j = nlohmann::json{
            {type::jstr_null, x.null()},
            {type::jstr_alt, x.alt()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, hypothesis_pair<t_value_type>& x)
    {
        using type = hypothesis_pair<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
