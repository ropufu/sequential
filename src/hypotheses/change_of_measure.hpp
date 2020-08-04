
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_CHANGE_OF_MEASURE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_CHANGE_OF_MEASURE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../draft/format.hpp"
#include "model.hpp"
#include "operating_characteristic.hpp"

#include <cstddef>    // std::size_t
#include <functional> // std::hash
#include <iostream>   // std::ostream
#include <stdexcept>  // std::invalid_argument
#include <string>     // std::string, std::to_string
#include <system_error> // std::error_code, std::errc
#include <vector>     // std::vector

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
        static constexpr char jstr_analyzed[] = "analyzed";
        static constexpr char jstr_simulated[] = "simulated";

    private:
        value_type m_analyzed = {};
        value_type m_simulated = {};

    public:
        change_of_measure() noexcept { }

        change_of_measure(value_type analyzed, value_type simulated)
            : m_analyzed(analyzed), m_simulated(simulated)
        {
            if (!aftermath::is_finite(analyzed)) throw std::logic_error("Analyzed mu must be finite.");
            if (!aftermath::is_finite(simulated)) throw std::logic_error("Simulated mu must be finite.");
        } // change_of_measure(...)

        change_of_measure(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            if (j.is_array())
            {
                std::vector<value_type> pair {};
                aftermath::noexcept_json::as(j, pair, ec);
                if (ec.value() != 0) return;
                if (pair.size() != 2) // Range should be a vector with two entries.
                {
                    ec = std::make_error_code(std::errc::bad_message);
                    return;
                } // if (...)
                this->m_analyzed = pair.front();
                this->m_simulated = pair.back();
            } // if (...)
            else
            {
                // Parse json entries.
                value_type analyzed = this->m_analyzed;
                value_type simulated = this->m_simulated;
                aftermath::noexcept_json::required(j, type::jstr_analyzed, analyzed, ec);
                aftermath::noexcept_json::required(j, type::jstr_simulated, simulated, ec);
                if (ec.value() != 0) return;

                // Validate entries.
                if (!aftermath::is_finite(analyzed)) ec = std::make_error_code(std::errc::bad_message);
                if (!aftermath::is_finite(simulated)) ec = std::make_error_code(std::errc::bad_message);
                if (ec.value() != 0) return;

                // Populate values.
                this->m_analyzed = analyzed;
                this->m_simulated = simulated;
            } // else (...)
        } // hypothesis_pair(...)

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

    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char change_of_measure<t_value_type>::jstr_analyzed[];
    template <typename t_value_type> constexpr char change_of_measure<t_value_type>::jstr_simulated[];
    
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
        using type = change_of_measure<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <change_of_measure> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

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
