
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "../json.hpp"

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
            struct generalized_sprt;

            template <typename t_process_type, bool t_sync_check = true>
            using generalized_sprt_t = generalized_sprt<typename t_process_type::signal_type, typename t_process_type::noise_type, t_sync_check>;

            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            struct generalized_sprt : public two_sprt<generalized_sprt<t_signal_type, t_noise_type, t_sync_check>, t_signal_type, t_noise_type, t_sync_check>
            {
                using type = generalized_sprt<t_signal_type, t_noise_type, t_sync_check>;
                using base_type = two_sprt<type, t_signal_type, t_noise_type, t_sync_check>;
                friend base_type;

                using signal_type = typename base_type::signal_type;
                using noise_type = typename base_type::noise_type;
                using process_type = typename base_type::process_type;
                using value_type = typename base_type::value_type;
                using model_type = typename base_type::model_type;
                using likelihood_type = typename base_type::likelihood_type;
                using statistic_type = typename base_type::statistic_type;
                
                static constexpr char sprt_type_name[] = "generalized sprt";
                
                // ~~ Json names ~~
                static constexpr char jstr_sprt_type[] = "type";
                static constexpr char jstr_id[] = "id";

            private:
                // ~~ Members reset with each \c toc() ~~
                value_type m_unscaled_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator.
                value_type m_unscaled_distance_from_alt = 0;  // Latest (unscaled) LLR vs. alt estimator.

            protected:
                /** @brief Auxiliary function to be executed right after the \c initialize() call. */
                void on_initialized() noexcept
                {
                    this->on_reset_override();
                } // on_initialized(...)

                /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
                void on_reset_override() noexcept
                {
                    this->m_unscaled_distance_from_null = 0;
                    this->m_unscaled_distance_from_alt = 0;
                } // on_reset_override(...)

                /** @brief Auxiliary function to be executed right after the \c on_tic() call. */
                void on_tic_override(const process_type& proc) noexcept
                {
                    value_type null_mu = this->likelihood().model().mu_under_null();
                    value_type alt_mu = this->likelihood().model().smallest_mu_under_alt();
                    value_type mu_null_hat = this->likelihood().null_estimator_of_mu().back();
                    value_type mu_alt_hat = (mu_null_hat < alt_mu) ? alt_mu : mu_null_hat;
                    
                    this->m_unscaled_distance_from_null = proc.unscaled_log_likelihood_between(mu_null_hat, null_mu);
                    this->m_unscaled_distance_from_alt = proc.unscaled_log_likelihood_between(mu_null_hat, mu_alt_hat);
                } // on_tic_override(...)

                /** @brief Auxiliary function to be executed right before the \c on_toc() call. */
                void on_toc_override(const process_type& /**proc*/) noexcept
                {
                    this->on_reset_override();
                } // on_toc_override(...)

            public:
                generalized_sprt() noexcept : base_type() { }

                explicit generalized_sprt(std::size_t id) noexcept : base_type(id) { }

                template <typename t_other_signal_type, typename t_other_noise_type, bool t_other_sync_check>
                /**implicit*/ generalized_sprt(const generalized_sprt<t_other_signal_type, t_other_noise_type, t_other_sync_check>& similar) noexcept : base_type(similar.id()) { }

                std::string to_path_string(std::size_t /**decimal_places*/) const noexcept
                {
                    std::string result = type::sprt_type_name;
                    return result;
                } // to_path_string(...)

                bool do_decide_null(value_type threshold) const noexcept { return this->m_unscaled_distance_from_alt > threshold; }
                bool do_decide_alt(value_type threshold) const noexcept { return this->m_unscaled_distance_from_null > threshold; }

                /** Output to a stream. */
                friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
                {
                    nlohmann::json j = self;
                    return os << j;
                } // operator <<(...)
            }; // struct generalized_sprt

            // ~~ Definitions ~~
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char generalized_sprt<t_signal_type, t_noise_type, t_sync_check>::sprt_type_name[];

            // ~~ Json name definitions ~~
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char generalized_sprt<t_signal_type, t_noise_type, t_sync_check>::jstr_sprt_type[];
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char generalized_sprt<t_signal_type, t_noise_type, t_sync_check>::jstr_id[];
            
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            void to_json(nlohmann::json& j, const generalized_sprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
            {
                using type = generalized_sprt<t_signal_type, t_noise_type, t_sync_check>;
                std::string sprt_type_str = type::sprt_type_name;

                j = nlohmann::json{
                    {type::jstr_sprt_type, sprt_type_str},
                    {type::jstr_id, x.id()}
                };
            } // to_json(...)
        
            template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
            void from_json(const nlohmann::json& j, generalized_sprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
            {
                quiet_json q(__FUNCTION__, __LINE__);
                using type = generalized_sprt<t_signal_type, t_noise_type, t_sync_check>;

                // Populate default values.
                std::string sprt_type_str = type::sprt_type_name;
                std::size_t id = x.id();

                // Parse json entries.
                if (!quiet_json::required(j, type::jstr_sprt_type, sprt_type_str)) return;
                if (!quiet_json::required(j, type::jstr_id, id)) return;
                
                // Reconstruct the object.
                x = type(id);

                q.validate();
            } // from_json(...)
        } // namespace hypotheses
    } // namespace sequential
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_GENERALIZED_SPRT_HPP_INCLUDED
