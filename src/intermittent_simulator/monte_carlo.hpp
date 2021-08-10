
#ifndef ROPUFU_SEQUENTIAL_INTERMITTENT_MONTE_CARLO_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_INTERMITTENT_MONTE_CARLO_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/arithmetic.hpp>
#include <ropufu/probability/empirical_measure.hpp>

#include "llr_transform.hpp"
#include "process.hpp"
#include "stopping_time.hpp"

#include <concepts>  // std::totally_ordered
#include <cstddef>   // std::size_t
#include <cstdint>   // std::int32_t
#include <set>       // std::set
#include <stdexcept> // std::invalid_argument
#include <vector>    // std::vector

namespace ropufu::sequential::intermittent
{
    template <std::totally_ordered t_value_type, std::size_t t_block_size = 1'000>
    struct monte_carlo
    {
        using type = monte_carlo<t_value_type, t_block_size>;
        using value_type = t_value_type;

	    static constexpr std::size_t block_size = t_block_size;

        template <typename t_type>
        using matrix_t = ropufu::aftermath::algebra::matrix<t_type>;
        using rule_type = stopping_time<value_type>;
        using empirical_measure_type = ropufu::aftermath::probability::empirical_measure<
            std::size_t, // key_type
            std::size_t, // count_type
            double,       // probability_type
            double,       // sum_type
            double>;      // mean_type

    private:
        std::size_t m_count_simulations = 0;

        static matrix_t<std::int32_t> to_distribution_matrix(const matrix_t<empirical_measure_type>& t, std::size_t column_index)
        {
            std::size_t m = t.height(); // Number of thresholds.
            std::size_t k = 0;
            for (std::size_t i = 0; i < m; ++i)
            {
                std::size_t max_length = t(i, column_index).max();
                if (max_length > k) k = max_length;
            } // for (...)

            matrix_t<std::int32_t> result(m, k);
            for (std::size_t i = 0; i < m; ++i)
            {
                const empirical_measure_type& em = t(i, column_index);
                // for (std::size_t j = 0; j < k; ++j)
                //     result(i, j) = static_cast<std::int32_t>(em.count(j + 1));
                for (const auto& [key, count] : em.data())
                    result(i, key) = static_cast<std::int32_t>(count);
            } // for (...)
            return result;
        } // to_distribution_matrix(...)

        static std::vector<matrix_t<std::int32_t>> to_distribution_matrix(const matrix_t<empirical_measure_type>& t)
        {
            // std::size_t m = t.height(); // Number of thresholds.
            std::size_t n = t.width();  // Number of rules.

            std::vector<matrix_t<std::int32_t>> result{};
            result.reserve(n);
            for (std::size_t j = 0; j < n; ++j) result.push_back(type::to_distribution_matrix(t, j));

            return result;
        } // to_distribution_matrix(...)

    public:
        monte_carlo() noexcept = default;

        explicit monte_carlo(std::size_t count_simulations) noexcept
            : m_count_simulations(count_simulations)
        {
        } // monte_carlo(...)

        template <typename t_rule_collection_type>
        std::vector<matrix_t<std::int32_t>> run_length(
            process<value_type>& proc,
            const llr_transform<value_type>& transform,
            t_rule_collection_type& rules,
            std::vector<value_type>& max_average_run_length)
        {
            std::size_t m = 0;
            std::size_t n = rules.size();
            for (const rule_type* r : rules) if (m < r->thresholds().size()) m = r->thresholds().size();
            for (const rule_type* r : rules) if (r->thresholds().size() != m) throw std::invalid_argument("All rules must have the same number of thresholds.");
            
            if (m == 0 || n == 0) return {};
            matrix_t<empirical_measure_type> t(m, n);
            
            for (std::size_t i = 0; i < this->m_count_simulations; ++i)
            {
                for (rule_type* r : rules) r->reset();
                std::set<std::size_t> running_indices {}; // Indices of rules that are still running.
                for (std::size_t k = 0; k < rules.size(); ++k) running_indices.insert(running_indices.end(), k);

                while (!running_indices.empty())
                {
                    std::vector<value_type> raw_values(block_size);
                    std::vector<value_type> log_likelihood_ratios(block_size);
                    for (std::size_t j = 0; j < block_size; ++j)
                    {
                        raw_values[j] = proc.next();
                        log_likelihood_ratios[j] = transform(raw_values[j]);
                    } // for (...)

                    std::set<std::size_t> still_running = running_indices;
                    for (std::size_t k : running_indices)
                    {
                        rule_type* r = rules[k];
                        r->observe(raw_values, log_likelihood_ratios);
                        if (r->is_stopped()) still_running.erase(k);
                    } // for (...)
                    running_indices = still_running;
                } // while (...)

                for (std::size_t j = 0; j < m; ++j)
                    for (std::size_t k = 0; k < n; ++k)
                        t(j, k).observe(rules[k]->when(j));
            } // for (...)

            max_average_run_length.resize(n);
            for (std::size_t k = 0; k < n; ++k) max_average_run_length[k] = t(m - 1, k).mean();
            return type::to_distribution_matrix(t);
        } // run_length(...)
    }; // struct monte_carlo
} // namespace ropufu::sequential::intermittent

#endif // ROPUFU_SEQUENTIAL_INTERMITTENT_MONTE_CARLO_HPP_INCLUDED
