
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include <ropufu/algebra/range.hpp>

#include "../hypotheses/format.hpp"
#include "../hypotheses/hypothesis_pair.hpp"
#include "spacing.hpp"

#include <cstddef>   // std::size_t
#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error
#include <string>    // std::string
#include <system_error> // std::error_code
#include <vector>    // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct init_info;
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const init_info<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, init_info<t_value_type>& x);

    /** @brief An auxiliary structure to initialize rules. */
    template <typename t_value_type>
    struct init_info
    {
        using type = init_info<t_value_type>;
        using value_type = t_value_type;
        using range_type = aftermath::algebra::range<t_value_type>;

        // ~~ Json names ~~
        static constexpr char jstr_rule_id[] = "id";
        static constexpr char jstr_threshold_range[] = "threshold range";
        static constexpr char jstr_anticipated_run_length[] = "anticipated run length";

    private:
        std::size_t m_rule_id = 0;
        hypothesis_pair<range_type> m_threshold_range = {};
        value_type m_anticipated_run_length = 0;

        static bool is_valid(value_type anticipated_run_length, std::string& message) noexcept
        {
            if (!aftermath::is_finite(anticipated_run_length) || anticipated_run_length < 0)
            {
                message = "Anticipated run length must be positive or zero.";
                return false;
            } // if (...)
            return true;
        } // validate(...)

        void validate() const
        {
            std::string message {};
            if (!type::is_valid(this->m_anticipated_run_length, message))
                throw std::logic_error(message);
        } // validate(...)

    public:
        init_info() noexcept { }

        explicit init_info(std::size_t id) noexcept : m_rule_id(id) { }

        init_info(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Parse json entries.
            std::size_t rule_id = this->m_rule_id;
            hypothesis_pair<range_type> threshold_range = this->m_threshold_range;
            value_type anticipated_run_length = this->m_anticipated_run_length;
            aftermath::noexcept_json::required(j, type::jstr_rule_id, rule_id, ec);
            aftermath::noexcept_json::required(j, type::jstr_threshold_range, threshold_range, ec);
            aftermath::noexcept_json::optional(j, type::jstr_anticipated_run_length, anticipated_run_length, ec);
            if (ec.value() != 0) return;

            // Validate entries.
            std::string message {};
            if (!type::is_valid(anticipated_run_length, message))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
            
            // Populate values.
            this->m_rule_id = rule_id;
            this->m_threshold_range = threshold_range;
            this->m_anticipated_run_length = anticipated_run_length;
        } // init_info(...)
        
        std::size_t rule_id() const noexcept { return this->m_rule_id; }

        const hypothesis_pair<range_type>& threshold_range() const noexcept { return this->m_threshold_range; }

        void set_threshold_range(const range_type& null_range, const range_type& alt_range) noexcept
        {
            this->m_threshold_range = hypothesis_pair<range_type>(null_range, alt_range);
        } // set_threshold_range(...)

        void make_thresholds(
            const hypothesis_pair<std::size_t>& count, spacing threshold_spacing,
            std::vector<value_type>& null_thresholds, std::vector<value_type>& alt_thresholds) const
        {
            aftermath::algebra::linear_spacing<value_type> lin {};
            aftermath::algebra::logarithmic_spacing<value_type> log {};
            aftermath::algebra::exponential_spacing<value_type> exp {};

            switch (threshold_spacing)
            {
                case spacing::linear:
                    this->m_threshold_range.null().explode(null_thresholds, count.null(), lin);
                    this->m_threshold_range.alt().explode(alt_thresholds, count.alt(), lin);
                    break;
                case spacing::logarithmic:
                    this->m_threshold_range.null().explode(null_thresholds, count.null(), log);
                    this->m_threshold_range.alt().explode(alt_thresholds, count.alt(), log);
                    break;
                case spacing::exponential:
                    this->m_threshold_range.null().explode(null_thresholds, count.null(), exp);
                    this->m_threshold_range.alt().explode(alt_thresholds, count.alt(), exp);
                    break;
                default:
                    throw std::invalid_argument("Spacing not recognized.");
            } // switch (...)
        } // make_thresholds(...)

        value_type anticipated_run_length() const noexcept { return this->m_anticipated_run_length; }

        void set_anticipated_run_length(value_type value)
        {
            this->m_anticipated_run_length = value;
            this->validate();
        } // set_anticipated_run_length(...)

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_rule_id == other.m_rule_id &&
                this->m_threshold_range == other.m_threshold_range &&
                this->m_anticipated_run_length == other.m_anticipated_run_length;
        } // operator ==(...)

        bool operator !=(const type& other) const noexcept
        {
            return !this->operator ==(other);
        } // operator !=(...)

        /** @brief Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct model

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char init_info<t_value_type>::jstr_rule_id[];
    template <typename t_value_type> constexpr char init_info<t_value_type>::jstr_threshold_range[];
    template <typename t_value_type> constexpr char init_info<t_value_type>::jstr_anticipated_run_length[];
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const init_info<t_value_type>& x) noexcept
    {
        using type = init_info<t_value_type>;

        j = nlohmann::json{
            {type::jstr_rule_id, x.rule_id()},
            {type::jstr_threshold_range, x.threshold_range()},
            {type::jstr_anticipated_run_length, x.anticipated_run_length()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, init_info<t_value_type>& x)
    {
        using type = init_info<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <init_info> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED
