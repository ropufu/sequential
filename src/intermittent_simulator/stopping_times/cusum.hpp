
#ifndef ROPUFU_SEQUENTIAL_INTERMITTENT_CUSUM_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_INTERMITTENT_CUSUM_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/format/cat.hpp>

#include "../stopping_time.hpp"

#include <concepts>    // std::totally_ordered
#include <cstddef>     // std::size_t
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::forward
#include <vector>      // std::vector

namespace ropufu::sequential::intermittent
{
    template <std::totally_ordered t_value_type>
    struct cusum;

    template <std::totally_ordered t_value_type>
    void to_json(nlohmann::json& j, const cusum<t_value_type>& x) noexcept;
    template <std::totally_ordered t_value_type>
    void from_json(const nlohmann::json& j, cusum<t_value_type>& x);

    /** Classical CUSUM chart. */
    template <std::totally_ordered t_value_type>
    struct cusum : public stopping_time<t_value_type>
    {
        using type = cusum<t_value_type>;
        using base_type = stopping_time<t_value_type>;
        using value_type = t_value_type;

        static constexpr std::string_view typename_string = "CUSUM";

        using thresholds_type = typename base_type::thresholds_type;

        // ~~ Json names ~~
        static constexpr std::string_view jstr_typename = "type";
        static constexpr std::string_view jstr_window_size = "window";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        // Latest statistic value.
        value_type m_statistic = 0;

    public:
        cusum() noexcept : cusum(thresholds_type{})
        {
        } // cusum(...)

        explicit cusum(const thresholds_type& thresholds)
            : base_type(thresholds)
        {
        } // cusum(...)

        explicit cusum(thresholds_type&& thresholds)
            : base_type(std::forward<thresholds_type>(thresholds))
        {
        } // cusum(...)

        std::string name() const noexcept override
        {
            return std::string{type::typename_string};
        } // name(...)

        friend void to_json(nlohmann::json& j, const type& x) noexcept
        {
            j = nlohmann::json{
                {type::jstr_typename, type::typename_string}
            };
            x.serialize_core(j);
        } // to_json(...)

        friend void from_json(const nlohmann::json& j, type& x)
        {
            if (!ropufu::noexcept_json::try_get(j, x))
                throw std::runtime_error("Parsing <cusum> failed: " + j.dump());
        } // from_json(...)

    protected:
        value_type update_statistic(const value_type& /*raw_value*/, const value_type& log_likelihood_ratio) noexcept override
        {
            value_type previous = this->m_statistic;
            if (previous < 0) previous = 0;
            this->m_statistic = previous + log_likelihood_ratio;
            return this->m_statistic;
        } // update_statistic(...)

        std::vector<value_type> update_statistic(const std::vector<value_type>& /*raw_values*/, const std::vector<value_type>& log_likelihood_ratios) noexcept override
        {
            std::vector<value_type> result {};
            result.reserve(log_likelihood_ratios.size());
            for (std::size_t k = 0; k < log_likelihood_ratios.size(); ++k)
            {
                value_type previous = this->m_statistic;
                if (previous < 0) previous = 0;
                this->m_statistic = previous + log_likelihood_ratios[k];
                result.push_back(this->m_statistic);
            } // for (...)

            return result;
        } // update_statistic(...)

        void on_reset() noexcept override
        {
            this->m_statistic = 0;
        } // on_reset(...)

        nlohmann::json serialize() const noexcept override
        {
            nlohmann::json j = *this;
            return j;
        } // serialize(...)
    }; // struct cusum
} // namespace ropufu::sequential::intermittent

namespace ropufu
{
    template <std::totally_ordered t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::intermittent::cusum<t_value_type>>
    {
        using result_type = ropufu::sequential::intermittent::cusum<t_value_type>;
        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            std::string typename_string;
            std::size_t window_size = 0;

            if (!noexcept_json::required(j, result_type::jstr_typename, typename_string)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_window_size, window_size)) return false;

            if (typename_string != result_type::typename_string) return false;
            if (window_size != 0) return false;
            
            if (!x.try_deserialize_core(j)) return false;
            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_INTERMITTENT_CUSUM_HPP_INCLUDED
