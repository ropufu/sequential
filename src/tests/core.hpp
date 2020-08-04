
#ifndef ROPUFU_AFTERMATH_TESTS_CORE_HPP_INCLUDED
#define ROPUFU_AFTERMATH_TESTS_CORE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/discrepancy.hpp>
#include <ropufu/number_traits.hpp>

#include <array>   // std::array
#include <chrono>  // std::chrono::steady_clock, std::chrono::system_clock
#include <cstddef> // std::size_t
#include <list>    // std::list
#include <random>  // std::seed_seq
#include <sstream> // std::ostringstream
#include <vector>  // std::vector

namespace ropufu::sequential::tests
{
    static int g_aux_test_counter = 0;
    
    template <typename t_left_type, typename t_right_type>
    struct type_pair
    {
        using left_type = t_left_type;
        using right_type = t_right_type;
    }; // struct type_pair
    
    template <typename t_left_type, typename t_middle_type, typename t_right_type>
    struct type_triplet
    {
        using left_type = t_left_type;
        using middle_type = t_middle_type;
        using right_type = t_right_type;
    }; // struct type_triplet

    template <typename t_test_type>
    double benchmark(t_test_type&& test) noexcept
    {
        auto tic = std::chrono::steady_clock::now();
        test();
        auto toc = std::chrono::steady_clock::now();
        
        return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic).count()) / 1'000.00;
    } // cusum_run_length(...)

    template <typename t_engine_type>
    void seed(t_engine_type& engine) noexcept
    {
        int time_seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
        std::seed_seq sequence { 1, 1, 2, 3, 5, 8, 1729, time_seed, ++ropufu::sequential::tests::g_aux_test_counter };
        engine.seed(sequence);
    } // seed(...)

    template <typename t_type>
    bool does_json_round_trip(const t_type& x) noexcept
    {
        try
        {
            nlohmann::json j = x;
            t_type y = j;
            return x == y;
        } // try
        catch (...) { return false; }
    } // does_json_round_trip(...)

    template <typename t_type>
    bool does_json_round_trip(const t_type& x, double tolerance) noexcept
    {
        try
        {
            nlohmann::json j = x;
            t_type y = j;
            return static_cast<double>(ropufu::aftermath::discrepancy(x, y)) < tolerance;
        } // try
        catch (...) { return false; }
    } // does_json_round_trip(...)

    template <typename t_type>
    bool are_ostream_equal(const t_type& x, const t_type& y) noexcept
    {
        try
        {
            std::ostringstream ssx { };
            std::ostringstream ssy { };

            ssx << x;
            ssy << y;
            return ssx.str() == ssy.str();
        } // try
        catch (...) { return false; }
    } // are_ostream_equal(...)
} // namespace ropufu::sequential::tests

#endif // ROPUFU_AFTERMATH_TESTS_CORE_HPP_INCLUDED
