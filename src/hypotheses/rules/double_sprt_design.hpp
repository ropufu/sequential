
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../../draft/format.hpp"

#include <cstddef>     // std::size_t
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

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
        static constexpr std::string_view jstr_typename = "type";
        static constexpr std::string_view jstr_id = "id";
        static constexpr std::string_view jstr_relative_mu_intermediate = "relative mu intermediate";
        static constexpr std::string_view jstr_asymptotic_init = "asymptotic init";
        static constexpr std::string_view jstr_huffman_correction = "huffman";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        std::size_t m_id = 0;
        value_type m_relative_mu_intermediate = static_cast<value_type>(0.5); // Relative threshold used to decide in favor of either of the hypotheses.
        bool m_asymptotic_init = false;
        bool m_huffman_correction = false;

        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_probability(this->m_relative_mu_intermediate)) return "Relative intermediate mu must be positive and less than one.";
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        double_sprt_design() noexcept { }

        explicit double_sprt_design(std::size_t id) noexcept
            : m_id(id)
        {
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
                result += ropufu::draft::detail::to_str(this->m_relative_mu_intermediate, decimal_places);
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
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <double_sprt_design> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::double_sprt_design<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::double_sprt_design<t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            if (!noexcept_json::required(j, result_type::jstr_typename, typename_str)) return false;
            if (typename_str != result_type::typename_string) return false; // SPRT type mismatch.

            // Parse json entries.
            if (!noexcept_json::required(j, result_type::jstr_id, x.m_id)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_asymptotic_init, x.m_asymptotic_init)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_huffman_correction, x.m_huffman_correction)) return false;
            if (x.m_asymptotic_init)
            {
                if (!noexcept_json::optional(j, result_type::jstr_relative_mu_intermediate, x.m_relative_mu_intermediate)) return false;
            } // if (...)
            else
            {
                if (!noexcept_json::required(j, result_type::jstr_relative_mu_intermediate, x.m_relative_mu_intermediate)) return false;
            } // if (...)
            
            // Validate entries.
            if (x.error_message().has_value()) return false;

            return true;
        } // double_sprt_design(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_RULES_DOUBLE_SPRT_DESIGN_HPP_INCLUDED
