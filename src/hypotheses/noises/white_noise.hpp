
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_WHITE_NOISE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_WHITE_NOISE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../json.hpp"

#include <aftermath/not_an_error.hpp> // aftermath::quiet_error
#include <aftermath/probability.hpp>  // aftermath::probability::dist_normal
#include <aftermath/random.hpp>       // std::mt19937_64, std::seed_seq

#include "../noise_base.hpp"

#include <chrono>   // std::chrono::high_resolution_clock
#include <cmath>    // std::isnan, std::isinf
#include <cstddef>  // std::size_t
#include <cstdint>  // std::int32_t
#include <iostream> // std::ostream

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
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
                struct sampler_switch<aftermath::probability::dist_normal<t_result_type>, t_engine_type>
                {
                    using type = aftermath::random::default_sampler_normal_t<t_engine_type, t_result_type>;
                    using builtin_type = std::normal_distribution<t_result_type>; // Corresponding built-in C++ distribution type.
                }; // struct sampler_switch<...>
            } // namespace detail

            template <typename t_value_type>
            struct white_noise : public noise_base<white_noise<t_value_type>, t_value_type>
            {
                using type = white_noise<t_value_type>;
                using engine_type = std::mt19937;
                using noise_distribution_type = ropufu::aftermath::probability::dist_normal<t_value_type>; // For aftermath engine.
                // using noise_distribution_type = std::normal_distribution<t_value_type>; // For built-in c++ engine.
                using noise_sampler_type = typename detail::sampler_switch<noise_distribution_type, engine_type>::type;
                static constexpr char noise_type_name[] = "white";

                // ~~ Json names ~~
                static constexpr char jstr_noise_type[] = "type";
                static constexpr char jstr_noise_sigma[] = "noise sigma";

            private:
                using base_type = noise_base<type, t_value_type>;
                friend base_type;

                // ~~ Auxiliary members ~~
                engine_type m_engine = engine_type(); // Uniform PRNG.
                noise_sampler_type m_sampler = { }; // White noise sampler.

                // ~~ Structural members ~~
                value_type m_noise_sigma = 1; // Standard deviation of noise.
                value_type m_noise_variance = 1; // Variance deviation of noise.

                void coerce() noexcept
                {
                    if (std::isnan(this->m_noise_sigma) || std::isinf(this->m_noise_sigma)) this->m_noise_sigma = 0;
                    
                    if (this->m_noise_sigma <= 0)
                    {
                        this->m_noise_sigma = 1;
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "Noise standard deviation has to be poisitive. Coerced noise sigma to one.", __FUNCTION__, __LINE__);
                    }
                    this->m_noise_variance = this->m_noise_sigma * this->m_noise_sigma;
                } // coerce(...)

                void chrono_seed() noexcept
                {
                    auto now = std::chrono::high_resolution_clock::now();
                    std::seed_seq ss = { 1729, 875, 393, 19, static_cast<std::int32_t>(now.time_since_epoch().count()) };
                    this->m_engine.seed(ss);
                } // chrono_seed(...)

            protected:
                /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
                void on_reset_override() noexcept { }

                /** @brief Updates the current value of the noise. */
                value_type next_value(value_type /**current_value*/) noexcept
                {
                    return this->m_noise_sigma * this->m_sampler(this->m_engine); // White noise.
                } // next_value(...)

            public:
                /** No AR. */
                white_noise() noexcept : white_noise(1) { }

                /** No AR. */
                explicit white_noise(value_type noise_sigma) noexcept
                    : base_type(), m_noise_sigma(noise_sigma)
                {
                    this->coerce();
                    this->chrono_seed();
                } // white_noise(...)

                /** Standard deviation of noise. */
                value_type noise_sigma() const noexcept { return this->m_noise_sigma; }

                /** Variance of noise. */
                value_type noise_variance() const noexcept { return this->m_noise_variance; }
                
                /** Standard deviation of noise. */
                void set_noise_level(value_type value) noexcept
                {
                    this->m_noise_sigma = value;
                    this->coerce();
                } // set_noise_level(...)

                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct white_noise

            // ~~ Some definitions ~~
            template <typename t_value_type> constexpr char white_noise<t_value_type>::noise_type_name[];
            // ~~ Json name definitions ~~
            template <typename t_value_type> constexpr char white_noise<t_value_type>::jstr_noise_sigma[];

            template <typename t_value_type>
            void to_json(nlohmann::json& j, const white_noise<t_value_type>& x) noexcept
            {
                using type = white_noise<t_value_type>;
                std::string noise_type = type::noise_type_name;

                j = nlohmann::json{
                    {type::jstr_noise_type, noise_type},
                    {type::jstr_noise_sigma, x.noise_sigma()}
                };
            } // to_json(...)
        
            template <typename t_value_type>
            void from_json(const nlohmann::json& j, white_noise<t_value_type>& x) noexcept
            {
                quiet_json q(__FUNCTION__, __LINE__);
                using type = white_noise<t_value_type>;

                // Populate default values.
                std::string noise_type = type::noise_type_name;
                t_value_type noise_sigma = x.noise_sigma();

                // Parse json entries.
                if (!quiet_json::optional(j, type::jstr_noise_sigma, noise_sigma)) return;
                
                // Reconstruct the object.
                x.set_noise_level(noise_sigma);
                x.reset();

                q.validate();
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_WHITE_NOISE_HPP_INCLUDED
