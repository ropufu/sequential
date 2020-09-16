
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <ropufu/discrepancy.hpp>
#include <ropufu/number_traits.hpp>
#include <ropufu/sliding_array.hpp>

#include "white_noise.hpp"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

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

    /** Auto-regressive (AR) process over white Gaussian noise. */
    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size>
    struct auto_regressive_noise;

    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size>
    void to_json(nlohmann::json& j, const auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>& x) noexcept;
    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size>
    void from_json(const nlohmann::json& j, auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>& x);
    
    /** Auto-regressive (AR) process over white Gaussian noise. */
    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size>
    struct auto_regressive_noise
        : public detail::named_auto_regressive_noise<t_ar_size>
    {
        using type = auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using white_noise_type = white_noise<engine_type, value_type>;
        using probability_type = typename white_noise_type::probability_type;
        using expectation_type = typename white_noise_type::expectation_type;
        using distribution_type = typename white_noise_type::distribution_type;

        using ar_container_type = std::array<value_type, t_ar_size>;
        using time_window_type = aftermath::sliding_array<value_type, t_ar_size + 1>;

        static constexpr std::size_t ar_size = t_ar_size;

        // ~~ Json names ~~
        static constexpr std::string_view jstr_typename = "type";
        static constexpr std::string_view jstr_white = "white noise";
        static constexpr std::string_view jstr_ar_parameters = "AR parameters";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        white_noise_type m_white = {}; // White noise.
        ar_container_type m_ar_parameters = {}; // AR parameters.
        time_window_type m_history = {}; // Brief history of AR noise used to generate observations.
        value_type m_current_value = 0; // Latest observed value.

        std::optional<std::string> error_message() const noexcept
        {
            for (const value_type& x : this->m_ar_parameters)
            {
                if (!aftermath::is_finite(x)) return "AR parameters must be finite.";
            } // for (...)
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        /** Zero AR noise. */
        auto_regressive_noise() noexcept { }

        /** White Gaussian noise with standard deviation \p sigma. */
        explicit auto_regressive_noise(const white_noise_type& white) noexcept
            : m_white(white)
        {
        } // auto_regressive_noise(...)

        /** @brief General case of an AR process.
         *  @exception std::logic_error \p ar_parameters must lie inside a unit sphere.
         */
        auto_regressive_noise(const white_noise_type& white, const ar_container_type& ar_parameters)
            : m_white(white), m_ar_parameters(ar_parameters)
        {
            this->validate();
        } // auto_regressive_noise(...)
        
        /** @brief White noise driving the AR. */
        const white_noise_type& white() const noexcept { return this->m_white; }
        /** @brief White noise driving the AR. */
        void set_white(const white_noise_type& value) noexcept { this->m_white = value; }

        /** @brief AR parameters. */
        const ar_container_type& ar_parameters() const noexcept { return this->m_ar_parameters; }
        /** @brief AR parameters . */
        value_type ar_parameters(std::size_t time_lag_index) const { return this->m_ar_parameters.at(time_lag_index); }
        /** @brief AR parameters.
         *  @exception std::logic_error \p value must lie inside a unit sphere.
         */
        void set_ar_parameters(const ar_container_type& value)
        {
            this->m_ar_parameters = value;
            this->validate();
        } // set_ar_parameters(...)

        /** Resets the timer on the noise. */
        void reset() noexcept
        {
            this->m_white.reset();
            this->m_history.fill(0);
            this->m_current_value = 0;
        } // on_reset(...)

        /** Latest observed value. */
        value_type current_value() const noexcept { return this->m_current_value; }

        /** @brief Updates the current value of the noise. */
        void tic(engine_type& uniform_engine) noexcept
        {
            this->m_white.tic(uniform_engine);
            // ~~ Observations ~~
            value_type w = this->m_white.current_value(); // White noise.
            //               
            // ------|------|---...---|------|------> time 
            //     now-p        ... now-1   now            
            //       0      1         p     p+1    history 
            //       p     p-1        0      -    AR param 
            // 
            value_type v = w;
            for (std::size_t i = 0; i < type::ar_size; ++i) v += this->m_ar_parameters[i] * this->m_history[type::ar_size - i];

            this->m_history.push_back(v); // Keep track of recent AR noise.
            this->m_current_value = v;
        } // tic(...)

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_white == other.m_white &&
                this->m_ar_parameters == other.m_ar_parameters;
        } // operator ==(...)

        bool operator !=(const type& other) const noexcept
        {
            return !this->operator ==(other);
        } // operator !=(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct auto_regressive_noise

    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size>
    void to_json(nlohmann::json& j, const auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>& x) noexcept
    {
        using type = auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>;
        std::string typename_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, typename_str},
            {type::jstr_white, x.white()},
            {type::jstr_ar_parameters, x.ar_parameters()}
        };
    } // to_json(...)

    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size>
    void from_json(const nlohmann::json& j, auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <auto_regressive_noise> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>>
    {
        using engine_type = t_engine_type;
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            if (!noexcept_json::required(j, result_type::jstr_typename, typename_str)) return false;
            if (typename_str != result_type::typename_string) return false; // Noise type mismatch.

            // Parse json entries.
            if (!noexcept_json::optional(j, result_type::jstr_white, x.m_white)) return false;
            if (!noexcept_json::optional(j, result_type::jstr_ar_parameters, x.m_ar_parameters)) return false;

            // Validate entries.
            if (x.error_message().has_value()) return false;

            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

namespace ropufu::aftermath
{
    namespace detail
    {
        template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size>
        struct discrepancy<ropufu::sequential::hypotheses::auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>>
        {
            using result_type = t_value_type;
            using argument_type = ropufu::sequential::hypotheses::auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>;

            result_type operator ()(const argument_type& x, const argument_type& y) const noexcept
            {
                result_type total = 0;
                total += ropufu::aftermath::discrepancy(x.white(), y.white());
                total += ropufu::aftermath::discrepancy(x.ar_parameters(), y.ar_parameters());
                return total;
            } // operator ()(...)
        }; // struct discrepancy<...>
    } // namespace detail
} // namespace ropufu::aftermath

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED
