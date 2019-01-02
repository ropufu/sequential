
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_DOUBLE_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_DOUBLE_SPRT_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

#include <ropufu/on_error.hpp>

#include "../core.hpp"
#include "../model.hpp"
#include "../process.hpp"
#include "../two_sprt.hpp"

#include <cstddef> // std::size_t
#include <iostream> // std::ostream
#include <stdexcept>    // std::runtime_error
#include <string>   // std::string
#include <system_error> // std::error_code, std::errc
#include <vector>  // std::vector

namespace ropufu::sequential::hypotheses
{
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check = true>
    struct double_sprt;

    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void to_json(nlohmann::json& j, const double_sprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept;
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void from_json(const nlohmann::json& j, double_sprt<t_signal_type, t_noise_type, t_sync_check>& x);

    template <typename t_process_type, bool t_sync_check = true>
    using double_sprt_t = double_sprt<typename t_process_type::signal_type, typename t_process_type::noise_type, t_sync_check>;

    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    struct double_sprt : public two_sprt<
        double_sprt<t_signal_type, t_noise_type, t_sync_check>,
        t_signal_type, t_noise_type, t_sync_check>
    {
        using type = double_sprt<t_signal_type, t_noise_type, t_sync_check>;
        using base_type = two_sprt<type, t_signal_type, t_noise_type, t_sync_check>;
        friend base_type;

        using signal_type = typename base_type::signal_type;
        using noise_type = typename base_type::noise_type;
        using process_type = typename base_type::process_type;
        using value_type = typename base_type::value_type;
        using model_type = typename base_type::model_type;
        using likelihood_type = typename base_type::likelihood_type;
        using statistic_type = typename base_type::statistic_type;
        
        static constexpr char typename_string[] = "double sprt";
        
        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_id[] = "id";
        static constexpr char jstr_relative_mu_intermediate[] = "relative mu intermediate";

    private:
        // ~~ Fundamental members ~~
        value_type m_relative_mu_intermediate = static_cast<value_type>(0.5); // Relative threshold used to measure distance from.
        value_type m_mu_intermediate = static_cast<value_type>(0.5); // Threshold used to measure distance from.

        // ~~ Members reset with each \c toc() ~~
        value_type m_unscaled_distance_from_null = 0; // Latest (unscaled) LLR vs. null estimator.
        value_type m_unscaled_distance_from_alt = 0;  // Latest (unscaled) LLR vs. alt estimator.

    protected:
        /** @brief Indicates if the choice of thresholds does not affect other design parameters. */
        bool is_design_threshold_independent() const noexcept { return true; }

        /** @brief Auxiliary function to be executed right after the \c initialize() call. */
        void on_initialized() noexcept
        {
            this->m_mu_intermediate = this->likelihood().model().mu_relative(this->m_relative_mu_intermediate);
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

            this->m_unscaled_distance_from_null = proc.unscaled_log_likelihood_between(this->m_mu_intermediate, null_mu);
            this->m_unscaled_distance_from_alt = proc.unscaled_log_likelihood_between(this->m_mu_intermediate, alt_mu);
        } // on_tic_override(...)

        /** @brief Auxiliary function to be executed right before the \c on_toc() call. */
        void on_toc_override(const process_type& /*proc*/) noexcept
        {
            this->on_reset_override();
        } // on_toc_override(...)

    public:
        double_sprt() noexcept : base_type() { }

        explicit double_sprt(std::size_t id, value_type relative_mu_intermediate) noexcept
            : base_type(id), m_relative_mu_intermediate(relative_mu_intermediate)
        {
        } // double_sprt(...)

        template <typename t_other_signal_type, typename t_other_noise_type, bool t_other_sync_check>
        /*implicit*/ double_sprt(const double_sprt<t_other_signal_type, t_other_noise_type, t_other_sync_check>& similar) noexcept
            : base_type(similar.id()), m_relative_mu_intermediate(static_cast<value_type>(similar.relative_mu_intermediate()))
        {
        } // double_sprt(...)
        
        double_sprt(const nlohmann::json& j, std::error_code& ec) noexcept
            : base_type()
        {
            // Ensure correct type.
            std::string typename_str {};
            aftermath::noexcept_json::required(j, type::jstr_typename, typename_str, ec);
            if (typename_str != type::typename_string)
            {
                aftermath::detail::on_error(ec, std::errc::invalid_argument, "SPRT type mismatch.");
                return;
            } // if (...)

            // Parse json entries.
            std::size_t id = this->id();
            aftermath::noexcept_json::required(j, type::jstr_id, id, ec);
            aftermath::noexcept_json::required(j, type::jstr_relative_mu_intermediate, this->m_relative_mu_intermediate, ec);
            this->set_id(id);

            // @todo Add validation/coercion.
            // if (!this->validate(ec)) this->coerce();
        } // double_sprt(...)
        
        value_type relative_mu_intermediate() const noexcept { return this->m_relative_mu_intermediate; }

        std::string to_path_string(std::size_t decimal_places = 3) const noexcept
        {
            std::string result = type::typename_string;
            result += " intermediate ";
            result += detail::to_str(this->m_relative_mu_intermediate, decimal_places);
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
    }; // struct double_sprt

    // ~~ Definitions ~~
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char double_sprt<t_signal_type, t_noise_type, t_sync_check>::typename_string[];

    // ~~ Json name definitions ~~
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char double_sprt<t_signal_type, t_noise_type, t_sync_check>::jstr_typename[];
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char double_sprt<t_signal_type, t_noise_type, t_sync_check>::jstr_id[];
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char double_sprt<t_signal_type, t_noise_type, t_sync_check>::jstr_relative_mu_intermediate[];
    
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void to_json(nlohmann::json& j, const double_sprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
    {
        using type = double_sprt<t_signal_type, t_noise_type, t_sync_check>;
        std::string sprt_type_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, sprt_type_str},
            {type::jstr_id, x.id()},
            {type::jstr_relative_mu_intermediate, x.relative_mu_intermediate()}
        };
    } // to_json(...)

    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void from_json(const nlohmann::json& j, double_sprt<t_signal_type, t_noise_type, t_sync_check>& x)
    {
        using type = double_sprt<t_signal_type, t_noise_type, t_sync_check>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_DOUBLE_SPRT_HPP_INCLUDED
