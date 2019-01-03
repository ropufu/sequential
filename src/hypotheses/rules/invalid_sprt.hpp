
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_INVALID_SPRT_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_INVALID_SPRT_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/json_traits.hpp>

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
    struct invalid_sprt;

    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void to_json(nlohmann::json& j, const invalid_sprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept;
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void from_json(const nlohmann::json& j, invalid_sprt<t_signal_type, t_noise_type, t_sync_check>& x);

    template <typename t_process_type, bool t_sync_check = true>
    using invalid_sprt_t = invalid_sprt<typename t_process_type::signal_type, typename t_process_type::noise_type, t_sync_check>;

    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    struct invalid_sprt : public two_sprt<
        invalid_sprt<t_signal_type, t_noise_type, t_sync_check>,
        t_signal_type, t_noise_type, t_sync_check>
    {
        using type = invalid_sprt<t_signal_type, t_noise_type, t_sync_check>;
        using base_type = two_sprt<type, t_signal_type, t_noise_type, t_sync_check>;
        friend base_type;

        using signal_type = typename base_type::signal_type;
        using noise_type = typename base_type::noise_type;
        using process_type = typename base_type::process_type;
        using value_type = typename base_type::value_type;
        using model_type = typename base_type::model_type;
        using likelihood_type = typename base_type::likelihood_type;
        using moment_statistic_type = typename base_type::moment_statistic_type;
        
        static constexpr char typename_string[] = "invalid sprt";

        // ~~ Json names ~~
        static constexpr char jstr_typename[] = "type";
        static constexpr char jstr_id[] = "id";

    private:
        std::string m_typename = "??";

    protected:
        /** @brief Indicates if the choice of thresholds does not affect other design parameters. */
        bool is_design_threshold_independent() const noexcept { return true; }

        /** @brief Auxiliary function to be executed right after the \c initialize() call. */
        void on_initialized() noexcept { }

        /** @brief Auxiliary function to be executed right before the \c on_reset() call. */
        void on_reset_override() noexcept { }

        /** @brief Auxiliary function to be executed right after the \c on_tic() call. */
        void on_tic_override(const process_type& /*proc*/) noexcept { }

        /** @brief Auxiliary function to be executed right before the \c on_toc() call. */
        void on_toc_override(const process_type& /*proc*/) noexcept { }

    public:
        invalid_sprt() noexcept : base_type() { }

        explicit invalid_sprt(std::size_t id) noexcept : base_type(id) { }

        /* Takes any JSON object that has "id" property with non-negative inte value. */
        invalid_sprt(const nlohmann::json& j, std::error_code& ec) noexcept
            : base_type()
        {
            // Parse json entries.
            std::size_t id = this->id();
            aftermath::noexcept_json::optional(j, type::jstr_typename, this->m_typename, ec);
            aftermath::noexcept_json::required(j, type::jstr_id, id, ec);
            this->set_id(id);
        } // invalid_sprt(...)
        
        template <typename t_other_signal_type, typename t_other_noise_type, bool t_other_sync_check>
        /*implicit*/ invalid_sprt(const invalid_sprt<t_other_signal_type, t_other_noise_type, t_other_sync_check>& similar) noexcept : base_type(similar.id()) { }

        const std::string& rule_typename() const noexcept { return this->m_typename; }
        
        std::string to_path_string(std::size_t /*decimal_places*/) const noexcept
        {
            std::string result = type::typename_string;
            return result;
        } // to_path_string(...)

        bool do_decide_null(value_type /*threshold*/, std::size_t /*row_index*/, std::size_t /*column_index*/) const noexcept { return true; }
        bool do_decide_alt(value_type /*threshold*/, std::size_t /*row_index*/, std::size_t /*column_index*/) const noexcept { return true; }

        /** Output to a stream. */
        friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
        {
            nlohmann::json j = self;
            return os << j;
        } // operator <<(...)
    }; // struct invalid_sprt

    // ~~ Definitions ~~
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char invalid_sprt<t_signal_type, t_noise_type, t_sync_check>::typename_string[];

    // ~~ Json name definitions ~~
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char invalid_sprt<t_signal_type, t_noise_type, t_sync_check>::jstr_typename[];
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check> constexpr char invalid_sprt<t_signal_type, t_noise_type, t_sync_check>::jstr_id[];
    
    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void to_json(nlohmann::json& j, const invalid_sprt<t_signal_type, t_noise_type, t_sync_check>& x) noexcept
    {
        using type = invalid_sprt<t_signal_type, t_noise_type, t_sync_check>;
        std::string sprt_type_str = type::typename_string;

        j = nlohmann::json{
            {type::jstr_typename, sprt_type_str},
            {type::jstr_id, x.id()}
        };
    } // to_json(...)

    template <typename t_signal_type, typename t_noise_type, bool t_sync_check>
    void from_json(const nlohmann::json& j, invalid_sprt<t_signal_type, t_noise_type, t_sync_check>& x)
    {
        using type = invalid_sprt<t_signal_type, t_noise_type, t_sync_check>;
        std::error_code ec {};
        x = type(j, ec);
        if (ec.value() != 0) throw std::runtime_error("Parsing failed: " + j.dump());
    } // from_json(...)
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_INVALID_SPRT_HPP_INCLUDED
