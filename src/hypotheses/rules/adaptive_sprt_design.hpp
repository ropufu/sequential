
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_DESIGN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_DESIGN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../../draft/format.hpp"
#include "adaptive_sprt_flavor.hpp"

#include <cstddef>     // std::size_t
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct adaptive_sprt_design;

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const adaptive_sprt_design<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, adaptive_sprt_design<t_value_type>& x);

    template <typename t_value_type>
    struct adaptive_sprt_design
    {
        using type = adaptive_sprt_design<t_value_type>;
        using value_type = t_value_type;
        
        static constexpr char typename_string[] = "adaptive sprt";
        
        // ~~ Json names ~~
        static constexpr std::string_view jstr_typename = "type";
        static constexpr std::string_view jstr_flavor = "flavor";
        static constexpr std::string_view jstr_id = "id";
        static constexpr std::string_view jstr_relative_mu_null_init = "relative mu null init";
        static constexpr std::string_view jstr_relative_mu_alt_init = "relative mu alt init";
        static constexpr std::string_view jstr_asymptotic_init = "asymptotic init";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        adaptive_sprt_flavor m_flavor = adaptive_sprt_flavor::simple;
        std::size_t m_id = 0;
        value_type m_relative_mu_null_init = 0;
        value_type m_relative_mu_alt_init = 1;
        bool m_asymptotic_init = true;

        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_probability(this->m_relative_mu_null_init)) return "Relative init for mu null must be between zero and one.";
            if (!aftermath::is_probability(this->m_relative_mu_alt_init)) return "Relative init for mu alt must be between zero and one.";
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        adaptive_sprt_design() noexcept { }

        explicit adaptive_sprt_design(adaptive_sprt_flavor flavor, std::size_t id) noexcept
            : m_flavor(flavor), m_id(id)
        {
        } // adaptive_sprt_design(...)

        bool is_threshold_independent() const noexcept { return !this->m_asymptotic_init; }

        adaptive_sprt_flavor flavor() const noexcept { return this->m_flavor; }
        void set_flavor(adaptive_sprt_flavor value) noexcept { this->m_flavor = value; }

        std::size_t id() const noexcept { return this->m_id; }
        void set_id(std::size_t value) noexcept { this->m_id = value; }

        value_type relative_mu_null_init() const noexcept { return this->m_relative_mu_null_init; }
        value_type relative_mu_alt_init() const noexcept { return this->m_relative_mu_alt_init; }
        void set_relative_init(value_type relative_mu_null_init, value_type relative_mu_alt_init)
        {
            this->m_asymptotic_init = false;
            this->m_relative_mu_null_init = relative_mu_null_init;
            this->m_relative_mu_alt_init = relative_mu_alt_init;
            this->validate();
        } // set_relative_init(...)

        bool asymptotic_init() const noexcept { return this->m_asymptotic_init; }
        void set_asymptotic_init(bool value) noexcept { this->m_asymptotic_init = value; }

        std::string to_path_string(std::size_t decimal_places) const noexcept
        {
            std::string result = type::typename_string;
            result += " ";
            result += std::to_string(this->m_flavor);
            if (this->m_asymptotic_init) result += " asymp";
            else
            {
                result += " guess null ";
                result += ropufu::draft::detail::to_str(this->m_relative_mu_null_init, decimal_places);
                result += " alt ";
                result += ropufu::draft::detail::to_str(this->m_relative_mu_alt_init, decimal_places);
            } // else (...)
            return result;
        } // to_path_string(...)

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_flavor == other.m_flavor &&
                this->m_id == other.m_id &&
                this->m_relative_mu_null_init == other.m_relative_mu_null_init &&
                this->m_relative_mu_alt_init == other.m_relative_mu_alt_init &&
                this->m_asymptotic_init == other.m_asymptotic_init;
        } // operator ==(...)

        bool operator !=(const type& other) const noexcept
        {
            return !this->operator ==(other);
        } // operator !=(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct adaptive_sprt_design

    // ~~ Definitions ~~
    template <typename t_value_type> constexpr char adaptive_sprt_design<t_value_type>::typename_string[];

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const adaptive_sprt_design<t_value_type>& x) noexcept
    {
        using type = adaptive_sprt_design<t_value_type>;
        std::string sprt_type_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, sprt_type_str},
            {type::jstr_flavor, x.flavor()},
            {type::jstr_id, x.id()},
            {type::jstr_relative_mu_null_init, x.relative_mu_null_init()},
            {type::jstr_relative_mu_alt_init, x.relative_mu_alt_init()},
            {type::jstr_asymptotic_init, x.asymptotic_init()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, adaptive_sprt_design<t_value_type>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <adaptive_sprt_design> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::adaptive_sprt_design<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::adaptive_sprt_design<t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            if (!noexcept_json::required(j, result_type::jstr_typename, typename_str)) return false;
            if (typename_str != result_type::typename_string) return false; // SPRT type mismatch.

            // Parse json entries.
            if (!noexcept_json::required(j, result_type::jstr_flavor, x.m_flavor)) return false;
            if (!noexcept_json::required(j, result_type::jstr_id, x.m_id)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_asymptotic_init, x.m_asymptotic_init)) return false;
            if (x.m_asymptotic_init)
            {
                if (!noexcept_json::optional(j, result_type::jstr_relative_mu_null_init, x.m_relative_mu_null_init)) return false;
                if (!noexcept_json::optional(j, result_type::jstr_relative_mu_alt_init, x.m_relative_mu_alt_init)) return false;
            } // if (...)
            else
            {
                if (!noexcept_json::required(j, result_type::jstr_relative_mu_null_init, x.m_relative_mu_null_init)) return false;
                if (!noexcept_json::required(j, result_type::jstr_relative_mu_alt_init, x.m_relative_mu_alt_init)) return false;
            } // if (...)

            // Validate entries.
            if (x.error_message().has_value()) return false;

            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_DESIGN_HPP_INCLUDED
