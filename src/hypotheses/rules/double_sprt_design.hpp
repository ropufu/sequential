
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../format.hpp"

#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error, std::logc_error
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

        static bool is_valid(value_type relative_mu_intermediate, std::string& message) noexcept
        {
            if (!aftermath::is_finite(relative_mu_intermediate))
            {
                message = "Relative intermediate mu must be finite.";
                return false;
            } // if (...)
            if (relative_mu_intermediate <= 0 || relative_mu_intermediate >= 1)
            {
                message = "Relative intermediate mu must be positive and less than one.";
                return false;
            } // if (...)
            return true;
        } // validate(...)

        void validate() const
        {
            std::string message {};
            if (!type::is_valid(this->m_relative_mu_intermediate, message))
                throw std::logic_error(message);
        } // validate(...)

    public:
        double_sprt_design() noexcept { }

        explicit double_sprt_design(std::size_t id) noexcept
            : m_id(id)
        {
        } // double_sprt_design(...)
        
        double_sprt_design(const nlohmann::json& j, std::error_code& ec) noexcept
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
            std::size_t id = this->m_id;
            value_type relative_mu_intermediate = this->m_relative_mu_intermediate;
            bool is_asymptotic = this->m_asymptotic_init;
            bool is_huffman = this->m_huffman_correction;
            aftermath::noexcept_json::required(j, type::jstr_id, id, ec);
            aftermath::noexcept_json::optional(j, type::jstr_asymptotic_init, is_asymptotic, ec);
            aftermath::noexcept_json::optional(j, type::jstr_huffman_correction, is_huffman, ec);
            if (is_asymptotic)
            {
                aftermath::noexcept_json::optional(j, type::jstr_relative_mu_intermediate, relative_mu_intermediate, ec);
            } // if (...)
            else
            {
                aftermath::noexcept_json::required(j, type::jstr_relative_mu_intermediate, relative_mu_intermediate, ec);
            } // else (...)
            if (ec.value() != 0) return;

            // Validate entries.
            std::string message {};
            if (!type::is_valid(relative_mu_intermediate, message))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
            
            // Populate values.
            this->m_id = id;
            this->m_relative_mu_intermediate = relative_mu_intermediate;
            this->m_asymptotic_init = is_asymptotic;
            this->m_huffman_correction = is_huffman;
        } // double_sprt_design(...)

        bool is_threshold_independent() const noexcept { return !(this->m_asymptotic_init || this->m_huffman_correction); }

        std::size_t id() const noexcept { return this->m_id; }
        void set_id(std::size_t value) noexcept { this->m_id = value; }

        value_type relative_mu_intermediate() const noexcept { return this->m_relative_mu_intermediate; }
        void set_relative_mu_intermediate(value_type value)
        {
            this->m_asymptotic_init = false;
            this->m_huffman_correction = false;
            this->m_relative_mu_intermediate = value;
            this->validate();
        } // set_relative_init(...)

        bool asymptotic_init() const noexcept { return this->m_asymptotic_init; }
        void set_asymptotic_init(bool value) noexcept { this->m_asymptotic_init = value; }

        bool huffman_correction() const noexcept { return this->m_huffman_correction; }
        void set_huffman_correction(bool value) noexcept { this->m_huffman_correction = value; }

        std::string to_path_string(std::size_t decimal_places) const noexcept
        {
            std::string result = type::typename_string;
            if (this->m_asymptotic_init)
            {
                if (this->m_huffman_correction) result += " huffman";
                else result += " asymp";
            } // if (...)
            else
            {
                result += " intermediate ";
                result += detail::to_str(this->m_relative_mu_intermediate, decimal_places);
            } // else (...)
            return result;
        } // to_path_string(...)

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_id == other.m_id &&
                this->m_relative_mu_intermediate == other.m_relative_mu_intermediate &&
                this->m_asymptotic_init == other.m_asymptotic_init &&
                this->m_huffman_correction == other.m_huffman_correction;
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
        if (ec.value() != 0) throw std::runtime_error("Parsing <double_sprt_design> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED
