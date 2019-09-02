
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <iostream>   // std::ostream
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string
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
            if (j.is_array())
            {
                std::vector<value_type> pair {};
                aftermath::noexcept_json::as(j, pair, ec);
                if (ec.value() != 0) return;
                if (pair.size() != 2) // Range should be a vector with two entries.
                {
                    ec = std::make_error_code(std::errc::bad_message);
                    return;
                } // if (...)
                this->m_null = pair.front();
                this->m_alt = pair.back();
            } // if (...)
            else
            {
                // Parse json entries.
                value_type null = this->m_null;
                value_type alt = this->m_alt;
                aftermath::noexcept_json::required(j, type::jstr_null, null, ec);
                aftermath::noexcept_json::required(j, type::jstr_alt, alt, ec);
                if (ec.value() != 0) return;

                // Populate values.
                this->m_null = null;
                this->m_alt = alt;
            } // else (...)
        } // hypothesis_pair(...)

        /** @brief Value under the null hypothesis. */
        const value_type& null() const noexcept { return this->m_null; }
        /** @brief Value under the alternative hypothesis. */
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
        if (ec.value() != 0) throw std::runtime_error("Parsing <hypothesis_pair> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace std
{
    template <typename t_value_type>
    struct hash<ropufu::sequential::hypotheses::hypothesis_pair<t_value_type>>
    {
        using argument_type = ropufu::sequential::hypotheses::hypothesis_pair<t_value_type>;
        using result_type = std::size_t;

        result_type operator ()(argument_type const& x) const noexcept
        {
            std::hash<typename argument_type::value_type> value_hash = {};
            return
                (value_hash(x.null()) << 4) ^ 
                (value_hash(x.alt()));
        } // operator ()(...)
    }; // struct hash<...>
} // namespace std

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
