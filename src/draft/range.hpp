
#ifndef ROPUFU_AFTERMATH_RANGE_HPP_INCLUDED
#define ROPUFU_AFTERMATH_RANGE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include "quiet_json.hpp"

#include <aftermath/not_an_error.hpp> // aftermath::quiet_error

#include <cmath>    // std::log10, std::pow
#include <cstddef>  // std::size_t
#include <cstdint>  // std::int_fast32_t
#include <initializer_list> // std::initializer_list
#include <iostream> // std::ostream
#include <string>   // std::string, std::to_string
#include <vector>   // std::vector

namespace ropufu
{
    namespace aftermath
    {
        enum struct spacing : std::int_fast32_t
        {
            linear = 0,
            logarithmic = 1,
            exponential = 2
        }; // enum struct spacing

        bool try_parse(const std::string& str, spacing& value) noexcept
        {
            if (str == "linear" || str == "lin") { value = spacing::linear; return true; }
            if (str == "logarithmic" || str == "log") { value = spacing::logarithmic; return true; }
            if (str == "exponential" || str == "exp") { value = spacing::exponential; return true; }
            return false;
        } // try_parse(...)

        namespace detail
        {
            template <typename t_data_type, typename t_container_type>
            struct range_container
            {
                using type = range_container<t_data_type, t_container_type>;
                using data_type = t_data_type;
                using container_type = t_container_type;

                static container_type make_empty(std::size_t count = 0) noexcept;
                static container_type make_init(std::initializer_list<data_type>) noexcept;
                static void shrink(container_type& container) noexcept;
            }; // struct range_container

            template <typename t_data_type>
            struct range_container<t_data_type, std::vector<t_data_type>>
            {
                using type = range_container<t_data_type, std::vector<t_data_type>>;
                using data_type = t_data_type;
                using container_type = std::vector<t_data_type>;

                static container_type make_empty(std::size_t count = 0) noexcept { return container_type(count); }
                static container_type make_init(std::initializer_list<data_type> data) noexcept { return container_type(data); }
                static void shrink(container_type& container) noexcept { container.shrink_to_fit(); };
            }; // struct range_container<...>
        } // namespace detail

        /** @brief Inspired by MATLAB's linspace function. */
        template <typename t_data_type>
        struct range
        {
            using type = range<t_data_type>;
            using data_type = t_data_type;

            // ~~ Json names ~~
            static constexpr char jstr_from[] = "from";
            static constexpr char jstr_to[] = "to";

        private:
            data_type m_from = { };
            data_type m_to = { };

        public:
            range() noexcept { }
            
            range(const data_type& from, const data_type& to) noexcept
                : m_from(from), m_to(to)
            {
            } // range(...)

            const data_type& from() const noexcept { return this->m_from; }
            const data_type& to() const noexcept { return this->m_to; }

            template <typename t_container_type>
            void explode(t_container_type& container, std::size_t count, spacing transform = spacing::linear) const noexcept
            {
                switch (transform)
                {
                    case spacing::linear: this->explode(container, count, [] (const data_type& x) { return x; }, [] (const data_type& x) { return x; }); break;
                    case spacing::logarithmic: this->explode(container, count, [] (const data_type& x) { return std::log10(x); }, [] (const data_type& x) { return std::pow(10, x); }); break;
                    case spacing::exponential: this->explode(container, count, [] (const data_type& x) { return std::pow(10, x); }, [] (const data_type& x) { return std::log10(x); }); break;
                } // switch (...)
            } // explode(...)

            template <typename t_container_type, typename t_forward_transform_type, typename t_backward_transform_type>
            void explode(t_container_type& container, std::size_t count, const t_forward_transform_type& forward, const t_backward_transform_type& backward) const noexcept
            {
                using helper_type = detail::range_container<data_type, t_container_type>;

                switch (count)
                {
                    case 0: container = helper_type::make_empty(); return;
                    case 1: container = helper_type::make_init({ this->m_from }); return;
                    case 2: container = helper_type::make_init({ this->m_from, this->m_to }); return;
                } // switch (...)

                container = helper_type::make_empty(count);

                data_type f_from = forward(this->m_from);
                data_type f_to = forward(this->m_to);
                data_type f_range = f_to - f_from;

                std::size_t i = 0;
                bool is_first = true;

                for (data_type& x : container)
                {
                    bool is_last = (i == count - 1);

                    if (is_first) x = this->m_from;
                    else if (is_last) x = this->m_to;
                    else
                    {
                        data_type f_step = (i * f_range) / (count - 1);
                        x = backward(f_from + f_step);
                    }
                    ++i;
                    is_first = false;
                }
                helper_type::shrink(container);
            } // explode(...)

            /** Checks if the two objects are equal. */
            bool operator ==(const type& other) const noexcept
            {
                return
                    this->m_from == other.m_from &&
                    this->m_to == other.m_to;
            } // operator ==(...)

            /** Checks if the two objects are not equal. */
            bool operator !=(const type& other) const noexcept { return !this->operator ==(other); }

            /** @brief Output to a stream. */
            friend std::ostream& operator <<(std::ostream& os, const type& self) noexcept
            {
                nlohmann::json j = self;
                return os << j;
            } // operator <<(...)
        }; // struct range

        // ~~ Json name definitions ~~
        template <typename t_data_type> constexpr char range<t_data_type>::jstr_from[];
        template <typename t_data_type> constexpr char range<t_data_type>::jstr_to[];
        
        template <typename t_data_type>
        void to_json(nlohmann::json& j, const range<t_data_type>& x) noexcept
        {
            using type = range<t_data_type>;

            j = nlohmann::json{
                {type::jstr_from, x.from()},
                {type::jstr_to, x.to()}
            };
        } // to_json(...)
    
        template <typename t_data_type>
        void from_json(const nlohmann::json& j, range<t_data_type>& x) noexcept
        {
            quiet_json q(j);
            using type = range<t_data_type>;

            // Populate default values.
            t_data_type from = x.from();
            t_data_type to = x.to();
            std::vector<t_data_type> v = { from, to };

            // Parse json entries.
            if (j.is_array()) q.interpret_as(v);
            else
            {
                q.required(type::jstr_from, from);
                q.required(type::jstr_to, to);
                v = { from, to };
            } // else
            
            // Reconstruct the object.
            if (!q.good())
            {
                aftermath::quiet_error::instance().push(
                    aftermath::not_an_error::runtime_error,
                    aftermath::severity_level::major, 
                    q.message(), __FUNCTION__, __LINE__);
                return;
            } // if (...)
            if (v.size() != 2)
            {
                aftermath::quiet_error::instance().push(
                    aftermath::not_an_error::logic_error,
                    aftermath::severity_level::major,
                    "Range should be a vector with two entries.", __FUNCTION__, __LINE__);
                return;
            }
            x = type(v.front(), v.back());
        } // from_json(...)
    } // namespace aftermath
} // namespace ropufu

namespace std
{
    std::string to_string(ropufu::aftermath::spacing value) noexcept
    {
        using type = ropufu::aftermath::spacing;

        switch (value)
        {
            case type::linear: return "linear";
            case type::logarithmic: return "logarithmic";
            case type::exponential: return "exponential";
            default: return "unknown";
        } // switch (...)
    } // to_string(...)
} // namespace std

#endif // ROPUFU_AFTERMATH_RANGE_HPP_INCLUDED
