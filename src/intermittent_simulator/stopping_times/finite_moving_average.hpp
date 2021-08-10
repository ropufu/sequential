
#ifndef ROPUFU_SEQUENTIAL_INTERMITTENT_FINITE_MOVING_AVERAGE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_INTERMITTENT_FINITE_MOVING_AVERAGE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/format/cat.hpp>

#include "../stopping_time.hpp"

#include <concepts>    // std::totally_ordered
#include <cstddef>     // std::size_t
#include <stdexcept>   // std::runtime_error, std::logic_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::forward
#include <vector>      // std::vector

namespace ropufu::sequential::intermittent
{
    template <std::totally_ordered t_value_type>
    struct finite_moving_average;

    template <std::totally_ordered t_value_type>
    void to_json(nlohmann::json& j, const finite_moving_average<t_value_type>& x) noexcept;
    template <std::totally_ordered t_value_type>
    void from_json(const nlohmann::json& j, finite_moving_average<t_value_type>& x);

    template <std::totally_ordered t_value_type>
    struct finite_moving_average : public stopping_time<t_value_type>
    {
        using type = finite_moving_average<t_value_type>;
        using base_type = stopping_time<t_value_type>;
        using value_type = t_value_type;

        static constexpr std::string_view typename_string = "FMA";
        
        using container_type = std::vector<value_type>;
        using thresholds_type = typename base_type::thresholds_type;

        // ~~ Json names ~~
        static constexpr std::string_view jstr_typename = "type";
        static constexpr std::string_view jstr_window_size = "window";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        // Collection of most recent observations.
        container_type m_container = {};
        // If L is the window size, then the structure at time n is:
        // ... --- (n - L + 1) ---  n --- (n - 1) --- (n - 2) --- ...
        //           oldest       newest
        std::size_t m_newest_index = 0;

        void initialize(std::size_t window_size)
        {
            if (window_size == 0) throw std::logic_error("Window size cannot be zero.");

            this->m_container = container_type(window_size);
            this->m_container.shrink_to_fit();
        } // initialize(...)

    public:
        finite_moving_average() noexcept : finite_moving_average(1, thresholds_type{})
        {
        } // finite_moving_average(...)

        explicit finite_moving_average(std::size_t window_size, const thresholds_type& thresholds)
            : base_type(thresholds)
        {
            this->initialize(window_size);
        } // finite_moving_average(...)

        explicit finite_moving_average(std::size_t window_size, thresholds_type&& thresholds)
            : base_type(std::forward<thresholds_type>(thresholds))
        {
            this->initialize(window_size);
        } // finite_moving_average(...)

        std::string name() const noexcept override
        {
            return ropufu::aftermath::format::cat(
                type::typename_string, " W", this->m_container.size()
            );
        } // name(...)

        friend void to_json(nlohmann::json& j, const type& x) noexcept
        {
            j = nlohmann::json{
                {type::jstr_typename, type::typename_string},
                {type::jstr_window_size, x.m_container.size()}
            };
            x.serialize_core(j);
        } // to_json(...)

        friend void from_json(const nlohmann::json& j, type& x)
        {
            if (!ropufu::noexcept_json::try_get(j, x))
                throw std::runtime_error("Parsing <finite_moving_average> failed: " + j.dump());
        } // from_json(...)

    protected:
        value_type update_statistic(const value_type& raw_value, const value_type& /*log_likelihood_ratio*/) noexcept override
        {
            this->m_newest_index = (this->m_newest_index + (this->m_container.size() - 1)) % this->m_container.size();
            this->m_container[this->m_newest_index] = raw_value;

            if (this->count_observations() < (this->m_container.size() - 1)) return 0;

            value_type sum = 0;
            for (value_type x : this->m_container) sum += x;
            return sum / static_cast<value_type>(this->m_container.size());
        } // update_statistic(...)

        std::vector<value_type> update_statistic(const std::vector<value_type>& raw_values, const std::vector<value_type>& /*log_likelihood_ratios*/) noexcept override
        {
            std::vector<value_type> result {};
            result.reserve(raw_values.size());

            std::size_t offset = 0;
            if (this->count_observations() < (this->m_container.size() - 1)) [[unlikely]]
                offset = (this->m_container.size() - 1) - this->count_observations();

            for (std::size_t k = 0; k < offset; ++k)
            {
                this->m_newest_index = (this->m_newest_index + (this->m_container.size() - 1)) % this->m_container.size();
                this->m_container[this->m_newest_index] = raw_values[k];
            } // for (...)

            for (std::size_t k = offset; k < raw_values.size(); ++k)
            {
                this->m_newest_index = (this->m_newest_index + (this->m_container.size() - 1)) % this->m_container.size();
                this->m_container[this->m_newest_index] = raw_values[k];

                value_type sum = 0;
                for (value_type x : this->m_container) sum += x;
                result.push_back(sum / static_cast<value_type>(this->m_container.size()));
            } // for (...)

            return result;
        } // update_statistic(...)

        void on_reset() noexcept override
        {
            for (value_type& x : this->m_container) x = 0;
            this->m_newest_index = 0;
        } // on_reset(...)

        nlohmann::json serialize() const noexcept override
        {
            nlohmann::json j = *this;
            return j;
        } // serialize(...)
    }; // struct finite_moving_average
} // namespace ropufu::sequential::intermittent

namespace ropufu
{
    template <std::totally_ordered t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::intermittent::finite_moving_average<t_value_type>>
    {
        using result_type = ropufu::sequential::intermittent::finite_moving_average<t_value_type>;
        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            std::string typename_string;
            std::size_t window_size;

            if (!noexcept_json::required(j, result_type::jstr_typename, typename_string)) return false;
            if (!noexcept_json::required(j, result_type::jstr_window_size, window_size)) return false;

            if (typename_string != result_type::typename_string) return false;
            if (window_size == 0) return false;

            if (!x.try_deserialize_core(j)) return false;
            x.initialize(window_size);
            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_INTERMITTENT_FINITE_MOVING_AVERAGE_HPP_INCLUDED
