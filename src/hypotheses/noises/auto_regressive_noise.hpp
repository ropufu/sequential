
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../../draft/quiet_json.hpp"

#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include "../noise_base.hpp"
#include "../sliding_array.hpp"
#include "white_noise.hpp"

#include <array>    // std::array
#include <cmath>    // std::isnan, std::isinf
#include <cstddef>  // std::size_t
#include <iostream> // std::ostream
#include <vector>   // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                template <std::size_t... t_digits>
                struct auto_regressive_noise_chars
                {
                    static constexpr char noise_type_name[] = { 'A', 'R', ' ', ('0' + t_digits)..., 0};
                };
                template <> struct auto_regressive_noise_chars<> { static constexpr char signal_type_name[] = "AR 0"; };
                template <std::size_t... t_digits> constexpr char auto_regressive_noise_chars<t_digits...>::noise_type_name[];

                template <std::size_t t_remainder, std::size_t... t_digits>
                struct named_auto_regressive_noise : public named_auto_regressive_noise<t_remainder / 10, t_remainder % 10, t_digits...> { };

                template <std::size_t... t_digits>
                struct named_auto_regressive_noise<0, t_digits...> : public auto_regressive_noise_chars<t_digits...> { };
            } // namespace detail

            /** Represents a descriptor for auto-regressive (AR) process. */
            template <typename t_value_type, std::size_t t_ar_size = 0>
            struct auto_regressive_noise;
            
            /** Represents a descriptor for auto-regressive (AR) process. */
            template <typename t_value_type>
            struct auto_regressive_noise<t_value_type, 0> : public white_noise<t_value_type>
            {
                using type = auto_regressive_noise<t_value_type, 0>;
                using base_type = white_noise<t_value_type>;

                using timed_type = typename base_type::timed_type;
                using noise_base_type = typename base_type::noise_base_type;
                using value_type = typename base_type::value_type;
                using ar_container_type = std::array<t_value_type, 0>;
                using time_window_type = sliding_array<t_value_type, 0>;

                // ~~ Json names ~~
                static constexpr char jstr_noise_type[] = "type";
                static constexpr char jstr_noise_sigma[] = "noise sigma";
                static constexpr char jstr_ar_parameters[] = "AR parameters";

            private:
                // ~~ Structural members ~~
                ar_container_type m_ar_parameters = { }; // AR parameters.

            public:
                // std::ostringstream& mat_prefix(std::ostringstream& os) const noexcept
                // {
                //     if (this->m_ar_parameters.empty()) return os;
                //     os << "_ar";
                //     for (value_type rho : this->m_regression.ar_parameters()) os << "_" << rho;
                //     return os;
                // } // mat_prefix(...)

                auto_regressive_noise() noexcept : base_type() { }

                auto_regressive_noise(value_type noise_sigma) noexcept : base_type(noise_sigma) { }

                /** AR with parameters specified by \p parameters. */
                auto_regressive_noise(value_type noise_sigma, const ar_container_type& /**ar_parameters*/) noexcept
                    : base_type(noise_sigma)/**, m_ar_parameters(ar_parameters)*/
                {
                } // auto_regressive_noise(...)

                /** AR parameter size. */
                constexpr std::size_t ar_size() const noexcept { return 0; }

                /** AR parameters. */
                const ar_container_type& ar_parameters() const noexcept { return this->m_ar_parameters; }

                /** AR parameters. */
                constexpr void set_ar_parameters(const ar_container_type& /**value*/) noexcept { }

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
                using base_type = noise_base<type, t_value_type>;
                friend base_type;

                using timed_type = typename base_type::timed_type;
                using noise_base_type = typename base_type::noise_base_type;
                using value_type = typename base_type::value_type;
                using ar_container_type = std::array<t_value_type, t_ar_size>;
                using time_window_type = sliding_array<t_value_type, t_ar_size>;

                // ~~ Json names ~~
                static constexpr char jstr_noise_type[] = "type";
                static constexpr char jstr_noise_sigma[] = "noise sigma";
                static constexpr char jstr_ar_parameters[] = "AR parameters";

            private:
                // ~~ Structural members ~~
                white_noise<value_type> m_white_noise = { }; // White noise.
                ar_container_type m_ar_parameters = { }; // AR parameters.
                time_window_type m_history = { }; // Brief history of AR noise used to generate observations.

                void coerce() noexcept
                {
                    value_type sum_squared = 0;
                    for (value_type& rho : this->m_ar_parameters)
                    {
                        if (std::isnan(rho) || std::isinf(rho))
                        {
                            rho = 0;
                            aftermath::quiet_error::instance().push(
                                aftermath::not_an_error::logic_error,
                                aftermath::severity_level::major,
                                "AR parameters has to be finite. Coerced to zero.", __FUNCTION__, __LINE__);
                        }
                        sum_squared += (rho * rho);
                    }
                    if (sum_squared >= 1)
                    {
                        this->m_ar_parameters.fill(0);
                        aftermath::quiet_error::instance().push(
                            aftermath::not_an_error::logic_error,
                            aftermath::severity_level::major,
                            "AR parameters have to lie inside a unit sphere. Coerced to no AR.", __FUNCTION__, __LINE__);
                    }
                } // coerce(...)

            protected:
                /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
                void on_reset_override() noexcept
                {
                    this->m_white_noise.reset();
                    this->m_history.fill(0);
                } // on_reset(...)

                /** @brief Updates the current value of the noise. */
                value_type next_value(value_type /**current_value*/) noexcept
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
                // std::ostringstream& mat_prefix(std::ostringstream& os) const noexcept
                // {
                //     if (this->m_ar_parameters.empty()) return os;
                //     os << "_ar";
                //     for (value_type rho : this->m_regression.ar_parameters()) os << "_" << rho;
                //     return os;
                // } // mat_prefix(...)

                /** No AR. */
                auto_regressive_noise() noexcept : auto_regressive_noise(1, { }) { }

                /** No AR. */
                explicit auto_regressive_noise(value_type noise_sigma) noexcept : auto_regressive_noise(noise_sigma, { }) { }

                /** AR with parameters specified by \p parameters. */
                auto_regressive_noise(value_type noise_sigma, const ar_container_type& ar_parameters) noexcept
                    : base_type(), m_white_noise(noise_sigma), m_ar_parameters(ar_parameters)
                {
                    this->coerce();
                } // auto_regressive_noise(...)

                /** Standard deviation of noise. */
                value_type noise_sigma() const noexcept { return this->m_white_noise.noise_sigma(); }

                /** Variance of noise. */
                value_type noise_variance() const noexcept { return this->m_white_noise.noise_variance(); }
                
                /** Standard deviation of noise. */
                void set_noise_level(value_type value) noexcept { this->m_white_noise.set_noise_level(value); }

                /** AR parameter size. */
                constexpr std::size_t ar_size() const noexcept { return t_ar_size; }

                /** AR parameters. */
                const ar_container_type& ar_parameters() const noexcept { return this->m_ar_parameters; }

                /** AR parameters. */
                void set_ar_parameters(const ar_container_type& value) noexcept { this->m_ar_parameters = value; }

                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct auto_regressive_noise

            // ~~ Json name definitions ~~
            template <typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_value_type, t_ar_size>::jstr_noise_type[];
            template <typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_value_type, t_ar_size>::jstr_noise_sigma[];
            template <typename t_value_type, std::size_t t_ar_size> constexpr char auto_regressive_noise<t_value_type, t_ar_size>::jstr_ar_parameters[];

            template <typename t_value_type, std::size_t t_ar_size>
            void to_json(nlohmann::json& j, const auto_regressive_noise<t_value_type, t_ar_size>& x) noexcept
            {
                using type = auto_regressive_noise<t_value_type, t_ar_size>;
                std::string noise_type_str = type::noise_type_name;

                j = nlohmann::json{
                    {type::jstr_noise_type, noise_type_str},
                    {type::jstr_noise_sigma, x.noise_sigma()},
                    {type::jstr_ar_parameters, x.ar_parameters()}
                };
            } // to_json(...)
        
            template <typename t_value_type, std::size_t t_ar_size>
            void from_json(const nlohmann::json& j, auto_regressive_noise<t_value_type, t_ar_size>& x) noexcept
            {
                quiet_json q(j);
                using type = auto_regressive_noise<t_value_type, t_ar_size>;

                // Populate default values.
                std::string noise_type_str = type::noise_type_name;
                typename type::value_type noise_sigma = x.noise_sigma();
                typename type::ar_container_type ar_parameters = x.ar_parameters();

                // Parse json entries.
                q.required(type::jstr_noise_type, noise_type_str);
                q.optional(type::jstr_noise_sigma, noise_sigma);
                q.optional(type::jstr_ar_parameters, ar_parameters);
                
                // Reconstruct the object.
                if (!q.good())
                {
                    aftermath::quiet_error::instance().push(
                        aftermath::not_an_error::runtime_error,
                        aftermath::severity_level::major,
                        q.message(), __FUNCTION__, __LINE__);
                    return;
                } // if (...)
                x.set_noise_level(noise_sigma);
                x.set_ar_parameters(ar_parameters);
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_AUTO_REGRESSIVE_NOISE_HPP_INCLUDED
