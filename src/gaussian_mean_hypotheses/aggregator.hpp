
#ifndef ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_AGGREGATOR_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_AGGREGATOR_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/probability/moment_statistic.hpp>

#include "model.hpp"
#include "xsprt.hpp"

#include <concepts>    // std::floating_point
#include <cstddef>     // std::size_t
#include <functional>  // std::hash
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::logic_error, std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <variant>     // std::variant, std::monostate, std::visit
#include <vector>      // std::vector

namespace ropufu::sequential::gaussian_mean_hypotheses
{
    template <std::floating_point t_value_type>
    struct aggregator
    {
        using type = aggregator<t_value_type>;
        using value_type = t_value_type;
        
        using statistic_type = xsprt<value_type>;
        using simulator_output_type = typename statistic_type::output_type;
        
        template <typename t_data_type>
        using matrix_t = ropufu::aftermath::algebra::matrix<t_data_type>;
        using sample_size_type = ropufu::aftermath::probability::moment_statistic<
            matrix_t<std::size_t>,
            matrix_t<value_type>>;
        using error_probability_type = ropufu::aftermath::probability::moment_statistic<matrix_t<value_type>>;

    private:
        xsprt_pair<sample_size_type> m_sample_size = {};
        xsprt_pair<error_probability_type> m_direct_error_indicator = {};
        xsprt_pair<error_probability_type> m_importance_error_indicator = {};
        std::size_t m_height = 0;
        std::size_t m_width = 0;
        value_type m_anticipated_sample_size = 0;

        bool empty() const noexcept
        {
            return this->m_height == 0 || this->m_width == 0;
        } // empty(...)

        void initialize(std::size_t height, std::size_t width, value_type anticipated_sample_size) noexcept
        {
            this->m_height = height;
            this->m_width = width;
            this->m_anticipated_sample_size = anticipated_sample_size;

            matrix_t<value_type> zero = matrix_t<value_type>(height, width);
            matrix_t<value_type> x = matrix_t<value_type>(height, width, anticipated_sample_size);
            this->m_sample_size = {sample_size_type(x), sample_size_type(x)};
            this->m_direct_error_indicator = {error_probability_type(zero), error_probability_type(zero)};
            this->m_importance_error_indicator = {error_probability_type(zero), error_probability_type(zero)};
        } // initialize(...)

    public:
        aggregator() noexcept
        {
        } // aggregator(...)

        const xsprt_pair<sample_size_type>& sample_size() const noexcept { return this->m_sample_size; }

        const xsprt_pair<error_probability_type>& direct_error_indicator() const noexcept { return this->m_direct_error_indicator; }

        const xsprt_pair<error_probability_type>& importance_error_indicator() const noexcept { return this->m_importance_error_indicator; }

        void operator()(const simulator_output_type& value)
        {
            if (this->empty()) this->initialize(value.height(), value.width(), value.anticipated_sample_size);

            this->m_sample_size.adaptive_sprt.observe(value.when_stopped.adaptive_sprt);
            this->m_sample_size.generalized_sprt.observe(value.when_stopped.generalized_sprt);

            this->m_direct_error_indicator.adaptive_sprt.observe(value.direct_error_indicator.adaptive_sprt);
            this->m_direct_error_indicator.generalized_sprt.observe(value.direct_error_indicator.generalized_sprt);

            this->m_importance_error_indicator.adaptive_sprt.observe(value.importance_error_indicator.adaptive_sprt);
            this->m_importance_error_indicator.generalized_sprt.observe(value.importance_error_indicator.generalized_sprt);
        } // operator ()(...)

        void operator()(const type& other)
        {
            if (this->empty()) this->initialize(other.m_height, other.m_width, other.m_anticipated_sample_size);

            this->m_sample_size.adaptive_sprt.observe(other.m_sample_size.adaptive_sprt);
            this->m_sample_size.generalized_sprt.observe(other.m_sample_size.generalized_sprt);

            this->m_direct_error_indicator.adaptive_sprt.observe(other.m_direct_error_indicator.adaptive_sprt);
            this->m_direct_error_indicator.generalized_sprt.observe(other.m_direct_error_indicator.generalized_sprt);

            this->m_importance_error_indicator.adaptive_sprt.observe(other.m_importance_error_indicator.adaptive_sprt);
            this->m_importance_error_indicator.generalized_sprt.observe(other.m_importance_error_indicator.generalized_sprt);
        } // operator ()(...)
    }; // struct aggregator
} // namespace ropufu::sequential::gaussian_mean_hypotheses

#endif // ROPUFU_SEQUENTIAL_GAUSSIAN_MEAN_HYPOTHESES_AGGREGATOR_HPP_INCLUDED
