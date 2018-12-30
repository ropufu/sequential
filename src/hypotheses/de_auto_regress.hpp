
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_DE_AUTO_REGRESS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_DE_AUTO_REGRESS_HPP_INCLUDED

#include "signals.hpp"
#include "noises.hpp"
#include "process.hpp"

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <system_error> // std::error_code, std::errc

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            namespace detail
            {
                /** @todo Default behavior should not be identity, but rather indicate that it was not implemented.*/
                template <typename t_signal_type, typename t_noise_type>
                struct de_auto_regress
                {
                    using signal_type = t_signal_type;
                    using noise_type = t_noise_type;

                    using adjusted_signal_type = t_signal_type;
                    using adjusted_noise_type = t_noise_type;

                    static constexpr adjusted_noise_type adjust_noise(const signal_type& /*signal*/, const noise_type& noise, std::error_code& /*ec*/) noexcept { return noise; }
                    
                    static constexpr adjusted_signal_type adjust_signal(const signal_type& signal, const noise_type& /*noise*/, std::error_code& /*ec*/) noexcept { return signal; }
                }; // struct de_auto_regress

                template <typename t_value_type>
                struct de_auto_regress<constant_signal<t_value_type>, auto_regressive_noise<t_value_type, 0>>
                {
                    using signal_type = constant_signal<t_value_type>;
                    using noise_type = auto_regressive_noise<t_value_type, 0>;

                    using adjusted_signal_type = constant_signal<t_value_type>;
                    using adjusted_noise_type = white_noise<t_value_type>;

                    static adjusted_noise_type adjust_noise(const signal_type& /*signal*/, const noise_type& noise, std::error_code& ec) noexcept { return adjusted_noise_type(noise.noise_sigma(), ec); }
                    
                    static constexpr adjusted_signal_type adjust_signal(const signal_type& signal, const noise_type& /*noise*/, std::error_code& /*ec*/) noexcept { return signal; }
                }; // struct de_auto_regress

                template <typename t_value_type, std::size_t t_ar_size>
                struct de_auto_regress<constant_signal<t_value_type>, auto_regressive_noise<t_value_type, t_ar_size>>
                {
                    using signal_type = constant_signal<t_value_type>;
                    using noise_type = auto_regressive_noise<t_value_type, t_ar_size>;

                    using adjusted_signal_type = transitionary_signal<t_value_type, t_ar_size>;
                    using adjusted_noise_type = white_noise<t_value_type>;

                    static adjusted_noise_type adjust_noise(const signal_type& /*signal*/, const noise_type& noise, std::error_code& ec) noexcept { return adjusted_noise_type(noise.noise_sigma(), ec); }
                    
                    static adjusted_signal_type adjust_signal(const signal_type& signal, const noise_type& noise, std::error_code& ec) noexcept
                    {
                        t_value_type s = signal.level(); // Stationary level.
                        const std::array<t_value_type, t_ar_size>& rho = noise.ar_parameters(); // AR parameters.
                        std::array<t_value_type, t_ar_size> transition {};
                        
                        for (std::size_t i = 0; i < t_ar_size; ++i)
                        {
                            transition[i] = s;
                            s -= rho[i] * signal.level();
                        }
                        return adjusted_signal_type(s, transition, ec);
                    } // adjust_signal(...)
                    
                    // /** @brief Adjust the \p sequence to compensate for AR noise.
                    //  *  @remark \tparam t_sequence_type has to implement operator [std::size_t] -> double.
                    //  */
                    // template <typename t_sequence_type>
                    // double adjust(const t_sequence_type& sequence, std::size_t time_index) const noexcept
                    // {
                    //     const std::vector<double>& rho = this->m_ar_parameters; // Just to make code more readable.
                    //     double s = sequence[time_index];
                        
                    //     //
                    //     //  ...-------- AR elements --->|                 
                    //     //       |<---- t elements ---->|                 
                    //     // ------|----|----|--...--|----|----|-----> time 
                    //     //       0    1    2  ... t-2  t-1   t            
                    //     //
                    //     std::size_t count = rho.size(); // How many elements should we take from the past.
                    //     if (count > time_index) count = time_index; // Take at most <time_index> observations from the past.

                    //     for (std::size_t i = 0; i < count; i++) s -= rho[i] * sequence[time_index - 1 - i];
                    //     return s;
                    // }

                    // /** @brief Build AR process from an i.i.d. sequence.
                    //  *  @param latest The latest observation from the i.i.d. sequence.
                    //  *  @param sequence The AR sequence.
                    //  *  @remark \tparam t_sequence_type has to implement operator [std::size_t] -> double.
                    //  */
                    // template <typename t_sequence_type>
                    // double auto_regress(double latest, const t_sequence_type& sequence, std::size_t time_index) const noexcept
                    // {
                    //     const std::vector<double>& rho = this->m_ar_parameters; // Just to make code more readable.
                    //     double s = latest;
                        
                    //     //
                    //     //  ...-------- AR elements --->|                 
                    //     //       |<---- t elements ---->|                 
                    //     // ------|----|----|--...--|----|----|-----> time 
                    //     //       0    1    2  ... t-2  t-1   t            
                    //     //
                    //     std::size_t count = rho.size(); // How many elements should we take from the past.
                    //     if (count > time_index) count = time_index; // Take at most <time_index> observations from the past.

                    //     for (std::size_t i = 0; i < count; i++) s += rho[i] * sequence[time_index - 1 - i];
                    //     return s;
                    // }
                }; // struct de_auto_regress

                template <typename t_signal_type, typename t_noise_type>
                using adjusted_process_t = process<
                    typename de_auto_regress<t_signal_type, t_noise_type>::adjusted_signal_type,
                    typename de_auto_regress<t_signal_type, t_noise_type>::adjusted_noise_type>;
            } // namespace detail

            template <typename t_signal_type, typename t_noise_type>
            auto adjust_process(const process<t_signal_type, t_noise_type>& proc, std::error_code& ec) noexcept -> detail::adjusted_process_t<t_signal_type, t_noise_type>
            {
                /*using signal_type = t_signal_type;*/
                /*using noise_type = t_noise_type;*/
                /*using process_type = process<t_signal_type, t_noise_type>;*/

                using de_auto_regress_type = detail::de_auto_regress<t_signal_type, t_noise_type>;

                using adjusted_process_type = detail::adjusted_process_t<t_signal_type, t_noise_type>;
                using adjusted_signal_type = typename adjusted_process_type::signal_type;
                using adjusted_noise_type = typename adjusted_process_type::noise_type;

                adjusted_signal_type signal = de_auto_regress_type::adjust_signal(proc.signal(), proc.noise(), ec);
                adjusted_noise_type noise = de_auto_regress_type::adjust_noise(proc.signal(), proc.noise(), ec);
                return adjusted_process_type(signal, noise, proc.actual_mu());
            }; // adjust_process(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_DE_AUTO_REGRESS_HPP_INCLUDED
