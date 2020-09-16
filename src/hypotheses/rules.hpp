
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include "rules/adaptive_sprt_design.hpp"
#include "rules/double_sprt_design.hpp"
#include "rules/generalized_sprt_design.hpp"

#include "rules/two_sprt.hpp"
#include "rules/adaptive_sprt.hpp"
#include "rules/double_sprt.hpp"
#include "rules/generalized_sprt.hpp"

#include <variant> // std::variant

namespace ropufu::sequential::hypotheses
{
    template <typename t_value_type>
    using rule_design_variant = std::variant<
        adaptive_sprt_design<t_value_type>,
        generalized_sprt_design<t_value_type>,
        double_sprt_design<t_value_type>>;
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SPRTS_HPP_INCLUDED
