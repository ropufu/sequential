
#ifndef ROPUFU_SEQUENTIAL_INTERMITTENT_STOPPING_TIME_FACTORY_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_INTERMITTENT_STOPPING_TIME_FACTORY_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <ropufu/noexcept_json.hpp>

#include "stopping_time.hpp"

#include "stopping_times/cusum.hpp"
#include "stopping_times/finite_moving_average.hpp"
#include "stopping_times/sliding_cusum.hpp"

#include <iostream> // std::cout, std::endl
#include <iterator> // std::iterator_traits
#include <string>   // std::string
#include <vector>   // std::vector

namespace ropufu::sequential::intermittent::detail
{
    /** If \c t_type is either
     *  -- const pointer to const object;
     *  -- const pointer to object; or
     *  -- pointer to const object;
     *  then \c type = const pointer to const object.
     *  If \c t_type is
     *  -- pointer to object;
     *  then \c type = const pointer to object.
     *  Otherwise, \c type equals \c t_type.
     **/
    template <typename t_type>
    struct pointer_const_shift
    {
        using type = t_type;
    }; // pointer_const_shift

    template <typename t_value_type>
    struct pointer_const_shift<const t_value_type*>
    {
        using type = const t_value_type* const;
    }; // struct pointer_const_shift<...>

    template <typename t_value_type>
    struct pointer_const_shift<t_value_type* const>
    {
        using type = const t_value_type* const;
    }; // struct pointer_const_shift<...>

    template <typename t_value_type>
    struct pointer_const_shift<t_value_type*>
    {
        using type = t_value_type* const;
    }; // struct pointer_const_shift<...>

    template <typename t_type>
    using pointer_const_shift_t = typename pointer_const_shift<t_type>::type;

    /** Wraps iterator over pointers, so that dereferencing casts:
     *  -- const pointer to object into const pointer to const object;
     *  -- pointer to const object into const pointer to const object;
     *  -- pointer to object into const pointer to object.
     **/
    template <typename t_iterator_type>
    struct iterator_over_pointers
    {
        using type = iterator_over_pointers<t_iterator_type>;
        using iterator_type = t_iterator_type;

        using iterator_traits_type = std::iterator_traits<iterator_type>;
        using iterator_value_type = typename iterator_traits_type::value_type;
        using value_type = pointer_const_shift_t<iterator_value_type>;

    private:
        iterator_type m_underlying_iterator;

    public:
        /*implicit*/ iterator_over_pointers(iterator_type underlying_iterator) noexcept
            : m_underlying_iterator(underlying_iterator)
        {
        } // iterator_over_pointers(...)

        bool operator !=(const type& other) const noexcept { return this->m_underlying_iterator != other.m_underlying_iterator; }
        bool operator ==(const type& other) const noexcept { return this->m_underlying_iterator == other.m_underlying_iterator; }

        value_type operator *() const noexcept
        {
            return static_cast<value_type>(*this->m_underlying_iterator);
        } // operator *(...)

        type& operator ++() noexcept
        {
            ++this->m_underlying_iterator;
            return *this;
        } // operator ++(...)
    }; // struct iterator_over_pointers
} // namespace ropufu::sequential::intermittent::detail

namespace ropufu::sequential::intermittent
{
    template <typename t_value_type>
    struct stopping_time_factory
    {
        using type = stopping_time_factory<t_value_type>;
        using value_type = t_value_type;
        using rule_type = stopping_time<value_type>;

        using container_type = std::vector<rule_type*>;
        using iterator_type = detail::iterator_over_pointers<typename container_type::iterator>;
        using const_iterator_type = detail::iterator_over_pointers<typename container_type::const_iterator>;

        static constexpr std::string_view jstr_typename = "type";

        using recognized_a_type = cusum<value_type>;
        using recognized_b_type = finite_moving_average<value_type>;
        using recognized_c_type = sliding_cusum<value_type>;

    private:
        container_type m_rule_ptrs = {};

        template <typename t_rule_type>
        bool try_parse(const nlohmann::json& j) noexcept
        {
            t_rule_type x {};
            if (!ropufu::noexcept_json::try_get(j, x)) return false;

            rule_type* result = new t_rule_type(x);
            this->m_rule_ptrs.push_back(result);
            std::cout << "Allocated stopping_time<value_type> at " << result << std::endl;
            return true;
        } // parse(...)

    public:
        bool try_make(const nlohmann::json& j) noexcept
        {
            std::string typename_string {};
            if (!noexcept_json::required(j, type::jstr_typename, typename_string)) return false;

            if (typename_string == recognized_a_type::typename_string) return this->try_parse<recognized_a_type>(j);
            if (typename_string == recognized_b_type::typename_string) return this->try_parse<recognized_b_type>(j);
            if (typename_string == recognized_c_type::typename_string) return this->try_parse<recognized_c_type>(j);

            return false;
        } // try_make(...)

        ~stopping_time_factory() noexcept
        {
            for (rule_type* r : this->m_rule_ptrs)
            {
                std::cout << "Deallocating stopping_time<value_type> at " << r << std::endl;
                delete r;
            } // for (...)
            this->m_rule_ptrs.clear();
        } // ~stopping_time_factory(...)

        constexpr void reserve(std::size_t capacity) { this->m_rule_ptrs.reserve(capacity); }

        std::size_t size() const noexcept { return this->m_rule_ptrs.size(); }

        rule_type* operator[](std::size_t index) { return this->m_rule_ptrs[index]; }
        const rule_type* operator[](std::size_t index) const { return this->m_rule_ptrs[index]; }

        iterator_type begin() noexcept { return this->m_rule_ptrs.begin(); }
        iterator_type end() noexcept { return this->m_rule_ptrs.end(); }

        const_iterator_type begin() const noexcept { return this->m_rule_ptrs.begin(); }
        const_iterator_type end() const noexcept { return this->m_rule_ptrs.end(); }

        const_iterator_type cbegin() const noexcept { return this->m_rule_ptrs.cbegin(); }
        const_iterator_type cend() const noexcept { return this->m_rule_ptrs.cend(); }
    }; // struct stopping_time_factory
} // namespace ropufu::sequential::intermittent

#endif // ROPUFU_SEQUENTIAL_INTERMITTENT_STOPPING_TIME_FACTORY_HPP_INCLUDED
