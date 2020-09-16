
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_WHITE_NOISE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_WHITE_NOISE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <ropufu/discrepancy.hpp>
#include <ropufu/number_traits.hpp>
#include <ropufu/probability/standard_normal_distribution.hpp>
#include <ropufu/random/normal_sampler_512.hpp>

#include <iostream>    // std::ostream
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace ropufu::sequential::hypotheses
{
    /** White Gaussian noise. */
    template <typename t_engine_type, typename t_value_type>
    struct white_noise;

    template <typename t_engine_type, typename t_value_type>
    void to_json(nlohmann::json& j, const white_noise<t_engine_type, t_value_type>& x) noexcept;
    template <typename t_engine_type, typename t_value_type>
    void from_json(const nlohmann::json& j, white_noise<t_engine_type, t_value_type>& x);

    /** White Gaussian noise. */
    template <typename t_engine_type, typename t_value_type>
    struct white_noise
    {
        using type = white_noise<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using sampler_type = aftermath::random::normal_sampler_512<engine_type, value_type>;
        using probability_type = typename sampler_type::probability_type;
        using expectation_type = typename sampler_type::expectation_type;
        using distribution_type = typename sampler_type::distribution_type;

        static constexpr char typename_string[] = "gaussian";

        // ~~ Json names ~~
        static constexpr std::string_view jstr_typename = "type";
        static constexpr std::string_view jstr_sigma = "sigma";

        friend ropufu::noexcept_json_serializer<type>;

    private:
        value_type m_sigma = 0; // Standard deviation.
        sampler_type m_sampler = {}; // White noise sampler.
        value_type m_current_value = 0; // Latest observed value.

        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_finite(this->m_sigma) || this->m_sigma < 0) return "Sigma must be positive or zero.";
            return std::nullopt;
        } // error_message(...)

        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)
        
    public:
        /** @brief Zero Gaussian noise. */
        white_noise() noexcept
        {
        } // white_noise(...)

        /** @brief White Gaussian noise.
         *  @exception std::logic_error \p value must be positive or zero.
         */
        explicit white_noise(value_type sigma)
            : m_sigma(sigma)
        {
            this->validate();
        } // white_noise(...)

        /** @brief Standard deviation of the noise. */
        value_type sigma() const noexcept { return this->m_sigma; }
        /** @brief Standard deviation of the noise. */
        value_type standard_deviation() const noexcept { return this->m_sigma; }
        /** @brief Variance of the noise. */
        value_type variance() const noexcept { return this->m_sigma * this->m_sigma; }
        /** @brief Standard deviation of the noise.
         *  @exception std::logic_error \p value must be positive or zero.
         */
        void set_sigma(value_type value)
        {
            this->m_sigma = value;
            this->validate();
        } // set_sigma(...)

        /** Resets the timer on the noise. */
        void reset() noexcept { this->m_current_value = 0; }

        /** Latest observed value. */
        value_type current_value() const noexcept { return this->m_current_value; }

        void tic(engine_type& uniform_engine) noexcept
        {
            this->m_current_value = this->m_sigma * this->m_sampler(uniform_engine);
        } // tic(...)

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_sigma == other.m_sigma;
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
    }; // struct white_noise

    // ~~ Definitions ~~
    template <typename t_engine_type, typename t_value_type> constexpr char white_noise<t_engine_type, t_value_type>::typename_string[];

    template <typename t_engine_type, typename t_value_type>
    void to_json(nlohmann::json& j, const white_noise<t_engine_type, t_value_type>& x) noexcept
    {
        using type = white_noise<t_engine_type, t_value_type>;
        std::string typename_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, typename_str},
            {type::jstr_sigma, x.sigma()}
        };
    } // to_json(...)

    template <typename t_engine_type, typename t_value_type>
    void from_json(const nlohmann::json& j, white_noise<t_engine_type, t_value_type>& x)
    {
        if (!noexcept_json::try_get(j, x)) throw std::runtime_error("Parsing <white_noise> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

namespace ropufu
{
    template <typename t_engine_type, typename t_value_type>
    struct noexcept_json_serializer<ropufu::sequential::hypotheses::white_noise<t_engine_type, t_value_type>>
    {
        using engine_type = t_engine_type;
        using value_type = t_value_type;
        using result_type = ropufu::sequential::hypotheses::white_noise<t_engine_type, t_value_type>;

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            // Ensure correct type.
            std::string typename_str {};
            if (!noexcept_json::required(j, result_type::jstr_typename, typename_str)) return false;
            if (typename_str != result_type::typename_string) return false; // Noise type mismatch.

            // Parse json entries.
            if (!noexcept_json::optional(j, result_type::jstr_sigma, x.m_sigma)) return false;
            
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
        template <typename t_engine_type, typename t_value_type>
        struct discrepancy<ropufu::sequential::hypotheses::white_noise<t_engine_type, t_value_type>>
        {
            using result_type = t_value_type;
            using argument_type = ropufu::sequential::hypotheses::white_noise<t_engine_type, t_value_type>;

            result_type operator ()(const argument_type& x, const argument_type& y) const noexcept
            {
                result_type total = 0;
                total += ropufu::aftermath::discrepancy(x.sigma(), y.sigma());
                return total;
            } // operator ()(...)
        }; // struct discrepancy<...>
    } // namespace detail
} // namespace ropufu::aftermath

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_WHITE_NOISE_HPP_INCLUDED
