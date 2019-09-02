
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_DESIGN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_DESIGN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../format.hpp"
#include "adaptive_sprt_flavor.hpp"

#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error, std::logc_error
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc

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
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_flavor[] = "flavor";
        static constexpr char jstr_id[] = "id";
        static constexpr char jstr_relative_mu_null_init[] = "relative mu null init";
        static constexpr char jstr_relative_mu_alt_init[] = "relative mu alt init";
        static constexpr char jstr_asymptotic_init[] = "asymptotic init";

    private:
        adaptive_sprt_flavor m_flavor = adaptive_sprt_flavor::simple;
        std::size_t m_id = 0;
        value_type m_relative_mu_null_init = 0;
        value_type m_relative_mu_alt_init = 1;
        bool m_asymptotic_init = true;

        static bool is_valid(value_type relative_mu_null_init, value_type relative_mu_alt_init, std::string& message) noexcept
        {
            if (!aftermath::is_finite(relative_mu_null_init))
            {
                message = "Relative init for mu null must be finite.";
                return false;
            } // if (...)
            if (!aftermath::is_finite(relative_mu_alt_init))
            {
                message = "Relative init for mu alt must be finite.";
                return false;
            } // if (...)
            if (relative_mu_null_init < 0 || relative_mu_null_init > 1)
            {
                message = "Relative init for mu null must be between zero and one.";
                return false;
            } // if (...)
            if (relative_mu_alt_init < 0 || relative_mu_alt_init > 1)
            {
                message = "Relative init for mu alt must be between zero and one.";
                return false;
            } // if (...)
            return true;
        } // validate(...)

        void validate() const
        {
            std::string message {};
            if (!type::is_valid(this->m_relative_mu_null_init, this->m_relative_mu_alt_init, message))
                throw std::logic_error(message);
        } // validate(...)

    public:
        adaptive_sprt_design() noexcept { }

        explicit adaptive_sprt_design(adaptive_sprt_flavor flavor, std::size_t id) noexcept
            : m_flavor(flavor), m_id(id)
        {
        } // adaptive_sprt_design(...)
        
        adaptive_sprt_design(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                ec = std::make_error_code(std::errc::bad_message); // SPRT type mismatch.
                return;
            } // if (...)

            // Parse json entries.
            adaptive_sprt_flavor flavor = this->m_flavor;
            std::size_t id = this->m_id;
            value_type relative_mu_null_init = this->m_relative_mu_null_init;
            value_type relative_mu_alt_init = this->m_relative_mu_alt_init;
            bool is_asymptotic = this->m_asymptotic_init;
            aftermath::noexcept_json::required(j, type::jstr_flavor, flavor, ec);
            aftermath::noexcept_json::required(j, type::jstr_id, id, ec);
            aftermath::noexcept_json::optional(j, type::jstr_asymptotic_init, is_asymptotic, ec);
            if (is_asymptotic)
            {
                aftermath::noexcept_json::optional(j, type::jstr_relative_mu_null_init, relative_mu_null_init, ec);
                aftermath::noexcept_json::optional(j, type::jstr_relative_mu_alt_init, relative_mu_alt_init, ec);
            } // if (...)
            else
            {
                aftermath::noexcept_json::required(j, type::jstr_relative_mu_null_init, relative_mu_null_init, ec);
                aftermath::noexcept_json::required(j, type::jstr_relative_mu_alt_init, relative_mu_alt_init, ec);
            } // else (...)
            if (ec.value() != 0) return;

            // Validate entries.
            std::string message {};
            if (!type::is_valid(relative_mu_null_init, relative_mu_alt_init, message))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
            
            // Populate values.
            this->m_flavor = flavor;
            this->m_id = id;
            this->m_relative_mu_null_init = relative_mu_null_init;
            this->m_relative_mu_alt_init = relative_mu_alt_init;
            this->m_asymptotic_init = is_asymptotic;
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
            if (this->m_asymptotic_init) result += " asymp";
            else
            {
                result += " guess null ";
                result += detail::to_str(this->m_relative_mu_null_init, decimal_places);
                result += " alt ";
                result += detail::to_str(this->m_relative_mu_alt_init, decimal_places);
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

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char adaptive_sprt_design<t_value_type>::jstr_typename[];
    template <typename t_value_type> constexpr char adaptive_sprt_design<t_value_type>::jstr_flavor[];
    template <typename t_value_type> constexpr char adaptive_sprt_design<t_value_type>::jstr_id[];
    template <typename t_value_type> constexpr char adaptive_sprt_design<t_value_type>::jstr_relative_mu_null_init[];
    template <typename t_value_type> constexpr char adaptive_sprt_design<t_value_type>::jstr_relative_mu_alt_init[];
    template <typename t_value_type> constexpr char adaptive_sprt_design<t_value_type>::jstr_asymptotic_init[];
    
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
        using type = adaptive_sprt_design<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <adaptive_sprt_design> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_ADAPTIVE_SPRT_DESIGN_HPP_INCLUDED
