
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/number_traits.hpp>

#include "../../draft/ropufu/sliding_array.hpp"
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
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_white[] = "white noise";
        static constexpr char jstr_ar_parameters[] = "AR parameters";

    private:
        white_noise_type m_white = {}; // White noise.
        ar_container_type m_ar_parameters = {}; // AR parameters.
        time_window_type m_history = {}; // Brief history of AR noise used to generate observations.
        value_type m_current_value = 0; // Latest observed value.

        static bool is_valid(const ar_container_type& ar_parameters, std::string& message) noexcept
        {
            value_type sum_squared = 0;
            for (const value_type& x : ar_parameters)
            {
                if (!aftermath::is_finite(x))
                {
                    message = "AR parameters must be finite.";
                    return false;
                } // if (...)
                sum_squared += (x * x);
            } // for (...)
            if (!aftermath::is_finite(sum_squared) || sum_squared >= 1)
            {
                message = "AR parameters must lie inside a unit sphere.";
                return false;
            } // if (...)
            return true;
        } // validate(...)

        void validate() const
        {
            std::string message {};
            if (!type::is_valid(this->m_ar_parameters, message))
                throw std::logic_error(message);
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
        
        auto_regressive_noise(const nlohmann::json& j, std::error_code& ec) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                ec = std::make_error_code(std::errc::bad_message); // Noise type mismatch.
                return;
            } // if (...)

            // Parse json entries.
            white_noise_type white = this->m_white;
            ar_container_type ar_parameters = this->m_ar_parameters;
            aftermath::noexcept_json::optional(j, type::jstr_white, white, ec);
            aftermath::noexcept_json::optional(j, type::jstr_ar_parameters, ar_parameters, ec);
            if (ec.value() != 0) return;

            // Validate entries.
            std::string message {};
            if (!type::is_valid(ar_parameters, message))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
            
            // Populate values.
            this->m_white = white;
            this->m_ar_parameters = ar_parameters;
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

    // ~~ Json name definitions ~~
    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>::jstr_typename[];
    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>::jstr_white[];
    template <typename t_engine_type, typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>::jstr_ar_parameters[];

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
        using type = auto_regressive_noise<t_engine_type, t_value_type, t_ar_size>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <auto_regressive_noise> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED
