
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>

#include "../draft/probability/moment_statistic.hpp"

#include "operating_characteristic.hpp"
#include "likelihood.hpp"
#include "simple_process.hpp"
#include "change_of_measure.hpp"

#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <stdexcept>   // std::invalid_argument
#include <type_traits> // std::is_convertible_v
#include <vector>      // std::vector

namespace ropufu::sequential::hypotheses
{
    /** @brief Process observer that keeps track of likelihood statistics.
     *  Every time the process updates tic(...) is called. Once the monitoring
     *  stops toc(...) is executed.
     *  Typical behavior involves a sequence of tics followed by a single toc.
     */
    template <typename t_engine_type, typename t_value_type>
    struct observer
    {
        using type = observer<t_engine_type, t_value_type>;
        using engine_type = t_engine_type;
        using value_type = t_value_type;
        using process_type = simple_process<engine_type, value_type>;
        using likelihood_type = likelihood<value_type>;
        using change_of_measure_type = change_of_measure<value_type>;

        template <typename t_data_type>
        using matrix_t = aftermath::algebra::matrix<t_data_type>;
        using moment_statistic_type = aftermath::probability::moment_statistic<matrix_t<value_type>>;

        virtual ~observer() noexcept { }

        /** @brief Resets the timer and all statistics. Should preceed any simulations. */
        virtual void clean_up() = 0;
        /** @brief Prepares the rule for another simulation pass to zero but keeps all statistics. */
        virtual void reset() = 0;
        /** @brief Updates the rule with the newest observation. */
        virtual void tic(const process_type& /*proc*/, const likelihood_type& /*likelihood*/) = 0;
        /** @brief Finalizes the flow of toc's and updates the statistics. */
        virtual void toc(const process_type& /*proc*/, const likelihood_type& /*likelihood*/, const change_of_measure_type& /*signal_strength*/) = 0;

        /** @brief Indicates if the observer is listening to tic's. */
        virtual bool is_listening() const noexcept = 0;
        
        virtual const std::vector<value_type>& unscaled_null_thresholds() const noexcept = 0;
        virtual const std::vector<value_type>& unscaled_alt_thresholds() const noexcept = 0;
        
        virtual const moment_statistic_type& decision_errors() const noexcept = 0;
        virtual const moment_statistic_type& run_lengths() const noexcept = 0;

        virtual std::string to_path_string(std::size_t /*decimal_places*/) const noexcept { return ""; }

        const moment_statistic_type& read_oc(operating_characteristic oc) const
        {
            switch (oc)
            {
                case operating_characteristic::ess_under_null: return this->run_lengths();
                case operating_characteristic::ess_under_alt: return this->run_lengths();
                case operating_characteristic::probability_of_false_alarm: return this->decision_errors();
                case operating_characteristic::probability_of_missed_signal: return this->decision_errors();
                default:
                    throw std::invalid_argument("OC not recognized.");
            } // switch (...)
        } // read_oc(...)

        template <typename t_collection_type>
        static bool any_listening(const t_collection_type& observer_pointers) noexcept
        {
            static_assert(
                std::is_convertible_v<decltype(**observer_pointers.begin()), const type&>,
                "Elements of the collection must be dereferencable to const observer references.");
            for (const auto& x : observer_pointers) if (x->is_listening()) return true;
            return false;
        } // are_listening(...)
    }; // struct observer
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_OBSERVER_HPP_INCLUDED
