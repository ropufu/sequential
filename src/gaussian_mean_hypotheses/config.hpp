
#ifndef ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_CONFIG_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_CONFIG_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <ropufu/algebra/interval.hpp>
#include <ropufu/algebra/interval_based_vector.hpp>
#include <ropufu/algebra/interval_spacing.hpp>
#include <ropufu/simple_vector.hpp>
#include <ropufu/vector_extender.hpp>

#include "model.hpp"

#include <concepts>    // std::floating_point
#include <cstddef>     // std::size_t
#include <functional>  // std::hash
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::logic_error, std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <variant>     // std::variant, std::monostate, std::visit
#include <vector>      // std::vector

#ifdef ROPUFU_TMP_TYPENAME
#undef ROPUFU_TMP_TYPENAME
#endif
#ifdef ROPUFU_TMP_TEMPLATE_SIGNATURE
#undef ROPUFU_TMP_TEMPLATE_SIGNATURE
#endif
#define ROPUFU_TMP_TYPENAME config<t_value_type>
#define ROPUFU_TMP_TEMPLATE_SIGNATURE template <std::floating_point t_value_type>

namespace ropufu::sequential::gaussian_mean_hypotheses
{
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    struct config;

    ROPUFU_TMP_TEMPLATE_SIGNATURE
    void to_json(nlohmann::json& j, const ROPUFU_TMP_TYPENAME& x) noexcept;
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    void from_json(const nlohmann::json& j, ROPUFU_TMP_TYPENAME& x);

    /** Calculates two stopping times: adaptive SPRT, and generalized SPRT. */
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    struct config
    {
        using type = ROPUFU_TMP_TYPENAME;
        using value_type = t_value_type;

        using model_type = ropufu::sequential::gaussian_mean_hypotheses::model<value_type>;
        using thresholds_type = std::pair<ropufu::aftermath::simple_vector<value_type>, ropufu::aftermath::simple_vector<value_type>>;

        // ~~ Json names ~~
        static constexpr std::string_view jstr_count_simulations = "simulations";
        static constexpr std::string_view jstr_model = "model";
        static constexpr std::string_view jstr_anticipated_sample_size = "anticipated sample size";
        static constexpr std::string_view jstr_asprt_thresholds = "ASPRT thresholds";
        static constexpr std::string_view jstr_gsprt_thresholds = "GSPRT thresholds";

        friend ropufu::noexcept_json_serializer<type>;

        std::size_t count_simulations;
        model_type model;
        std::pair<value_type, value_type> anticipated_sample_size;
        thresholds_type asprt_thresholds;
        thresholds_type gsprt_thresholds;

        config() noexcept = default;

        friend void to_json(nlohmann::json& j, const type& x) noexcept
        {
            j = nlohmann::json{
                {type::jstr_count_simulations, x.count_simulations},
                {type::jstr_model, x.model},
                {type::jstr_anticipated_sample_size, x.anticipated_sample_size},
                {type::jstr_asprt_thresholds, x.asprt_thresholds},
                {type::jstr_gsprt_thresholds, x.gsprt_thresholds}
            };
        } // to_json(...)

        friend void from_json(const nlohmann::json& j, type& x)
        {
            if (!ropufu::noexcept_json::try_get(j, x))
                throw std::runtime_error("Parsing <config> failed: " + j.dump());
        } // from_json(...)
    }; // struct config
} // namespace ropufu::sequential::gaussian_mean_hypotheses

namespace ropufu
{
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    struct noexcept_json_serializer<ropufu::sequential::gaussian_mean_hypotheses::ROPUFU_TMP_TYPENAME>
    {
        using result_type = ropufu::sequential::gaussian_mean_hypotheses::ROPUFU_TMP_TYPENAME;
        using value_type = typename result_type::value_type;
        using initializer_type = ropufu::vector_initializer_t<
            ropufu::aftermath::algebra::linear_spacing<value_type>,
            ropufu::aftermath::algebra::logarithmic_spacing<value_type>,
            ropufu::aftermath::algebra::exponential_spacing<value_type>>;

        static void initialize(const std::pair<initializer_type, initializer_type>& init, typename result_type::thresholds_type& x) noexcept
        {
            std::visit([&x] (auto&& arg) {
                using arg_type = std::decay_t<decltype(arg)>;
                if constexpr (!std::same_as<arg_type, std::monostate>) arg.explode(x.first);
            }, init.first);

            std::visit([&x] (auto&& arg) {
                using arg_type = std::decay_t<decltype(arg)>;
                if constexpr (!std::same_as<arg_type, std::monostate>) arg.explode(x.second);
            }, init.second);
        } // initialize(...)

        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            std::pair<initializer_type, initializer_type> asprt_thresholds;
            std::pair<initializer_type, initializer_type> gsprt_thresholds;

            if (!noexcept_json::required(j, result_type::jstr_count_simulations, x.count_simulations)) return false;
            if (!noexcept_json::required(j, result_type::jstr_model, x.model)) return false;
            if (!noexcept_json::required(j, result_type::jstr_anticipated_sample_size, x.anticipated_sample_size)) return false;
            if (!noexcept_json::required(j, result_type::jstr_asprt_thresholds, asprt_thresholds)) return false;
            if (!noexcept_json::required(j, result_type::jstr_gsprt_thresholds, gsprt_thresholds)) return false;
            
            initialize(asprt_thresholds, x.asprt_thresholds);
            initialize(gsprt_thresholds, x.gsprt_thresholds);
            
            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

#endif // ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_CONFIG_HPP_INCLUDED
