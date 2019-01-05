
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_A_DESIGN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_A_DESIGN_HPP_INCLUDED

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
    struct generalized_sprt_a_design;

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const generalized_sprt_a_design<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, generalized_sprt_a_design<t_value_type>& x);

    template <typename t_value_type>
    struct generalized_sprt_a_design
    {
        using type = generalized_sprt_a_design<t_value_type>;
        using value_type = t_value_type;
        
        static constexpr char typename_string[] = "generalized sprt a";
        
        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_id[] = "id";

    private:
        std::size_t m_id = 0;

    protected:
        constexpr bool validate(std::error_code& /*ec*/) const noexcept { return true; }

        void coerce() noexcept { }

    public:
        generalized_sprt_a_design() noexcept { }

        generalized_sprt_a_design(std::size_t id, std::error_code& ec) noexcept
            : m_id(id)
        {
            if (!this->validate(ec)) this->coerce();
        } // generalized_sprt_a_design(...)
        
        generalized_sprt_a_design(const nlohmann::json& j, std::error_code& ec) noexcept
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

            if (!this->validate(ec)) this->coerce();
        } // generalized_sprt_a_design(...)

        constexpr bool is_threshold_independent() const noexcept { return true; }

        std::size_t id() const noexcept { return this->m_id; }

        std::string to_path_string(std::size_t /*decimal_places*/) const noexcept
        {
            std::string result = type::typename_string;
            return result;
        } // to_path_string(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct generalized_sprt_a_design

    // ~~ Definitions ~~
    template <typename t_value_type> constexpr char generalized_sprt_a_design<t_value_type>::typename_string[];

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char generalized_sprt_a_design<t_value_type>::jstr_typename[];
    template <typename t_value_type> constexpr char generalized_sprt_a_design<t_value_type>::jstr_id[];
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const generalized_sprt_a_design<t_value_type>& x) noexcept
    {
        using type = generalized_sprt_a_design<t_value_type>;
        std::string sprt_type_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, sprt_type_str},
            {type::jstr_id, x.id()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, generalized_sprt_a_design<t_value_type>& x)
    {
        using type = generalized_sprt_a_design<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_A_DESIGN_HPP_INCLUDED
