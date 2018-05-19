
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_INIT_INFO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_INIT_INFO_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <aftermath/quiet_json.hpp>

#include <aftermath/algebra.hpp>      // aftermath::algebra::range
#include <aftermath/not_an_error.hpp> // quiet_error, not_an_error, severity_level

#include "../hypotheses/core.hpp"
#include "../hypotheses/modules/interpolator.hpp"
#include "../hypotheses/modules/numbers.hpp"
#include "hypothesis_pair.hpp"

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
                using range_type = aftermath::algebra::range<t_value_type>;

                // ~~ Json names ~~
                static constexpr char jstr_rule_id[] = "id";
                static constexpr char jstr_threshold_range[] = "threshold range";
                static constexpr char jstr_anticipated_run_length[] = "anticipated run length";

            private:
                std::size_t m_rule_id = 0;
                hypothesis_pair<range_type> m_threshold_range = { };
                value_type m_anticipated_run_length = { };

            public:
                init_info() noexcept { }

                init_info(std::size_t id) noexcept : m_rule_id(id) { }

                std::size_t rule_id() const noexcept { return this->m_rule_id; }

                const hypothesis_pair<range_type>& threshold_range() const noexcept { return this->m_threshold_range; }

                void set_threshold_range(const range_type& null_range, const range_type& alt_range) noexcept
                {
                    this->m_threshold_range = hypothesis_pair<range_type>(null_range, alt_range);
                } // set_threshold_range(...)

                void make_thresholds(const hypothesis_pair<std::size_t>& count, aftermath::algebra::spacing transform, std::vector<value_type>& null_thresholds, std::vector<value_type>& alt_thresholds) const noexcept
                {
                    this->m_threshold_range.null().explode(null_thresholds, count.null(), transform);
                    this->m_threshold_range.alt().explode(alt_thresholds, count.alt(), transform);
                } // make_thresholds(...)

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
            void from_json(const nlohmann::json& j, init_info<t_value_type>& x) noexcept
            {
                aftermath::quiet_json q(j);
                using type = init_info<t_value_type>;

                // Populate default values.
                std::size_t rule_id = x.rule_id();
                hypothesis_pair<typename type::range_type> threshold_range = x.threshold_range();
                t_value_type anticipated_run_length = x.anticipated_run_length();

                // Parse json entries.
                q.required(type::jstr_rule_id, rule_id);
                q.required(type::jstr_threshold_range, threshold_range);
                q.optional(type::jstr_anticipated_run_length, anticipated_run_length);
                
                // Reconstruct the object.
                if (!q.good())
                {
                    aftermath::quiet_error::instance().push(
                        aftermath::not_an_error::runtime_error,
                        aftermath::severity_level::major,
                        q.message(), __FUNCTION__, __LINE__);
                    return;
                } // if (...)
                x = type(rule_id);
                x.set_threshold_range(threshold_range.null(), threshold_range.alt());
                x.set_anticipated_run_length(anticipated_run_length);
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

                aftermath::algebra::range<t_value_type> null_thresholds(
                    (q) * left.threshold_range().null().from() + (p) * right.threshold_range().null().from(),
                    (q) * left.threshold_range().null().to() + (p) * right.threshold_range().null().to());
                aftermath::algebra::range<t_value_type> alt_thresholds(
                    (q) * left.threshold_range().alt().from() + (p) * right.threshold_range().alt().from(),
                    (q) * left.threshold_range().alt().to() + (p) * right.threshold_range().alt().to());
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
