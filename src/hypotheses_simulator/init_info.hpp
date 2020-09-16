
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include <ropufu/algebra/interval.hpp>

#include "../draft/format.hpp"
#include "../hypotheses/hypothesis_pair.hpp"
#include "spacing.hpp"

#include <cstddef>     // std::size_t
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

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
        using interval_type = aftermath::algebra::interval<t_value_type>;

        // ~~ Json names ~~
        static constexpr std::string_view jstr_rule_id = "id";
        static constexpr std::string_view jstr_threshold_range = "threshold range";
        static constexpr std::string_view jstr_anticipated_run_length = "anticipated run length";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        std::size_t m_rule_id = 0;
        hypothesis_pair<interval_type> m_threshold_range = {};
        value_type m_anticipated_run_length = 0;

        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_finite(this->m_anticipated_run_length) || this->m_anticipated_run_length < 0)
                return "Anticipated run length must be positive or zero.";
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        init_info() noexcept { }

        explicit init_info(std::size_t id) noexcept : m_rule_id(id)
        {
        } // init_info(...)
        
        std::size_t rule_id() const noexcept { return this->m_rule_id; }

        const hypothesis_pair<interval_type>& threshold_range() const noexcept { return this->m_threshold_range; }

        void set_threshold_range(const interval_type& null_range, const interval_type& alt_range) noexcept
        {
            this->m_threshold_range = hypothesis_pair<interval_type>(null_range, alt_range);
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
                    aftermath::algebra::explode(this->m_threshold_range.null(), null_thresholds, count.null(), lin);
                    aftermath::algebra::explode(this->m_threshold_range.alt(), alt_thresholds, count.alt(), lin);
                    break;
                case spacing::logarithmic:
                    aftermath::algebra::explode(this->m_threshold_range.null(), null_thresholds, count.null(), log);
                    aftermath::algebra::explode(this->m_threshold_range.alt(), alt_thresholds, count.alt(), log);
                    break;
                case spacing::exponential:
                    aftermath::algebra::explode(this->m_threshold_range.null(), null_thresholds, count.null(), exp);
                    aftermath::algebra::explode(this->m_threshold_range.alt(), alt_thresholds, count.alt(), exp);
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
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <init_info> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::init_info<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::init_info<t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            // Parse json entries.
            if (!noexcept_json::required(j, result_type::jstr_rule_id, x.m_rule_id)) return false;
            if (!noexcept_json::required(j, result_type::jstr_threshold_range, x.m_threshold_range)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_anticipated_run_length, x.m_anticipated_run_length)) return false;

            // Validate entries.
            if (x.error_message().has_value()) return false;

            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_INIT_INFO_HPP_INCLUDED
