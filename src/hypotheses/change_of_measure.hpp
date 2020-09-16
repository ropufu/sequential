
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_CHANGE_OF_MEASURE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_CHANGE_OF_MEASURE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../draft/format.hpp"
#include "model.hpp"
#include "operating_characteristic.hpp"

#include <cstddef>     // std::size_t
#include <functional>  // std::hash
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    struct change_of_measure;

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const change_of_measure<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, change_of_measure<t_value_type>& x);

    template <typename t_value_type>
    struct change_of_measure
    {
        using type = change_of_measure<t_value_type>;
        using value_type = t_value_type;
        using model_type = hypotheses::model<t_value_type>;

        // ~~ Json names ~~
        static constexpr std::string_view jstr_analyzed = "analyzed";
        static constexpr std::string_view jstr_simulated = "simulated";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        value_type m_analyzed = {};
        value_type m_simulated = {};

        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_finite(this->m_analyzed)) return "Analyzed mu must be finite.";
            if (!aftermath::is_finite(this->m_simulated)) return "Simulated mu must be finite.";
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        change_of_measure() noexcept { }

        change_of_measure(value_type analyzed, value_type simulated)
            : m_analyzed(analyzed), m_simulated(simulated)
        {
            this->validate();
        } // change_of_measure(...)

        static type from_oc(operating_characteristic oc, const model_type& model)
        {
            switch (oc)
            {
                case operating_characteristic::ess_under_null:
                    return {
                        model.mu_under_null(),
                        model.mu_under_null()};
                case operating_characteristic::ess_under_alt:
                    return {
                        model.smallest_mu_under_alt(),
                        model.smallest_mu_under_alt()};
                case operating_characteristic::probability_of_false_alarm:
                    return {
                        model.mu_under_null(),
                        model.smallest_mu_under_alt()};
                case operating_characteristic::probability_of_missed_signal:
                    return {
                        model.smallest_mu_under_alt(),
                        model.mu_under_null()};
                default:
                    throw std::invalid_argument("OC not recognized.");
            } // switch (...)
        } // change_of_measure(...)

        /** Signal strength conrresponding to what measure we want to analyze. */
        value_type analyzed() const noexcept { return this->m_analyzed; }

        /** Signal strength conrresponding to what measure is used to generate observations. */
        value_type simulated() const noexcept { return this->m_simulated; }

        bool is_identity() const noexcept { return this->m_analyzed == this->m_simulated; }
        
        std::string to_path_string(std::size_t decimal_places = 3) const noexcept
        {
            std::string result = "analyze ";
            result += ropufu::draft::detail::to_str(this->m_analyzed, decimal_places);
            result += " simulate ";
            result += ropufu::draft::detail::to_str(this->m_simulated, decimal_places);
            return result;
        } // to_path_string(...)

        /** Checks if the two objects are equal. */
        bool operator ==(const type& other) const noexcept
        {
            return
                //this->m_oc == other.m_oc &&
                this->m_analyzed == other.m_analyzed &&
                this->m_simulated == other.m_simulated;
        } // operator ==(...)

        /** Checks if the two objects are not equal. */
        bool operator !=(const type& other) const noexcept { return !this->operator ==(other); }

        /** @brief Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct change_of_measure
    
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const change_of_measure<t_value_type>& x) noexcept
    {
        using type = change_of_measure<t_value_type>;

        j = nlohmann::json{
            {type::jstr_analyzed, x.analyzed()},
            {type::jstr_simulated, x.simulated()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, change_of_measure<t_value_type>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <change_of_measure> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::change_of_measure<t_value_type>>
    {
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::change_of_measure<t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            if (j.is_array())
            {
                std::vector<value_type> pair {};
                if (!noexcept_json::try_get(j, pair)) return false;
                if (pair.size() != 2) return false; // Range should be a vector with two entries.

                x.m_analyzed = pair.front();
                x.m_simulated = pair.back();
            } // if (...)
            else
            {
                // Parse json entries.
                if (!noexcept_json::required(j, result_type::jstr_analyzed, x.m_analyzed)) return false;
                if (!noexcept_json::required(j, result_type::jstr_simulated, x.m_simulated)) return false;
            } // if (...)
            
            // Validate entries.
            if (x.error_message().has_value()) return false;

            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

namespace std
{
    template <typename t_value_type>
    struct hash<ropufu::sequential::hypotheses::change_of_measure<t_value_type>>
    {
        using argument_type = ropufu::sequential::hypotheses::change_of_measure<t_value_type>;
        using result_type = std::size_t;

        result_type operator ()(argument_type const& x) const noexcept
        {
            std::hash<typename argument_type::value_type> value_hash = {};
            return
                (value_hash(x.analyzed()) << 4) ^ 
                (value_hash(x.simulated()));
        } // operator ()(...)
    }; // struct hash<...>
} // namespace std

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_CHANGE_OF_MEASURE_HPP_INCLUDED
