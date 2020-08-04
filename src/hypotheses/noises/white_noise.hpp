
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_WHITE_NOISE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_NOISES_WHITE_NOISE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <ropufu/discrepancy.hpp>
#include <ropufu/number_traits.hpp>
#include <ropufu/probability/standard_normal_distribution.hpp>
#include <ropufu/random/normal_sampler_512.hpp>

#include <iostream>  // std::ostream
#include <stdexcept> // std::runtime_error, std::logic_error
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc

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
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_sigma[] = "sigma";

    private:
        value_type m_sigma = 0; // Standard deviation.
        sampler_type m_sampler = {}; // White noise sampler.
        value_type m_current_value = 0; // Latest observed value.

        static bool is_valid(value_type sigma, std::string& message) noexcept
        {
            if (!aftermath::is_finite(sigma) || sigma < 0)
            {
                message = "Sigma must be positive or zero.";
                return false;
            } // if (...)
            return true;
        } // validate(...)

        void validate() const
        {
            std::string message {};
            if (!type::is_valid(this->m_sigma, message))
                throw std::logic_error(message);
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

        /** @brief White Gaussian noise. */
        white_noise(const nlohmann::json& j, std::error_code& ec) noexcept
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
            value_type sigma = this->m_sigma;
            aftermath::noexcept_json::optional(j, type::jstr_sigma, sigma, ec);
            if (ec.value() != 0) return;

            // Validate entries.
            std::string message {};
            if (!type::is_valid(sigma, message))
            {
                ec = std::make_error_code(std::errc::bad_message);
                return;
            } // if (...)
            
            // Populate values.
            this->m_sigma = sigma;
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

    // ~~ Some definitions ~~
    template <typename t_engine_type, typename t_value_type> constexpr char white_noise<t_engine_type, t_value_type>::typename_string[];
    // ~~ Json name definitions ~~
    template <typename t_engine_type, typename t_value_type> constexpr char white_noise<t_engine_type, t_value_type>::jstr_typename[];
    template <typename t_engine_type, typename t_value_type> constexpr char white_noise<t_engine_type, t_value_type>::jstr_sigma[];

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
        using type = white_noise<t_engine_type, t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing <white_noise> failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

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
