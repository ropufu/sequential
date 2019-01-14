
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/on_error.hpp> // aftermath::detail::on_error
#include "../../draft/algebra/numbers.hpp"

#include "../noise_base.hpp"
#include "../sliding_array.hpp"
#include "white_noise.hpp"

#include <array>    // std::array
#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc
#include <vector>   // std::vector

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        template <std::size_t... t_digits>
        struct auto_regressive_noise_chars
        {
            static constexpr char typename_string[] = { 'A', 'R', ' ', ('0' + t_digits)..., 0};
        }; // struct auto_regressive_noise_chars
        template <> struct auto_regressive_noise_chars<> { static constexpr char typename_string[] = "AR 0"; };
        template <std::size_t... t_digits> constexpr char auto_regressive_noise_chars<t_digits...>::typename_string[];

        template <std::size_t t_remainder, std::size_t... t_digits>
        struct named_auto_regressive_noise : public named_auto_regressive_noise<t_remainder / 10, t_remainder % 10, t_digits...> { };

        template <std::size_t... t_digits>
        struct named_auto_regressive_noise<0, t_digits...> : public auto_regressive_noise_chars<t_digits...> { };
    } // namespace detail

    /** Represents a descriptor for auto-regressive (AR) process. */
    template <typename t_value_type, std::size_t t_ar_size = 0>
    struct auto_regressive_noise;
    
    /** Trivial case of AR process: white noise. */
    template <typename t_value_type>
    struct auto_regressive_noise<t_value_type, 0> : public white_noise<t_value_type>
    {
        using type = auto_regressive_noise<t_value_type, 0>;
        using value_type = t_value_type;
        using ar_container_type = std::array<value_type, 0>;
        using time_window_type = sliding_array<value_type, 0>;

        using base_type = white_noise<value_type>;

        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_noise_sigma[] = "noise sigma";
        static constexpr char jstr_ar_parameters[] = "AR parameters";

    private:
        // ~~ Structural members ~~
        ar_container_type m_ar_parameters = {}; // AR parameters.

    public:
        auto_regressive_noise() noexcept : base_type() { }

        auto_regressive_noise(value_type noise_sigma, std::error_code& ec) noexcept
            : base_type(noise_sigma, ec)
        {
        } // auto_regressive_noise(...)

        /** AR with parameters specified by \p parameters. */
        auto_regressive_noise(value_type noise_sigma, const ar_container_type& /*ar_parameters*/, std::error_code& ec) noexcept
            : base_type(noise_sigma, ec)/*, m_ar_parameters(ar_parameters)*/
        {
        } // auto_regressive_noise(...)
        
        auto_regressive_noise(const nlohmann::json& j, std::error_code& ec) noexcept
            : base_type()
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                aftermath::detail::on_error(ec, std::errc::invalid_argument, "Noise type mismatch.");
                return;
            } // if (...)

            // Parse json entries.
            value_type noise_sigma {};
            aftermath::noexcept_json::optional(j, type::jstr_noise_sigma, noise_sigma, ec);
            aftermath::noexcept_json::optional(j, type::jstr_ar_parameters, this->m_ar_parameters, ec);
            this->set_noise_level(noise_sigma, ec);

            if (!this->validate(ec)) this->coerce();
        } // auto_regressive_noise(...)

        /** AR parameter size. */
        constexpr std::size_t ar_size() const noexcept { return 0; }

        /** AR parameters. */
        const ar_container_type& ar_parameters() const noexcept { return this->m_ar_parameters; }
        /** AR parameters. */
        constexpr value_type ar_parameter(std::size_t /*time_lag_index*/) const noexcept { return 0; }
        /** AR parameters. */
        constexpr void set_ar_parameter(std::size_t /*time_lag_index*/, value_type /*value*/, std::error_code& /*ec*/) noexcept { }

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct auto_regressive_noise<...>

    /** Represents a descriptor for auto-regressive (AR) process. */
    template <typename t_value_type, std::size_t t_ar_size>
    struct auto_regressive_noise
        : public noise_base<auto_regressive_noise<t_value_type, t_ar_size>, t_value_type>,
        public detail::named_auto_regressive_noise<t_ar_size>
    {
        using type = auto_regressive_noise<t_value_type, t_ar_size>;
        using value_type = t_value_type;
        using ar_container_type = std::array<value_type, t_ar_size>;
        using time_window_type = sliding_array<value_type, t_ar_size>;

        using base_type = noise_base<type, value_type>;
        friend base_type;

        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_noise_sigma[] = "noise sigma";
        static constexpr char jstr_ar_parameters[] = "AR parameters";

    private:
        // ~~ Structural members ~~
        white_noise<value_type> m_white_noise = {}; // White noise.
        ar_container_type m_ar_parameters = {}; // AR parameters.
        time_window_type m_history = {}; // Brief history of AR noise used to generate observations.

    protected:
        bool validate(std::error_code& ec) const noexcept
        {
            value_type sum_squared = 0;
            for (const value_type& x : this->m_ar_parameters)
            {
                if (modules::is_nan(x) || modules::is_infinite(x)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "AR parameters have to be finite numbers.", false);
                sum_squared += (x * x);
            } // for (...)
            if (sum_squared >= 1) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "AR parameters have to lie inside a unit sphere.", false);
            return true;
        } // validate(...)

        void coerce() noexcept
        {
            value_type sum_squared = 0;
            for (value_type& x : this->m_ar_parameters)
            {
                if (modules::is_nan(x) || modules::is_infinite(x)) x = 0;
                sum_squared += (x * x);
            } // for (...)
            if (sum_squared >= 1) this->m_ar_parameters.fill(0);
        } // coerce(...)

    protected:
        /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
        void on_reset() noexcept
        {
            this->m_white_noise.reset();
            this->m_history.fill(0);
        } // on_reset(...)

        /** @brief Updates the current value of the noise. */
        value_type next_value(value_type /*current_value*/) noexcept
        {
            this->m_white_noise.tic();
            // ~~ Observations ~~
            value_type w = this->m_white_noise.current_value(); // White noise.
            //               
            // ------|------|---...---|------|------> time 
            //     now-p        ... now-1   now            
            //       0      1         p     p+1    history 
            //       p     p-1        0      -    AR param 
            // 
            value_type v = w;
            for (std::size_t i = 0; i < t_ar_size; ++i) v += this->m_ar_parameters[i] * this->m_history[t_ar_size - i];

            this->m_history.push_back(v); // Keep track of recent AR noise.
            return v;
        } // next_value(...)

    public:
        /** No AR. */
        auto_regressive_noise() noexcept : base_type() { }

        /** No AR. */
        auto_regressive_noise(value_type noise_sigma, std::error_code& ec) noexcept
            : base_type(), m_white_noise(noise_sigma, ec)
        {
        } // auto_regressive_noise(...)

        /** AR with parameters specified by \p parameters. */
        auto_regressive_noise(value_type noise_sigma, const ar_container_type& ar_parameters, std::error_code& ec) noexcept
            : base_type(), m_white_noise(noise_sigma, ec), m_ar_parameters(ar_parameters)
        {
            if (!this->validate(ec)) this->coerce();
        } // auto_regressive_noise(...)
        
        auto_regressive_noise(const nlohmann::json& j, std::error_code& ec) noexcept
            : base_type()
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                aftermath::detail::on_error(ec, std::errc::invalid_argument, "Noise type mismatch.");
                return;
            } // if (...)

            // Parse json entries.
            value_type noise_sigma {};
            aftermath::noexcept_json::optional(j, type::jstr_noise_sigma, noise_sigma, ec);
            aftermath::noexcept_json::optional(j, type::jstr_ar_parameters, this->m_ar_parameters, ec);
            this->m_white_noise.set_noise_level(noise_sigma, ec);

            if (!this->validate(ec)) this->coerce();
        } // auto_regressive_noise(...)

        /** Standard deviation of noise. */
        value_type noise_sigma() const noexcept { return this->m_white_noise.noise_sigma(); }

        /** Variance of noise. */
        value_type noise_variance() const noexcept { return this->m_white_noise.noise_variance(); }
        
        /** Standard deviation of noise. */
        void set_noise_level(value_type value, std::error_code& ec) noexcept { this->m_white_noise.set_noise_level(value, ec); }

        /** AR parameter size. */
        constexpr std::size_t ar_size() const noexcept { return t_ar_size; }

        /** AR parameters. */
        const ar_container_type& ar_parameters() const noexcept { return this->m_ar_parameters; }
        /** AR parameters. */
        value_type ar_parameter(std::size_t time_lag_index) const noexcept { return this->m_ar_parameters[time_lag_index]; }
        /** AR parameters. */
        void set_ar_parameter(std::size_t time_lag_index, value_type value, std::error_code& ec) noexcept
        {
            this->m_ar_parameters[time_lag_index] = value;
            if (!this->validate(ec)) this->coerce();
        } // set_ar_parameter(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct auto_regressive_noise

    // ~~ Json name definitions ~~
    template <typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_value_type, t_ar_size>::jstr_typename[];
    template <typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_value_type, t_ar_size>::jstr_noise_sigma[];
    template <typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_value_type, t_ar_size>::jstr_ar_parameters[];

    template <typename t_value_type, std::size_t t_ar_size>
    void to_json(nlohmann::json& j, const auto_regressive_noise<t_value_type, t_ar_size>& x) noexcept
    {
        using type = auto_regressive_noise<t_value_type, t_ar_size>;
        std::string typename_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, typename_str},
            {type::jstr_noise_sigma, x.noise_sigma()},
            {type::jstr_ar_parameters, x.ar_parameters()}
        };
    } // to_json(...)

    template <typename t_value_type, std::size_t t_ar_size>
    void from_json(const nlohmann::json& j, auto_regressive_noise<t_value_type, t_ar_size>& x)
    {
        using type = auto_regressive_noise<t_value_type, t_ar_size>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED
