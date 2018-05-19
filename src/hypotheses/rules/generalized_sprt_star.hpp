
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_STAR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_STAR_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <aftermath/quiet_json.hpp>

#include <aftermath/not_an_error.hpp>

#include "../core.hpp"
#include "../model.hpp"
#include "../process.hpp"
#include "../two_sprt.hpp"

#include <cstddef> // std::size_t
#include <iostream> // std::ostream
#include <string>  // std::string
#include <vector>  // std::vector

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check = true>
            struct generalized_sprt_star;

            template <typename t_process_type, bool t_sync_check = true>
            using generalized_sprt_star_t = generalized_sprt_star<typename t_process_type::signal_type, typename t_process_type::noise_type, t_sync_check>;

            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            struct generalized_sprt_star : public two_sprt<generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>, t_signal_type, t_noise_type, t_sync_check>
            {
                using type = generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>;
                using base_type = two_sprt<type, t_signal_type, t_noise_type, t_sync_check>;
                friend base_type;

                using signal_type = typename base_type::signal_type;
                using noise_type = typename base_type::noise_type;
                using process_type = typename base_type::process_type;
                using value_type = typename base_type::value_type;
                using model_type = typename base_type::model_type;
                using likelihood_type = typename base_type::likelihood_type;
                using statistic_type = typename base_type::statistic_type;
                
                static constexpr char sprt_type_name[] = "generalized sprt star";
                
                // ~~ Json names ~~
                static constexpr char jstr_sprt_type[] = "type";
                static constexpr char jstr_id[] = "id";
                static constexpr char jstr_relative_mu_cutoff[] = "relative mu cutoff";

            private:
                // ~~ Fundamental members ~~
                value_type m_relative_mu_cutoff = static_cast<value_type>(0.5); // Relative threshold used to decide in favor of either of the hypotheses.
                value_type m_mu_cutoff = static_cast<value_type>(0.5); // Threshold used to decide in favor of either of the hypotheses.
                
                // ~~ Members reset with each \c toc() ~~
                value_type m_unscaled_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator.
                value_type m_unscaled_distance_from_alt = 0;  // Latest (unscaled) LLR vs. alt estimator.
                bool m_is_estimator_low = false; // Indicator if the latest estimator of mu is below the threshold.
                bool m_is_estimator_high = false; // Indicator if the latest estimator of mu is above the threshold.

            protected:
                /** @brief Auxiliary function to be executed right after the \c initialize() call. */
                void on_initialized() noexcept
                {
                    this->m_mu_cutoff = this->likelihood().model().mu_relative(this->m_relative_mu_cutoff);
                    this->on_reset_override();
                } // on_initialized(...)

                /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
                void on_reset_override() noexcept
                {
                    this->m_unscaled_distance_from_null = 0;
                    this->m_unscaled_distance_from_alt = 0;
                    this->m_is_estimator_low = false;
                    this->m_is_estimator_high = false;
                } // on_reset_override(...)

                /** @brief Auxiliary function to be executed right after the \c on_tic() call. */
                void on_tic_override(const process_type& proc) noexcept
                {
                    value_type null_mu = this->likelihood().model().mu_under_null();
                    value_type alt_mu = this->likelihood().model().smallest_mu_under_alt();
                    value_type mu_null_hat = this->likelihood().null_estimator_of_mu().back();
                    //value_type mu_alt_hat = (mu_null_hat < alt_mu) ? alt_mu : mu_null_hat;
                    
                    this->m_unscaled_distance_from_null = proc.unscaled_log_likelihood_between(mu_null_hat, null_mu);
                    this->m_unscaled_distance_from_alt = proc.unscaled_log_likelihood_between(mu_null_hat, alt_mu);
                    this->m_is_estimator_low = (mu_null_hat <= this->m_mu_cutoff);
                    this->m_is_estimator_high = (mu_null_hat >= this->m_mu_cutoff);
                } // on_tic_override(...)

                /** @brief Auxiliary function to be executed right before the \c on_toc() call. */
                void on_toc_override(const process_type& /**proc*/) noexcept
                {
                    this->on_reset_override();
                } // on_toc_override(...)

            public:
                generalized_sprt_star() noexcept : base_type() { }

                explicit generalized_sprt_star(std::size_t id, value_type relative_mu_cutoff) noexcept
                    : base_type(id), m_relative_mu_cutoff(relative_mu_cutoff)
                {
                } // generalized_sprt_star(...)

                template <typename t_other_signal_type, typename t_other_noise_type, bool t_other_sync_check>
                /**implicit*/ generalized_sprt_star(const generalized_sprt_star<t_other_signal_type, t_other_noise_type, t_other_sync_check>& similar) noexcept
                    : base_type(similar.id()), m_relative_mu_cutoff(static_cast<value_type>(similar.relative_mu_cutoff()))
                {
                } // generalized_sprt_star(...)
                
                value_type relative_mu_cutoff() const noexcept { return this->m_relative_mu_cutoff; }

                std::string to_path_string(std::size_t decimal_places = 3) const noexcept
                {
                    std::string result = type::sprt_type_name;
                    result += " cutoff ";
                    result += detail::to_str(this->m_relative_mu_cutoff, decimal_places);
                    return result;
                } // to_path_string(...)

                bool do_decide_null(value_type threshold) const noexcept { return this->m_is_estimator_low && this->m_unscaled_distance_from_alt > threshold; }
                bool do_decide_alt(value_type threshold) const noexcept { return this->m_is_estimator_high && this->m_unscaled_distance_from_null > threshold; }

                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct generalized_sprt_star

            // ~~ Definitions ~~
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>::sprt_type_name[];

            // ~~ Json name definitions ~~
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>::jstr_sprt_type[];
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>::jstr_id[];
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>::jstr_relative_mu_cutoff[];
            
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            void to_json(nlohmann::json& j, const generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
            {
                using type = generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>;
                std::string sprt_type_str = type::sprt_type_name;

                j = nlohmann::json{
                    {type::jstr_sprt_type, sprt_type_str},
                    {type::jstr_id, x.id()},
                    {type::jstr_relative_mu_cutoff, x.relative_mu_cutoff()}
                };
            } // to_json(...)
        
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            void from_json(const nlohmann::json& j, generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
            {
                aftermath::quiet_json q(j);
                using type = generalized_sprt_star<t_signal_type, t_noise_type, t_sync_check>;

                // Populate default values.
                std::string sprt_type_str = type::sprt_type_name;
                std::size_t id = x.id();
                typename type::value_type relative_mu_cutoff = x.relative_mu_cutoff();

                // Parse json entries.
                q.required(type::jstr_sprt_type, sprt_type_str);
                q.required(type::jstr_id, id);
                q.required(type::jstr_relative_mu_cutoff, relative_mu_cutoff);
                
                // Reconstruct the object.
                if (!q.good())
                {
                    aftermath::quiet_error::instance().push(
                        aftermath::not_an_error::runtime_error,
                        aftermath::severity_level::major,
                        q.message(), __FUNCTION__, __LINE__);
                    return;
                } // if (...)
                x = type(id, relative_mu_cutoff);
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_STAR_HPP_INCLUDED
