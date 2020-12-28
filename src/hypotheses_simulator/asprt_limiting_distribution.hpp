
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_ASPRT_LIMITING_DISTRIBUTION_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_ASPRT_LIMITING_DISTRIBUTION_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/probability/empirical_measure.hpp>
#include <ropufu/probability/moment_statistic.hpp>
#include <ropufu/random/normal_sampler_512.hpp>
#include <ropufu/format/mat4_ostream.hpp>

#include "../hypotheses/model.hpp"

#include <array>        // std::array
#include <cmath>        // std::floor
#include <cstddef>      // std::size_t
#include <filesystem>   // std::filesystem::path
#include <stdexcept>    // std::runtime_error, std::length_error
#include <string>       // std::string
#include <system_error> // std::error_code, std::errc

namespace ropufu::sequential::hypotheses
{
    /** Takes care of writing .mat output. */
    template <typename t_engine_type, typename t_value_type>
    struct asprt_limiting_distribution
    {
        using type = asprt_limiting_distribution<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using model_type = hypotheses::model<value_type>;
        using sampler_type = aftermath::random::normal_sampler_512<engine_type, value_type>;
        using empirical_measure_type = aftermath::probability::empirical_measure<int>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        using moment_statistic_type = aftermath::probability::moment_statistic<matrix_t<value_type>>;
        using matstream_type = aftermath::format::mat4_ostream;

    private:
        bool m_is_good = false;
        std::filesystem::path m_root = "."; // Where to write output to.
        std::string m_model_name = "";
        value_type m_initial_value = 0;

    public:
        explicit asprt_limiting_distribution(
            const std::filesystem::path& mat_output_path,
            const model_type& model)
            : m_root(mat_output_path), m_model_name(model.to_path_string())
        {
            // constexpr long double star = (long double)(0.58578643762690495119831127579030192143);
            // long double alt_mu = static_cast<long double>(model.smallest_mu_under_alt());
            // this->m_initial_value = static_cast<value_type>(star * alt_mu);
            this->m_initial_value = model.smallest_mu_under_alt();

            std::error_code ec {};
            // Create required directory structure.
            std::filesystem::create_directories(this->m_root, ec);
            if (ec.value() == 0) this->m_is_good = true;
        } // asprt_limiting_distribution(...)

        /** 
         *  @param count_simulations Number of Monte-Carlo simulations to perform.
         *  @param count_observations Number of observations generated per each Monte-Carlo simulation.
         *  @param support_resolution Width of a bin when storing the distribution estimate.
         */
        void touch(engine_type& engine, std::size_t count_simulations,
            std::size_t count_observations, std::size_t time_cutoff) noexcept
        {   
            std::string mat_name = this->m_model_name + " limiting dist one.mat";
            std::filesystem::path mat_path = this->m_root / mat_name;
            matstream_type mat {mat_path};
            // mat.wipe(); // Clear the existing contents.

            sampler_type sampler {};
            matrix_t<value_type> dist {count_simulations, count_observations, this->m_initial_value};
            if (dist.empty()) return;

            for (std::size_t i = 0; i < count_simulations; ++i)
            {
                for (value_type n = 1; n <= time_cutoff; ++n)
                {
                    for (value_type& x : dist.row(i))
                    {
                        value_type epsilon = sampler(engine);
                        value_type negative_part = 0;
                        if (x < 0) negative_part = -x;

                        x += (epsilon + negative_part) / n;
                    } // for (...)
                } // for (...)
            } // for (...)
            
            if (!this->m_is_good) std::cout << "Creating .mat file failed." << std::endl;
            if (this->m_is_good)
            {
                // Write metadata.
                mat << "count_simulations" << matrix_t<value_type>(1, 1, static_cast<value_type>(count_simulations))
                    << "count_observations" << matrix_t<value_type>(1, 1, static_cast<value_type>(count_observations));
                mat << "time_cutoff" << matrix_t<value_type>(1, 1, static_cast<value_type>(time_cutoff));
                mat << "iv" << matrix_t<value_type>(1, 1, this->m_initial_value);
                // Write results.
                mat << "distribution" << dist;
            } // if (...)

            // Visual output.
            empirical_measure_type law {};
            value_type support_resolution = value_type(0.1);
            for (std::size_t i = 0; i < count_simulations; ++i)
            {
                for (const value_type& x : dist.row(i))
                {
                    int bin_index = static_cast<int>(std::floor(x / support_resolution));
                    law << bin_index;
                } // for (...)
            } // for (...)
            std::cout << law << std::endl;
        } // touch(...)
    }; // struct asprt_limiting_distribution
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_ASPRT_LIMITING_DISTRIBUTION_HPP_INCLUDED
