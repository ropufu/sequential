
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_WHITE_NOISE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_WHITE_NOISE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/on_error.hpp>    // aftermath::detail::on_error
#include <ropufu/probability.hpp> // aftermath::probability::normal_distribution
#include <ropufu/random.hpp>      // aftermath::random::default_sampler_normal_t

#include "../noise_base.hpp"

#include <chrono>   // std::chrono::high_resolution_clock
#include <cmath>    // std::isnan, std::isinf
#include <cstddef>  // std::size_t
#include <cstdint>  // std::int32_t
#include <iostream> // std::ostream
#include <random>   // std::mt19937, std::seed_seq
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        template <typename t_distribution_type, typename t_engine_type>
        struct sampler_switch
        {
            using type = t_distribution_type; // Built-in C++ distributions are also samplers.
            using builtin_type = t_distribution_type; // Corresponding built-in C++ distribution type.
        }; // struct sampler_switch
        
        template <typename t_result_type, typename t_engine_type>
        struct sampler_switch<aftermath::probability::normal_distribution<t_result_type>, t_engine_type>
        {
            using type = aftermath::random::normal_sampler<t_engine_type, t_result_type>;
            using builtin_type = std::normal_distribution<t_result_type>; // Corresponding built-in C++ distribution type.
        }; // struct sampler_switch<...>
    } // namespace detail
    
    template <typename t_value_type>
    struct white_noise;
    template <typename t_value_type>
    void to_json(nlohmann::json& j, const white_noise<t_value_type>& x) noexcept;
    template <typename t_value_type>
    void from_json(const nlohmann::json& j, white_noise<t_value_type>& x);

    template <typename t_value_type>
    struct white_noise : public noise_base<white_noise<t_value_type>, t_value_type>
    {
        using type = white_noise<t_value_type>;
        using base_type = noise_base<type, t_value_type>;
        friend base_type;

        using timed_type = typename base_type::timed_type;
        using noise_base_type = typename base_type::noise_base_type;
        using value_type = typename base_type::value_type;
        using engine_type = std::mt19937;
        using noise_distribution_type = ropufu::aftermath::probability::normal_distribution<t_value_type>; // For aftermath engine.
        // using noise_distribution_type = std::normal_distribution<t_value_type>; // For built-in c++ engine.
        using noise_sampler_type = typename detail::sampler_switch<noise_distribution_type, engine_type>::type;
        static constexpr char typename_string[] = "white";

        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_noise_sigma[] = "noise sigma";

    private:
        // ~~ Auxiliary members ~~
        engine_type m_engine = engine_type(); // Uniform PRNG.
        noise_sampler_type m_sampler = {}; // White noise sampler.

        // ~~ Structural members ~~
        value_type m_noise_sigma = 1; // Standard deviation of noise.
        value_type m_noise_variance = 1; // Variance deviation of noise.

        void chrono_seed() noexcept
        {
            auto now = std::chrono::high_resolution_clock::now();
            std::seed_seq ss = { 1729, 875, 393, 19, static_cast<std::int32_t>(now.time_since_epoch().count()) };
            this->m_engine.seed(ss);
        } // chrono_seed(...)

    protected:
        bool validate(std::error_code& ec) const noexcept
        {
            if (std::isnan(this->m_noise_sigma) || std::isinf(this->m_noise_sigma)) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Noise standard deviation has to be a finite number.", false);
            if (this->m_noise_sigma <= 0) return aftermath::detail::on_error(ec, std::errc::invalid_argument, "Noise standard deviation has to be poisitive.", false);
            return true;
        } // validate(...)

        void coerce() noexcept
        {
            if (std::isnan(this->m_noise_sigma) || std::isinf(this->m_noise_sigma)) this->m_noise_sigma = 0;
            if (this->m_noise_sigma <= 0) this->m_noise_sigma = 1;
            this->m_noise_variance = this->m_noise_sigma * this->m_noise_sigma;
        } // coerce(...)

        /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
        void on_reset_override() noexcept { }

        /** @brief Updates the current value of the noise. */
        value_type next_value(value_type /*current_value*/) noexcept
        {
            return this->m_noise_sigma * this->m_sampler(this->m_engine); // White noise.
        } // next_value(...)
        
    public:
        white_noise() noexcept : base_type() { }

        /** No AR. */
        white_noise(value_type noise_sigma, std::error_code& ec) noexcept
            : base_type(), m_noise_sigma(noise_sigma)
        {
            if (!this->validate(ec)) this->coerce();
            this->chrono_seed();
        } // white_noise(...)

        white_noise(const nlohmann::json& j, std::error_code& ec) noexcept
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
            aftermath::noexcept_json::optional(j, type::jstr_noise_sigma, this->m_noise_sigma, ec);
            
            if (!this->validate(ec)) this->coerce();
        } // white_noise(...)

        /** Standard deviation of noise. */
        value_type noise_sigma() const noexcept { return this->m_noise_sigma; }

        /** Variance of noise. */
        value_type noise_variance() const noexcept { return this->m_noise_variance; }
        
        /** Standard deviation of noise. */
        void set_noise_level(value_type value, std::error_code& ec) noexcept
        {
            this->m_noise_sigma = value;
            if (!this->validate(ec)) this->coerce();
        } // set_noise_level(...)

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct white_noise

    // ~~ Some definitions ~~
    template <typename t_value_type> constexpr char white_noise<t_value_type>::typename_string[];
    // ~~ Json name definitions ~~
    template <typename t_value_type> constexpr char white_noise<t_value_type>::jstr_noise_sigma[];

    template <typename t_value_type>
    void to_json(nlohmann::json& j, const white_noise<t_value_type>& x) noexcept
    {
        using type = white_noise<t_value_type>;
        std::string typename_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, typename_str},
            {type::jstr_noise_sigma, x.noise_sigma()}
        };
    } // to_json(...)

    template <typename t_value_type>
    void from_json(const nlohmann::json& j, white_noise<t_value_type>& x)
    {
        using type = white_noise<t_value_type>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_WHITE_NOISE_HPP_INCLUDED
