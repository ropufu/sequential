
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_GENERALIZED_SPRT_DESIGN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_GENERALIZED_SPRT_DESIGN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../../draft/format.hpp"
#include "generalized_sprt_flavor.hpp"

#include <cstddef>     // std::size_t
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct generalized_sprt_design;

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const generalized_sprt_design<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, generalized_sprt_design<t_value_type>& x);

    template <typename t_value_type>
    struct generalized_sprt_design
    {
        using type = generalized_sprt_design<t_value_type>;
        using value_type = t_value_type;
        
        static constexpr char typename_string[] = "generalized sprt";
        
        // ~~ Json names ~~
        static constexpr std::string_view jstr_typename = "type";
        static constexpr std::string_view jstr_flavor = "flavor";
        static constexpr std::string_view jstr_id = "id";
        static constexpr std::string_view jstr_relative_mu_cutoff = "relative mu cutoff";
        static constexpr std::string_view jstr_asymptotic_init = "asymptotic init";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        generalized_sprt_flavor m_flavor = generalized_sprt_flavor::general;
        std::size_t m_id = 0;
        value_type m_relative_mu_cutoff = static_cast<value_type>(0.5); // Relative threshold used to decide in favor of either of the hypotheses.
        bool m_asymptotic_init = true;

        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_probability(this->m_relative_mu_cutoff)) return "Relative mu cutoff must be positive and less than one.";
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        generalized_sprt_design() noexcept { }

        explicit generalized_sprt_design(generalized_sprt_flavor flavor, std::size_t id) noexcept
            : m_flavor(flavor), m_id(id)
        {
        } // generalized_sprt_design(...)
        
        bool is_threshold_independent() const noexcept { return !this->m_asymptotic_init; }

        generalized_sprt_flavor flavor() const noexcept { return this->m_flavor; }
        void set_flavor(generalized_sprt_flavor value) noexcept { this->m_flavor = value; }

        std::size_t id() const noexcept { return this->m_id; }
        void set_id(std::size_t value) noexcept { this->m_id = value; }

        value_type relative_mu_cutoff() const noexcept { return this->m_relative_mu_cutoff; }
        void set_relative_mu_cutoff(value_type relative_mu_cutoff)
        {
            this->m_asymptotic_init = false;
            this->m_relative_mu_cutoff = relative_mu_cutoff;
            this->validate();
        } // set_relative_mu_cutoff(...)

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
                result += " cutoff ";
                result += ropufu::draft::detail::to_str(this->m_relative_mu_cutoff, decimal_places);
            } // else (...)
            return result;
        } // to_path_string(...)

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_flavor == other.m_flavor &&
                this->m_id == other.m_id &&
                this->m_relative_mu_cutoff == other.m_relative_mu_cutoff &&
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
    }; // struct generalized_sprt_design

    // ~~ Definitions ~~
    template <typename t_value_type> constexpr char generalized_sprt_design<t_value_type>::typename_string[];

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const generalized_sprt_design<t_value_type>& x) noexcept
    {
        using type = generalized_sprt_design<t_value_type>;
        std::string sprt_type_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, sprt_type_str},
            {type::jstr_flavor, x.flavor()},
            {type::jstr_id, x.id()},
            {type::jstr_relative_mu_cutoff, x.relative_mu_cutoff()},
            {type::jstr_asymptotic_init, x.asymptotic_init()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, generalized_sprt_design<t_value_type>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <generalized_sprt_design> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::generalized_sprt_design<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::generalized_sprt_design<t_value_type>;

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
                if (!noexcept_json::optional(j, result_type::jstr_relative_mu_cutoff, x.m_relative_mu_cutoff)) return false;
            } // if (...)
            else
            {
                if (!noexcept_json::required(j, result_type::jstr_relative_mu_cutoff, x.m_relative_mu_cutoff)) return false;
            } // if (...)
            
            // Validate entries.
            if (x.error_message().has_value()) return false;

            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_GENERALIZED_SPRT_DESIGN_HPP_INCLUDED
