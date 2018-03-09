
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_INIT_INFO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_INIT_INFO_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../hypotheses/json.hpp"

#include <aftermath/not_an_error.hpp> // quiet_error, not_an_error, severity_level

#include "../hypotheses/core.hpp"
#include "../hypotheses/matlab.hpp"
#include "../hypotheses/modules/interpolator.hpp"
#include "../hypotheses/modules/numbers.hpp"

#include <cmath>    // std::isnan, std::isinf
#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <string>   // std::string
#include <system_error> // std::error_code
#include <vector>   // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            /** @brief An auxiliary structure to initialize rules. */
            template <typename t_value_type>
            struct init_info
            {
                using type = init_info<t_value_type>;
                using value_type = t_value_type;

                // ~~ Json names ~~
                static constexpr char jstr_rule_id[] = "id";
                static constexpr char jstr_null_thresholds[] = "null threshold range";
                static constexpr char jstr_alt_thresholds[] = "alt threshold range";
                static constexpr char jstr_anticipated_run_length[] = "anticipated run length";

            private:
                std::size_t m_rule_id = 0;
                value_type m_null_thresholds_from = 0;
                value_type m_null_thresholds_to = 0;
                value_type m_alt_thresholds_from = 0;
                value_type m_alt_thresholds_to = 0;
                value_type m_anticipated_run_length = 0;

            public:
                init_info() noexcept { }

                init_info(std::size_t id) noexcept : m_rule_id(id) { }

                std::size_t rule_id() const noexcept { return this->m_rule_id; }

                value_type null_thresholds_from() const noexcept { return this->m_null_thresholds_from; }
                value_type null_thresholds_to() const noexcept { return this->m_null_thresholds_to; }

                value_type alt_thresholds_from() const noexcept { return this->m_alt_thresholds_from; }
                value_type alt_thresholds_to() const noexcept { return this->m_alt_thresholds_to; }

                void set_threshold_range(const std::vector<value_type>& null_range, const std::vector<value_type>& alt_range) noexcept
                {
                    if (null_range.size() != 2 || alt_range.size() != 2)
                    {
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "Threshold range should be a vector with two entries.", __FUNCTION__, __LINE__);
                        return;
                    }

                    this->m_null_thresholds_from = null_range.front();
                    this->m_null_thresholds_to = null_range.back();

                    this->m_alt_thresholds_from = alt_range.front();
                    this->m_alt_thresholds_to = alt_range.back();
                } // set_threshold_range(...)

                void make_thresholds(std::size_t count, const std::string& spacing, std::vector<value_type>& null_thresholds, std::vector<value_type>& alt_thresholds) const noexcept
                {
                    null_thresholds = aftermath::matlab<value_type>::parse_space(spacing, this->m_null_thresholds_from, this->m_null_thresholds_to, count);
                    alt_thresholds = aftermath::matlab<value_type>::parse_space(spacing, this->m_alt_thresholds_from, this->m_alt_thresholds_to, count);
                }

                value_type anticipated_run_length() const noexcept { return this->m_anticipated_run_length; }
                void set_anticipated_run_length(value_type value) noexcept { this->m_anticipated_run_length = value; }

                /** @brief Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct model

            // ~~ Json name definitions ~~
            template <typename t_value_type> constexpr char init_info<t_value_type>::jstr_rule_id[];
            template <typename t_value_type> constexpr char init_info<t_value_type>::jstr_null_thresholds[];
            template <typename t_value_type> constexpr char init_info<t_value_type>::jstr_alt_thresholds[];
            template <typename t_value_type> constexpr char init_info<t_value_type>::jstr_anticipated_run_length[];
            
            template <typename t_value_type>
            void to_json(nlohmann::json& j, const init_info<t_value_type>& x) noexcept
            {
                using type = init_info<t_value_type>;

                std::vector<t_value_type> null_thresholds = { x.null_thresholds_from(), x.null_thresholds_to() };
                std::vector<t_value_type> alt_thresholds = { x.alt_thresholds_from(), x.alt_thresholds_to() };

                j = nlohmann::json{
                    {type::jstr_rule_id, x.rule_id()},
                    {type::jstr_null_thresholds, null_thresholds},
                    {type::jstr_alt_thresholds, alt_thresholds},
                    {type::jstr_anticipated_run_length, x.anticipated_run_length()}
                };
            } // to_json(...)
        
            template <typename t_value_type>
            void from_json(const nlohmann::json& j, init_info<t_value_type>& x) noexcept
            {
                quiet_json q(__FUNCTION__, __LINE__);
                using type = init_info<t_value_type>;

                // Populate default values.
                std::size_t rule_id = x.rule_id();
                std::vector<t_value_type> null_thresholds = { x.null_thresholds_from(), x.null_thresholds_to() };
                std::vector<t_value_type> alt_thresholds = { x.alt_thresholds_from(), x.alt_thresholds_to() };
                t_value_type anticipated_run_length = x.anticipated_run_length();

                // Parse json entries.
                if (!quiet_json::required(j, type::jstr_rule_id, rule_id)) return;
                if (!quiet_json::required(j, type::jstr_null_thresholds, null_thresholds)) return;
                if (!quiet_json::required(j, type::jstr_alt_thresholds, alt_thresholds)) return;
                if (!quiet_json::optional(j, type::jstr_anticipated_run_length, anticipated_run_length)) return;
                
                // Reconstruct the object.
                x = type(rule_id);
                x.set_threshold_range(null_thresholds, alt_thresholds);
                x.set_anticipated_run_length(anticipated_run_length);
                q.validate();
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

namespace ropufu
{
    namespace modules
    {
        template <typename t_value_type, typename t_position_type>
        struct interpolator<sequential::hypotheses::init_info<t_value_type>, t_position_type>
        {
            using type = interpolator<sequential::hypotheses::init_info<t_value_type>, t_position_type>;
            using value_type = sequential::hypotheses::init_info<t_value_type>;
            using position_type = t_position_type;
            using clipper_type = clipper<t_position_type>;

            static value_type interpolate(const value_type& left, const value_type& right, const position_type& relative_position, std::error_code& ec) noexcept
            {
                std::size_t rule_id = left.rule_id();
                if (left.rule_id() != right.rule_id())
                {
                    ec = std::make_error_code(std::errc::function_not_supported);
                    return { };
                } // if (...)

                ec.clear();
                position_type p = relative_position; // Make a copy of <relative_position>.

                if (!clipper_type::was_finite(p, 0) || !clipper_type::was_between(p, 0, 1))
                    aftermath::quiet_error::instance().push(
                        aftermath::not_an_error::logic_error,
                        aftermath::severity_level::major,
                        "Relative position out of range. Clipped to the interval [0, 1].", __FUNCTION__, __LINE__);

                position_type q = 1 - p;

                std::vector<t_value_type> null_thresholds = {
                    (q) * left.null_thresholds_from() + (p) * right.null_thresholds_from(),
                    (q) * left.null_thresholds_to() + (p) * right.null_thresholds_to() };
                std::vector<t_value_type> alt_thresholds = {
                    (q) * left.alt_thresholds_from() + (p) * right.alt_thresholds_from(),
                    (q) * left.alt_thresholds_to() + (p) * right.alt_thresholds_to() };
                t_value_type anticipated_run_length = (q) * left.anticipated_run_length() + (p) * right.anticipated_run_length();

                value_type x(rule_id);
                x.set_threshold_range(null_thresholds, alt_thresholds);
                x.set_anticipated_run_length(anticipated_run_length);
                return x;
            } // interpolate(...)
        }; // struct interpolator<...>
    } // namespace modules
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_INIT_INFO_HPP_INCLUDED
