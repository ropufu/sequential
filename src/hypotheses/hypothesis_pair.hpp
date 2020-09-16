
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_HYPOTHESIS_PAIR_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <cstddef>     // std::size_t
#include <functional>  // std::hash
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

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
        static constexpr std::string_view jstr_null = "null";
        static constexpr std::string_view jstr_alt = "alt";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        value_type m_null = {}; // Null hypothesis value.
        value_type m_alt = {}; // Alternative hypothesis value.

    public:
        hypothesis_pair() noexcept { }

        hypothesis_pair(const value_type& null_value, const value_type& alt_value) noexcept
            : m_null(null_value), m_alt(alt_value)
        {
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
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <hypothesis_pair> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::hypothesis_pair<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::hypothesis_pair<t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            if (j.is_array())
            {
                std::vector<value_type> pair {};
                if (!noexcept_json::try_get(j, pair)) return false;
                if (pair.size() != 2) return false; // Range should be a vector with two entries.
                
                x.m_null = pair.front();
                x.m_alt = pair.back();
            } // if (...)
            else
            {
                // Parse json entries.
                if (!noexcept_json::required(j, result_type::jstr_null, x.m_null)) return false;
                if (!noexcept_json::required(j, result_type::jstr_alt, x.m_alt)) return false;
            } // if (...)
            
            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

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
