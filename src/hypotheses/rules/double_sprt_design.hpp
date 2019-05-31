
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED

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
    struct double_sprt_design;

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const double_sprt_design<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, double_sprt_design<t_value_type>& x);

    template <typename t_value_type>
    struct double_sprt_design
    {
        using type = double_sprt_design<t_value_type>;
        using value_type = t_value_type;
        
        static constexpr char typename_string[] = "double sprt";
        
        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_id[] = "id";
        static constexpr char jstr_relative_mu_intermediate[] = "relative mu intermediate";
        static constexpr char jstr_asymptotic_init[] = "asymptotic init";
        static constexpr char jstr_huffman_correction[] = "huffman";

    private:
        std::size_t m_id = 0;
        value_type m_relative_mu_intermediate = static_cast<value_type>(0.5); // Relative threshold used to decide in favor of either of the hypotheses.
        bool m_asymptotic_init = false;
        bool m_huffman_correction = false;

    protected:
        bool validate(std::error_code& ec) const noexcept
        {
            if (modules::is_nan(this->m_relative_mu_intermediate) || modules::is_infinite(this->m_relative_mu_intermediate)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Relative cutoff has to be a finite number.", false);
            if (this->m_relative_mu_intermediate <= 0 || this->m_relative_mu_intermediate >= 1) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Relative cutoff has to be strictly between zero and one.", false);
            return true;
        } // validate(...)

        void coerce() noexcept
        {
            if (modules::is_nan(this->m_relative_mu_intermediate) || modules::is_infinite(this->m_relative_mu_intermediate)) this->m_relative_mu_intermediate = static_cast<value_type>(0.5);
            if (this->m_relative_mu_intermediate <= 0) this->m_relative_mu_intermediate = static_cast<value_type>(0.5);
            if (this->m_relative_mu_intermediate >= 1) this->m_relative_mu_intermediate = static_cast<value_type>(0.5);
        } // coerce(...)

    public:
        double_sprt_design() noexcept { }

        double_sprt_design(std::size_t id, value_type relative_mu_intermediate, std::error_code& ec) noexcept
            : m_id(id),
            m_relative_mu_intermediate(relative_mu_intermediate)
        {
            if (!this->validate(ec)) this->coerce();
        } // double_sprt_design(...)
        
        double_sprt_design(const nlohmann::json& j, std::error_code& ec) noexcept
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
            aftermath::noexcept_json::optional(j, type::jstr_huffman_correction, this->m_huffman_correction, ec);
            bool is_asymptotic = j.count(type::jstr_asymptotic_init) != 0;
            if (is_asymptotic)
            {
                aftermath::noexcept_json::optional(j, type::jstr_relative_mu_intermediate, this->m_relative_mu_intermediate, ec);
                aftermath::noexcept_json::required(j, type::jstr_asymptotic_init, this->m_asymptotic_init, ec);
            } // if (...)
            else
            {
                aftermath::noexcept_json::required(j, type::jstr_relative_mu_intermediate, this->m_relative_mu_intermediate, ec);
                aftermath::noexcept_json::optional(j, type::jstr_asymptotic_init, this->m_asymptotic_init, ec);
            } // else (...)

            if (!this->validate(ec)) this->coerce();
        } // double_sprt_design(...)

        bool is_threshold_independent() const noexcept { return !this->m_asymptotic_init; }

        std::size_t id() const noexcept { return this->m_id; }
        value_type relative_mu_intermediate() const noexcept { return this->m_relative_mu_intermediate; }
        bool asymptotic_init() const noexcept { return this->m_asymptotic_init; }
        bool huffman_correction() const noexcept { return this->m_huffman_correction; }

        std::string to_path_string(std::size_t decimal_places) const noexcept
        {
            std::string result = type::typename_string;
            if (this->m_asymptotic_init)
            {
                result += " asymp";
                if (this->m_huffman_correction) result += " huffman";
            } // if (...)
            else
            {
                result += " intermediate ";
                result += detail::to_str(this->m_relative_mu_intermediate, decimal_places);
            } // else (...)
            return result;
        } // to_path_string(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct double_sprt_design

    // ~~ Definitions ~~
    template <typename t_value_type> constexpr char double_sprt_design<t_value_type>::typename_string[];

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char double_sprt_design<t_value_type>::jstr_typename[];
    template <typename t_value_type> constexpr char double_sprt_design<t_value_type>::jstr_id[];
    template <typename t_value_type> constexpr char double_sprt_design<t_value_type>::jstr_relative_mu_intermediate[];
    template <typename t_value_type> constexpr char double_sprt_design<t_value_type>::jstr_asymptotic_init[];
    template <typename t_value_type> constexpr char double_sprt_design<t_value_type>::jstr_huffman_correction[];
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const double_sprt_design<t_value_type>& x) noexcept
    {
        using type = double_sprt_design<t_value_type>;
        std::string sprt_type_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, sprt_type_str},
            {type::jstr_id, x.id()},
            {type::jstr_relative_mu_intermediate, x.relative_mu_intermediate()},
            {type::jstr_asymptotic_init, x.asymptotic_init()},
            {type::jstr_huffman_correction, x.huffman_correction()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, double_sprt_design<t_value_type>& x)
    {
        using type = double_sprt_design<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED
