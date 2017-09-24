
#ifndef ROPUFU_SEQUENTIAL_HYPOTHESES_QUIET_RETURN_HPP_INCLUDED
#define ROPUFU_SEQUENTIAL_HYPOTHESES_QUIET_RETURN_HPP_INCLUDED

namespace ropufu
{
    namespace sequential
    {
        namespace hypotheses
        {
            enum struct not_an_error
            {
                all_good,         // Indicates no error.
                logic_error,      // Indicates violations of logical preconditions or class invariants.
                invalid_argument, // Reports invalid arguments.
                domain_error,     // Reports domain errors.
                length_error,     // Reports attempts to exceed maximum allowed size.
                out_of_range,     // Reports arguments outside of expected range.
                runtime_error,    // Indicates conditions only detectable at run time.
                range_error,      // Reports range errors in internal computations.
                overflow_error,   // Reports arithmetic overflows.
                underflow_error   // Reports arithmetic underflows.
            };

            template <typename t_return_type = void>
            struct quiet_return
            {
                typedef quiet_return<t_return_type> type;
                typedef t_return_type return_type;

            private:
                return_type m_return_value;
                not_an_error m_error = not_an_error::all_good;

            public:
                quiet_return(const return_type& return_value) noexcept
                    : m_return_value(return_value)
                {
                }

                quiet_return(return_type&& return_value) noexcept
                    : m_return_value(return_value)
                {
                }

                quiet_return(const return_type& return_value, not_an_error error) noexcept
                    : m_return_value(return_value), m_error(error)
                {
                }

                quiet_return(return_type&& return_value, not_an_error error) noexcept
                    : m_return_value(return_value), m_error(error)
                {
                }

                operator return_type() const noexcept { return this->m_return_value; }

                const return_type& value() const noexcept { return this->m_return_value; }
                not_an_error error() const noexcept { return this->m_error; }
            };

            template <>
            struct quiet_return<void>
            {
                typedef quiet_return<void> type;
                typedef void return_type;

            private:
                not_an_error m_error = not_an_error::all_good;

            public:
                quiet_return(not_an_error error) noexcept
                    : m_error(error)
                {
                }

                operator not_an_error() const noexcept { return this->m_error; }

                void value() const noexcept { }
                not_an_error error() const noexcept { return this->m_error; }
            };
        }
    }
}

#endif // ROPUFU_SEQUENTIAL_HYPOTHESES_QUIET_RETURN_HPP_INCLUDED
