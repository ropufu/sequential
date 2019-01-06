
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_B_DESIGN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_B_DESIGN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/on_error.hpp>
#include "../../draft/algebra/numbers.hpp"

#include "../core.hpp"

#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct adaptive_sprt_b_design;

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const adaptive_sprt_b_design<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, adaptive_sprt_b_design<t_value_type>& x);

    template <typename t_value_type>
    struct adaptive_sprt_b_design
    {
        using type = adaptive_sprt_b_design<t_value_type>;
        using value_type = t_value_type;
        
        static constexpr char typename_string[] = "adaptive sprt b";
        
        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_id[] = "id";
        static constexpr char jstr_relative_mu_guess_null[] = "relative mu guess null";
        static constexpr char jstr_relative_mu_guess_alt[] = "relative mu guess alt";
        static constexpr char jstr_asymptotic_init[] = "asymptotic init";

    private:
        std::size_t m_id = 0;
        value_type m_relative_mu_guess_for_null = 0;
        value_type m_relative_mu_guess_for_alt = 1;
        bool m_asymptotic_init = false;

    protected:
        bool validate(std::error_code& ec) const noexcept
        {
            if (modules::is_nan(this->m_relative_mu_guess_for_null) || modules::is_infinite(this->m_relative_mu_guess_for_null)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Relative guess has to be a finite number.", false);
            if (modules::is_nan(this->m_relative_mu_guess_for_alt) || modules::is_infinite(this->m_relative_mu_guess_for_alt)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Relative guess has to be a finite number.", false);
            if (this->m_relative_mu_guess_for_null < 0 || this->m_relative_mu_guess_for_null > 1) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Relative guess has to be between zero and one.", false);
            if (this->m_relative_mu_guess_for_alt < 0 || this->m_relative_mu_guess_for_alt > 1) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Relative guess has to be between zero and one.", false);
            return true;
        } // validate(...)

        void coerce() noexcept
        {
            if (modules::is_nan(this->m_relative_mu_guess_for_null) || modules::is_infinite(this->m_relative_mu_guess_for_null)) this->m_relative_mu_guess_for_null = 0;
            if (modules::is_nan(this->m_relative_mu_guess_for_alt) || modules::is_infinite(this->m_relative_mu_guess_for_alt)) this->m_relative_mu_guess_for_alt = 1;
            if (this->m_relative_mu_guess_for_null < 0) this->m_relative_mu_guess_for_null = 0;
            if (this->m_relative_mu_guess_for_null > 1) this->m_relative_mu_guess_for_null = 1;
            if (this->m_relative_mu_guess_for_alt < 0) this->m_relative_mu_guess_for_alt = 0;
            if (this->m_relative_mu_guess_for_alt > 1) this->m_relative_mu_guess_for_alt = 1;
        } // coerce(...)

    public:
        adaptive_sprt_b_design() noexcept { }

        adaptive_sprt_b_design(std::size_t id, value_type relative_mu_guess_for_null, value_type relative_mu_guess_for_alt, std::error_code& ec) noexcept
            : m_id(id),
            m_relative_mu_guess_for_null(relative_mu_guess_for_null),
            m_relative_mu_guess_for_alt(relative_mu_guess_for_alt)
        {
            if (!this->validate(ec)) this->coerce();
        } // adaptive_sprt_b_design(...)
        
        adaptive_sprt_b_design(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                aftermath::detail::on_error(ec, std::errc::invalid_argument, "SPRT type mismatch.");
                return;
            } // if (...)

            // Parse json entries.
            aftermath::noexcept_json::required(j, type::jstr_id, this->m_id, ec);
            bool is_asymptotic = j.count(type::jstr_asymptotic_init) != 0;
            if (is_asymptotic)
            {
                aftermath::noexcept_json::optional(j, type::jstr_relative_mu_guess_null, this->m_relative_mu_guess_for_null, ec);
                aftermath::noexcept_json::optional(j, type::jstr_relative_mu_guess_alt, this->m_relative_mu_guess_for_alt, ec);
                aftermath::noexcept_json::required(j, type::jstr_asymptotic_init, this->m_asymptotic_init, ec);
            } // if (...)
            else
            {
                aftermath::noexcept_json::required(j, type::jstr_relative_mu_guess_null, this->m_relative_mu_guess_for_null, ec);
                aftermath::noexcept_json::required(j, type::jstr_relative_mu_guess_alt, this->m_relative_mu_guess_for_alt, ec);
                aftermath::noexcept_json::optional(j, type::jstr_asymptotic_init, this->m_asymptotic_init, ec);
            } // else (...)

            if (!this->validate(ec)) this->coerce();
        } // adaptive_sprt_b_design(...)

        bool is_threshold_independent() const noexcept { return !this->m_asymptotic_init; }

        std::size_t id() const noexcept { return this->m_id; }
        value_type relative_mu_guess_for_null() const noexcept { return this->m_relative_mu_guess_for_null; }
        value_type relative_mu_guess_for_alt() const noexcept { return this->m_relative_mu_guess_for_alt; }
        bool asymptotic_init() const noexcept { return this->m_asymptotic_init; }

        std::string to_path_string(std::size_t decimal_places) const noexcept
        {
            std::string result = type::typename_string;
            result += " guess null ";
            result += detail::to_str(this->m_relative_mu_guess_for_null, decimal_places);
            result += " alt ";
            result += detail::to_str(this->m_relative_mu_guess_for_alt, decimal_places);
            return result;
        } // to_path_string(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct adaptive_sprt_b_design

    // ~~ Definitions ~~
    template <typename t_value_type> constexpr char adaptive_sprt_b_design<t_value_type>::typename_string[];

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char adaptive_sprt_b_design<t_value_type>::jstr_typename[];
    template <typename t_value_type> constexpr char adaptive_sprt_b_design<t_value_type>::jstr_id[];
    template <typename t_value_type> constexpr char adaptive_sprt_b_design<t_value_type>::jstr_relative_mu_guess_null[];
    template <typename t_value_type> constexpr char adaptive_sprt_b_design<t_value_type>::jstr_relative_mu_guess_alt[];
    template <typename t_value_type> constexpr char adaptive_sprt_b_design<t_value_type>::jstr_asymptotic_init[];
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const adaptive_sprt_b_design<t_value_type>& x) noexcept
    {
        using type = adaptive_sprt_b_design<t_value_type>;
        std::string sprt_type_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, sprt_type_str},
            {type::jstr_id, x.id()},
            {type::jstr_relative_mu_guess_null, x.relative_mu_guess_for_null()},
            {type::jstr_relative_mu_guess_alt, x.relative_mu_guess_for_alt()},
            {type::jstr_asymptotic_init, x.asymptotic_init()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, adaptive_sprt_b_design<t_value_type>& x)
    {
        using type = adaptive_sprt_b_design<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_ADAPTIVE_SPRT_B_DESIGN_HPP_INCLUDED
