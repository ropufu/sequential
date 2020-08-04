
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_WRITER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_WRITER_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/probability/moment_statistic.hpp>
#include <ropufu/format/mat4_ostream.hpp>

#include "../hypotheses/operating_characteristic.hpp"
#include "../hypotheses/model.hpp"
#include "../hypotheses/change_of_measure.hpp"
#include "../hypotheses/observer.hpp"

#include <array>        // std::array
#include <cstddef>      // std::size_t
#include <filesystem>   // std::filesystem::path
#include <stdexcept>    // std::runtime_error, std::length_error
#include <string>       // std::string
#include <system_error> // std::error_code, std::errc

namespace ropufu::sequential::hypotheses
{
    /** Takes care of writing .mat output. */
    template <typename t_engine_type, typename t_value_type>
    struct writer
    {
        using type = writer<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;

        using model_type = hypotheses::model<value_type>;
        using change_of_measure_type = hypotheses::change_of_measure<value_type>;
        using observer_type = hypotheses::observer<engine_type, value_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        using moment_statistic_type = aftermath::probability::moment_statistic<matrix_t<value_type>>;
        using oc_statistic_type = hypotheses::oc_array_t<moment_statistic_type>;
        using matstream_type = aftermath::format::mat4_ostream;

        using congif_type = hypotheses::config<engine_type, value_type>;

    private:
        bool m_is_good = false;
        model_type m_model = {};
        std::filesystem::path m_root = "."; // Where to write output to.
        std::filesystem::path m_mat_subfolder = "."; // Path to subfolder, storing the mat files.
        std::filesystem::path m_config_path = "."; // Path to a copy of config file.

        /** @brief Generates prefix for .mat files, and copies the config file to the output folder.
         *  @remark If prefix has been fixed, and has already been generated, does nothing.
         */
        template <std::size_t t_prefix_size = 3>
        void initialize_subfolders(const std::filesystem::path& config_path, std::error_code& ec)
        {
            static_assert(t_prefix_size >= 1 && t_prefix_size <= 8, "Prefix size has to be an integer between 1 and 8.");

            constexpr char letter_from = 'a';
            constexpr char letter_to = 'z';

            this->m_is_good = false;
            std::array<char, t_prefix_size> prefix {};
            prefix.fill(letter_from);

            ec.clear();
            while (true)
            {
                // Build subfolder name: "<prefix> space <string representation of model>".
                std::string subfolder_name = "";
                for (std::size_t i = 0; i < t_prefix_size; ++i) subfolder_name += prefix[i];
                subfolder_name += " ";
                subfolder_name += this->m_model.to_path_string();
                std::string config_name = subfolder_name + ".json";
                subfolder_name += "/";

                // Build name for a copy of config file.
                this->m_mat_subfolder = this->m_root / subfolder_name;
                this->m_config_path = this->m_root / config_name;

                // Check that neither exists; otherwise generate another prefix.
                bool is_subfolder_new = !std::filesystem::exists(this->m_mat_subfolder, ec);
                bool is_config_new = !std::filesystem::exists(this->m_config_path, ec);
                if (ec.value() != 0) return;
                if (is_subfolder_new && is_config_new) break;

                // The prefix is already taken: switch to next one.
                std::size_t index = t_prefix_size - 1;
                ++prefix[index];
                bool is_overflow = prefix[index] > letter_to;
                while (is_overflow)
                {
                    prefix[index] = letter_from;
                    if (index == 0) break;
                    --index;
                    ++prefix[index];
                    is_overflow = prefix[index] > letter_to;
                } // while (...)

                // Check if we've run out of options.
                if (is_overflow) throw std::length_error("Subfolder name overflow.");
            } // while (...)

            // Create required directory structure.
            std::filesystem::create_directories(this->m_mat_subfolder, ec);
            if (ec.value() != 0) return;

            // Copy the current configuration to output folder.
            std::filesystem::copy_file(config_path, this->m_config_path, ec);
            if (ec.value() != 0) return;

        } // initialize_subfolders(...)
    
    public:
        explicit writer(
            const std::filesystem::path& mat_output_path,
            const std::filesystem::path& config_path,
            const model_type& model)
            : m_model(model), m_root(mat_output_path)
        {
            std::error_code ec {};
            this->initialize_subfolders(config_path, ec);
            this->m_is_good = (ec.value() == 0);
        } // writer(...)

