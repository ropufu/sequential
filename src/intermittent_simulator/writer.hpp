
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_WRITER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_WRITER_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>
#include <ropufu/format/mat4_ostream.hpp>

#include "stopping_time.hpp"

#include <concepts>   // std::totally_ordered
#include <cstddef>    // std::size_t
#include <cstdint>    // std::int32_t
#include <filesystem> // std::filesystem::path
#include <stdexcept>  // std::invalid_argument
#include <string>     // std::string
#include <vector>     // std::vector

namespace ropufu::sequential::intermittent
{
    /** Takes care of writing .mat output. */
    template <std::totally_ordered t_value_type>
    static void write_mat(const std::filesystem::path& mat_path,
        const stopping_time<t_value_type>& rule,
        const ropufu::aftermath::algebra::matrix<std::int32_t>& pmf,
        std::string prefix = "")
    {
        using matrix_type = ropufu::aftermath::algebra::matrix<t_value_type>;
        using matstream_type = aftermath::format::mat4_ostream;

        matrix_type thresholds = matrix_type::column_vector(rule.thresholds());
        if (thresholds.height() != pmf.height()) throw std::invalid_argument("Point mass function size mismatch.");

        matstream_type mat {mat_path};
        prefix += rule.mat_name();
        mat
            << (prefix + "_thresholds") << thresholds
            << (prefix + "_pmf") << pmf;
    } // write_mat(...)
} // namespace ropufu::sequential::intermittent

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_WRITER_HPP_INCLUDED
