
#ifndef ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_MODEL_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_MODEL_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include <ropufu/number_traits.hpp>
#include <ropufu/simple_vector.hpp>

#include <concepts>    // std::floating_point
#include <cstddef>     // std::size_t
#include <functional>  // std::hash
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::logic_error, std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

#ifdef ROPUFU_TMP_TYPENAME
#undef ROPUFU_TMP_TYPENAME
#endif
#ifdef ROPUFU_TMP_TEMPLATE_SIGNATURE
#undef ROPUFU_TMP_TEMPLATE_SIGNATURE
#endif
#define ROPUFU_TMP_TYPENAME model<t_value_type>
#define ROPUFU_TMP_TEMPLATE_SIGNATURE template <std::floating_point t_value_type>


namespace ropufu::sequential::gaussian_mean_hypotheses
{
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    struct model;

    ROPUFU_TMP_TEMPLATE_SIGNATURE
    void to_json(nlohmann::json& j, const ROPUFU_TMP_TYPENAME& x) noexcept;
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    void from_json(const nlohmann::json& j, ROPUFU_TMP_TYPENAME& x);

    /** Calculates two stopping times: adaptive SPRT, and generalized SPRT. */
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    struct model
    {
        using type = ROPUFU_TMP_TYPENAME;
        using value_type = t_value_type;

        /** Names the model. */
        static constexpr std::string_view name = "Gaussian mean hypotheses";

        // ~~ Json names ~~
        static constexpr std::string_view jstr_type = "type";
        static constexpr std::string_view jstr_weakest_signal_strength = "weakest signal strength";

        friend ropufu::noexcept_json_serializer<type>;
        friend std::hash<type>;

    private:
        value_type m_weakest_signal_strength = 1;

        /** @brief Validates the structure and returns an error message, if any. */
        std::optional<std::string> error_message() const noexcept
        {
            if (!aftermath::is_finite(this->m_weakest_signal_strength)) return "Weakest signal strength must be finite.";
            if (this->m_weakest_signal_strength <= 0) return "Weakest signal strength must be positive.";
            
            return std::nullopt;
        } // error_message(...)
        
        /** @exception std::logic_error Validation failed. */
        void validate() const
        {
            std::optional<std::string> message = this->error_message();
            if (message.has_value()) throw std::logic_error(message.value());
        } // validate(...)

    public:
        model() noexcept = default;

        explicit model(value_type weakest_signal_strength)
            : m_weakest_signal_strength(weakest_signal_strength)
        {
            this->validate();
        } // model(...)

        // Signal as a function of time.
        constexpr t_value_type signal_at(std::size_t /*time*/) const noexcept { return 1; }

        value_type weakest_signal_strength() const noexcept { return this->m_weakest_signal_strength; }

        bool operator ==(const type& other) const noexcept
        {
            return
                this->m_weakest_signal_strength == other.m_weakest_signal_strength;
        } // operator ==(...)

        bool operator !=(const type& other) const noexcept
        {
            return !this->operator ==(other);
        } // operator !=(...)

        friend void to_json(nlohmann::json& j, const type& x) noexcept
        {
            j = nlohmann::json{
                {type::jstr_type, type::name},
                {type::jstr_weakest_signal_strength, x.m_weakest_signal_strength}
            };
        } // to_json(...)

        friend void from_json(const nlohmann::json& j, type& x)
        {
            if (!ropufu::noexcept_json::try_get(j, x))
                throw std::runtime_error("Parsing <model> failed: " + j.dump());
        } // from_json(...)
    }; // struct model
} // namespace ropufu::sequential::gaussian_mean_hypotheses

namespace ropufu
{
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    struct noexcept_json_serializer<ropufu::sequential::gaussian_mean_hypotheses::ROPUFU_TMP_TYPENAME>
    {
        using result_type = ropufu::sequential::gaussian_mean_hypotheses::ROPUFU_TMP_TYPENAME;
        static bool try_get(const nlohmann::json& j, result_type& x) noexcept
        {
            std::string model_name;
            if (!noexcept_json::required(j, result_type::jstr_type, model_name)) return false;
            if (!noexcept_json::required(j, result_type::jstr_weakest_signal_strength, x.m_weakest_signal_strength)) return false;
            
            if (model_name != result_type::name) return false;
            if (x.error_message().has_value()) return false;

            return true;
        } // try_get(...)
    }; // struct noexcept_json_serializer<...>
} // namespace ropufu

namespace std
{
    ROPUFU_TMP_TEMPLATE_SIGNATURE
    struct hash<ropufu::sequential::gaussian_mean_hypotheses::ROPUFU_TMP_TYPENAME>
    {
        using argument_type = ropufu::sequential::gaussian_mean_hypotheses::ROPUFU_TMP_TYPENAME;
        using result_type = std::size_t;

        result_type operator ()(argument_type const& x) const noexcept
        {
            std::hash<typename argument_type::value_type> signal_strength_hasher{};
            return signal_strength_hasher(x.m_weakest_signal_strength);
        } // operator ()(...)
    }; // struct hash<...>
} // namespace std

#endif // ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_MODEL_HPP_INCLUDED