        bool good() const noexcept { return this->m_is_good; }

        void write_mat(std::size_t count_simulations, const observer_type* rule_ptr, const oc_array_t<moment_statistic_type>& oc)
        {
            if (!this->m_is_good) return; // Make sure the writer has been successfully initialized.

            std::string mat_name = rule_ptr->to_path_string(2) + " oc.mat";
            std::filesystem::path mat_path = this->m_mat_subfolder / mat_name;

            // Threshold matrices.
            matrix_t<value_type> unscaled_null_thresholds = matrix_t<value_type>::column_vector(rule_ptr->unscaled_null_thresholds());
            matrix_t<value_type> unscaled_alt_thresholds = matrix_t<value_type>::row_vector(rule_ptr->unscaled_alt_thresholds());
            //unscaled_null_thresholds.try_reshape(unscaled_null_thresholds.size(), 1);
            //unscaled_alt_thresholds.try_reshape(1, unscaled_alt_thresholds.size());

            matstream_type mat {mat_path};
            // mat.wipe(); // Clear the existing contents.

            // Write model.
            mat << "count_simulations" << matrix_t<value_type>(1, 1, static_cast<value_type>(count_simulations));
            mat << "mu_null" << matrix_t<value_type>(1, 1, this->m_model.mu_under_null())
                << "mu_alt" << matrix_t<value_type>(1, 1, this->m_model.smallest_mu_under_alt());
            // Write thresholds.
            mat << "b_null" << unscaled_null_thresholds
                << "b_alt" << unscaled_alt_thresholds;
            // Write observations.
            for (const auto& pair : oc)
            {
                std::string expected_value_varname {};
                std::string variance_varname {};
                if (!detail::mat_var_name(pair.key(), expected_value_varname, variance_varname)) throw std::runtime_error("OC not recognized.");

                const moment_statistic_type& value = pair.value();
                mat <<
                    expected_value_varname << value.mean() <<
                    variance_varname << value.variance();
            } // for (...)
        } // write_mat(...)

        void write_mat(std::size_t count_simulations, const observer_type* rule_ptr, const change_of_measure_type& mu_pair)
        {
            if (!this->m_is_good) return; // Make sure the writer has been successfully initialized.

            std::string mat_name = rule_ptr->to_path_string(2) + " more " + mu_pair.to_path_string() + ".mat";
            std::filesystem::path mat_path = this->m_mat_subfolder / mat_name;

            // Threshold matrices.
            matrix_t<value_type> unscaled_null_thresholds = matrix_t<value_type>::column_vector(rule_ptr->unscaled_null_thresholds());
            matrix_t<value_type> unscaled_alt_thresholds = matrix_t<value_type>::row_vector(rule_ptr->unscaled_alt_thresholds());
            //unscaled_null_thresholds.try_reshape(unscaled_null_thresholds.size(), 1);
            //unscaled_alt_thresholds.try_reshape(1, unscaled_alt_thresholds.size());
            
            // Observation matrices.
            const moment_statistic_type& decision_errors = rule_ptr->decision_errors();
            const moment_statistic_type& run_lengths = rule_ptr->run_lengths();

            matstream_type mat {mat_path};
            // mat.wipe(); // Clear the existing contents.

            // Write model.
            mat << "count_simulations" << matrix_t<value_type>(1, 1, static_cast<value_type>(count_simulations));
            mat << "mu_null" << matrix_t<value_type>(1, 1, this->m_model.mu_under_null())
                << "mu_alt" << matrix_t<value_type>(1, 1, this->m_model.smallest_mu_under_alt());
            // Write custom simulation information.
            mat << "analyzed_mu" << matrix_t<value_type>(1, 1, mu_pair.analyzed())
                << "simulated_mu" << matrix_t<value_type>(1, 1, mu_pair.simulated());
            // Write thresholds.
            mat << "b_null" << unscaled_null_thresholds
                << "b_alt" << unscaled_alt_thresholds;
            // Write observations.
            mat << "perror" << decision_errors.mean() << "verror" << decision_errors.variance()
                << "ess" << run_lengths.mean() << "vss" << run_lengths.variance();
        } // write_mat(...)
    }; // struct writer
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_WRITER_HPP_INCLUDED
