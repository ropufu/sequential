
#ifndef ROPUFU_SEQUENTIAL_INTERMITTENT_CONFIG_9B676DD5124D43F9A002A4872F58C7B7
#define ROPUFU_SEQUENTIAL_INTERMITTENT_CONFIG_9B676DD5124D43F9A002A4872F58C7B7

//=======================================================================
// This file was generated automatically.
// Please do not edit it directly, since any changes may be overwritten.
//=======================================================================
// UTC Date: 6/26/2021 3:01:00 PM
//=======================================================================

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <concepts>    // std::same_as
#include <cstddef>     // std::size_t
#include <functional>  // std::hash
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace ropufu::sequential::intermittent
{
    /** Tells which stopping times to simulate. */
    struct config;

    void to_json(nlohmann::json& j, const config& x) noexcept;
    void from_json(const nlohmann::json& j, config& x);

    struct config
    {
        using type = config;

        // ~~ Field typedefs ~~
        using simulations_type = std::size_t;
        using stopping_times_type = std::vector<nlohmann::json>;

        // ~~ Json keys ~~
        static constexpr std::string_view jstr_simulations = "simulations";
        static constexpr std::string_view jstr_stopping_times = "stopping times";

        friend ropufu::noexcept_json_serializer<type>;
        friend std::hash<type>;

    protected:
        simulations_type m_simulations = 0;
        stopping_times_type m_stopping_times = {}; // Rules to run.

        /** @brief Validates the structure and returns an error message, if any. */
        std::optional<std::string> error_message() const noexcept
        {
            const type& self = *this;

            if (self.m_simulations < 0) return "Number of simulations cannot be negative.";
            return std::nullopt;
        } // error_message(...)

    public:
        /** @exception std::logic_error Validation failed. */
        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

        config()
        {
            this->validate();
        } // config(...)

        config(simulations_type simulations, const stopping_times_type& stopping_times)
            : m_simulations(simulations),
            m_stopping_times(stopping_times)
        {
            this->validate();
        } // config(...)

        simulations_type simulations() const noexcept { return this->m_simulations; }

        void set_simulations(simulations_type value)
        {
            this->m_simulations = value;
            this->validate();
        } // set_simulations(...)

        const stopping_times_type& stopping_times() const noexcept { return this->m_stopping_times; }

        void set_stopping_times(const stopping_times_type& value) noexcept { this->m_stopping_times = value; }

        /** Checks if this object is equivalent to \param other. */
        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_simulations == other.m_simulations &&
                this->m_stopping_times == other.m_stopping_times;
        } // operator ==(...)

        /** Checks if this object is not equivalent to \param other. */
        bool operator !=(const type& other) const noexcept { return !this->operator ==(other); }

        friend void to_json(nlohmann::json& j, const type& x) noexcept
        {
            j = nlohmann::json{
                {type::jstr_simulations, x.m_simulations},
                {type::jstr_stopping_times, x.m_stopping_times}
            };
        } // to_json(...)

        friend void from_json(const nlohmann::json& j, type& x)
        {
            if (!ropufu::noexcept_json::try_get(j, x))
                throw std::runtime_error("Parsing <config> failed: " + j.dump());
        } // from_json(...)
    }; // struct config
} // namespace ropufu::sequential::intermittent

namespace ropufu
{
    template <>
    struct noexcept_json_serializer<ropufu::sequential::intermittent::config>
    {
        using result_type = ropufu::sequential::intermittent::config;
        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            if (!noexcept_json::required(j, result_type::jstr_simulations, x.m_simulations)) return false;
            if (!noexcept_json::required(j, result_type::jstr_stopping_times, x.m_stopping_times)) return false;

            if (x.error_message().has_value()) return false;
            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_INTERMITTENT_CONFIG_9B676DD5124D43F9A002A4872F58C7B7
