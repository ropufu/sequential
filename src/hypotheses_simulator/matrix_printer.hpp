
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_MATRIX_PRINTER_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_MATRIX_PRINTER_HPP_INCLUDED

#include <ropufu/algebra/matrix.hpp>

#include <cmath>    // std::sqrt
#include <cstddef>  // std::size_t
#include <iomanip>  // std::setw
#include <ios>      // std::left, std::right
#include <iostream> // std::endl
#include <string>   // std::string

namespace ropufu::sequential::hypotheses
{
    template <typename t_numeric_type>
    struct matrix_printer
    {
        using type = matrix_printer<t_numeric_type>;
        using numeric_type = t_numeric_type;

        /** @brief Prints the corner values of two matrices.
         *  @param emat Matrix of expected values (sample means).
         *  @param vmat Matrix of variances (sample variances).
         */
        template <typename t_arrangement_type, typename t_allocator_type>
        static void print_corners(std::ostream& os,
            const aftermath::algebra::matrix<numeric_type, t_arrangement_type, t_allocator_type>& emat,
            const aftermath::algebra::matrix<numeric_type, t_arrangement_type, t_allocator_type>& vmat,
            std::string&& prefix, std::size_t fixed_width = 15) noexcept
        {
            if (emat.size() == 0 || vmat.size() == 0)
            {
                os << prefix << "empty" << std::endl;
                return;
            } // if (...)
            if (emat.height() != vmat.height() || emat.width() != vmat.width())
            {
                os << prefix << "size mismatch" << std::endl;
                return;
            } // if (...)

            std::string left_padding(prefix.size(), ' ');
            const std::string middle_blank = "        ";
            const std::string middle_center = "   pm   ";

            std::size_t m = emat.height() - 1; // Cannot cause underflow because <emat> is not empty.
            std::size_t n = emat.width() - 1; // Cannot cause underflow because <emat> is not empty.

            //   a --- b      x --- y
            //   | ... |  pm  | ... |
            //   c --- d      z --- w
            numeric_type a = emat(0, n); numeric_type x = std::sqrt(vmat(0, n));
            numeric_type b = emat(m, n); numeric_type y = std::sqrt(vmat(m, n));
            numeric_type c = emat(0, 0); numeric_type z = std::sqrt(vmat(0, 0));
            numeric_type d = emat(m, 0); numeric_type w = std::sqrt(vmat(m, 0));

            os << left_padding << std::setw(fixed_width) << std::left << a
                << " --- " << std::setw(fixed_width) << std::right << b
                << middle_blank << std::setw(fixed_width) << std::left << x
                << " --- " << std::setw(fixed_width) << std::right << y << std::endl;
            os << prefix << std::setw(fixed_width) << std::left << " |"
                << " ... " << std::setw(fixed_width) << std::right << "| "
                << middle_center << std::setw(fixed_width) << std::left << " |"
                << " ... " << std::setw(fixed_width) << std::right << "| " << std::endl;
            os << left_padding << std::setw(fixed_width) << std::left << c
                << " --- " << std::setw(fixed_width) << std::right << d
                << middle_blank << std::setw(fixed_width) << std::left << z
                << " --- " << std::setw(fixed_width) << std::right << w << std::endl;
        } // print_corners(...)
    }; // enum matrix_printer
} // namespace ropufu::sequential::hypotheses

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_SIMULATOR_MATRIX_PRINTER_HPP_INCLUDED
