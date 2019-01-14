
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_TYPE_TRAITS_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_TYPE_TRAITS_HPP_INCLUDED

#include <type_traits> // ...
#include <utility>     // std::declval

namespace ropufu::sequential::hypotheses
{
    namespace detail
    {
        /** Checks if the pair (\tparam t_ref_type, \tparam t_const_ref_type)
         *  has the form (T&, const T&) for some decayed type T.
         */
        template <typename t_ref_type, typename t_const_ref_type>
        struct is_ref_const_ref_pair
        {
            using left_type = std::decay_t<t_ref_type>;
            using right_type = std::decay_t<t_const_ref_type>;

            static constexpr bool is_left_ref = std::is_same_v<
                std::add_lvalue_reference_t<left_type>,
                t_ref_type>;

            static constexpr bool is_right_const_ref = std::is_same_v<
                std::add_lvalue_reference_t<std::add_const_t<right_type>>,
                t_const_ref_type>;

            static constexpr bool value = std::is_same_v<left_type, right_type> && is_left_ref && is_right_const_ref;
        }; // struct is_ref_const_ref_pair

        /** Checks if the pair (\tparam t_ref_type, \tparam t_const_ref_type)
         *  has the form (T&, const T&) for some decayed type T.
         */
        template <typename t_ref_type, typename t_const_ref_type>
        inline constexpr bool is_ref_const_ref_pair_v = is_ref_const_ref_pair<t_ref_type, t_const_ref_type>::value;
    } // namespace detail
} // namespace ropufu::sequential::hypotheses


// The following macro creates auxiliary structures to check for CRTP inheritance.
// Specifically, the inheritance can by summarized in the following diagram:
//     A -> T -> "crtp"<T, ...>
// where (A) is type to check, ("crtp") is the CRTP structure, and (T) is the first
// type to inherit from ("crtp"). Note that (T) could be (A) itself.
// To perform this check, ("crtp") has to implement two functions:
//     as_"crtp"() -> T&
//     as_"crtp"() const -> const T&
//

#define ROPUFU_SEQUENTIAL_HYPOTHESES_TYPE_TRAITS_CRTP(CRTP_NAME, CRTP_STRUCT, ...)              \
    namespace detail                                                                            \
    {                                                                                           \
        template <typename t_top_type, typename t_ref_type, typename t_const_ref_type>          \
        struct is_crtp_##CRTP_STRUCT##_chain                                                    \
        {                                                                                       \
            using top_type = std::decay_t<t_top_type>;                                          \
            using core_type = std::decay_t<t_ref_type>;                                         \
            using crtp_type = CRTP_STRUCT<core_type __VA_ARGS__>;                               \
                                                                                                \
            static constexpr bool value =                                                       \
                is_ref_const_ref_pair_v<t_ref_type, t_const_ref_type> &&                        \
                std::is_base_of_v<core_type, top_type> &&                                       \
                std::is_base_of_v<crtp_type, core_type>;                                        \
        };                                                                                      \
                                                                                                \
        template <typename, typename = void>                                                    \
        struct is_##CRTP_NAME : public std::false_type { };                                     \
                                                                                                \
        template <typename t_type>                                                              \
        struct is_##CRTP_NAME<                                                                  \
            t_type, std::void_t<                                                                \
                std::enable_if_t<                                                               \
                    is_crtp_##CRTP_STRUCT##_chain<                                              \
                        t_type,                                                                 \
                        decltype( std::declval<t_type&>().as_##CRTP_NAME() ),                   \
                        decltype( std::declval<const t_type&>().as_##CRTP_NAME() )>::value,     \
                    t_type>>> : public std::true_type{ };                                       \
    }                                                                                           \
                                                                                                \
    template <typename t_type>                                                                  \
    inline constexpr bool is_##CRTP_NAME##_v = detail::is_##CRTP_NAME<t_type>::value;           \
                                                                                                \


#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_TYPE_TRAITS_HPP_INCLUDED
