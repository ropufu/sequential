
#ifndef ROPUFU_AFTERMATH_ALGEBRA_MATRIX_MASK_HPP_INCLUDED
#define ROPUFU_AFTERMATH_ALGEBRA_MATRIX_MASK_HPP_INCLUDED

#include <ropufu/algebra/matrix_index.hpp>

#include <cstddef> // std::size_t
#include <utility> // std::swap
#include <vector>  // std::vector

namespace ropufu::aftermath::algebra
{
    template <typename t_size_type, typename t_value_type>
    struct sparse_matrix_cell
    {
        using type = sparse_matrix_cell<t_size_type, t_value_type>;
        using size_type = t_size_type;
        using value_type = t_value_type;
        using index_type = matrix_index<size_type>;

    private:
        index_type m_index = {};
        value_type m_value = {};

    public:
        sparse_matrix_cell() noexcept { }
        sparse_matrix_cell(size_type row, size_type column) noexcept : m_index(row, column) { }

        const index_type& index() const noexcept { return this->m_index; }
        size_type row() const noexcept { return this->m_index.row; }
        size_type column() const noexcept { return this->m_index.column; }

        const value_type& value() const noexcept { return this->m_value; }
        value_type& value() noexcept { return this->m_value; }
    }; // struct sparse_matrix_cell

    template <typename t_size_type>
    struct sparse_matrix_cell<t_size_type, bool>
    {
        using type = sparse_matrix_cell<t_size_type, bool>;
        using size_type = t_size_type;
        using value_type = bool;
        using index_type = matrix_index<size_type>;

    private:
        index_type m_index = {};
        value_type m_value = {};

    public:
        sparse_matrix_cell() noexcept { }
        sparse_matrix_cell(size_type row, size_type column) noexcept : m_index(row, column) { }

        const index_type& index() const noexcept { return this->m_index; }
        size_type row()    const noexcept { return this->m_index.row; }
        size_type column() const noexcept { return this->m_index.column; }

        const value_type& value() const noexcept { return this->m_value; }
        value_type& value() noexcept { return this->m_value; }

        void set()    noexcept { this->m_value = true; }
        void unset()  noexcept { this->m_value = false; }
        void toggle() noexcept { this->m_value = !this->m_value; }
    }; // struct sparse_matrix_cell<...>

    template <typename t_size_type = std::size_t>
    struct matrix_mask
    {
        using type = matrix_mask<t_size_type>;
        using size_type = t_size_type;
        using value_type = sparse_matrix_cell<size_type, bool>;
        using collection_type = std::vector<value_type>;
        using iterator_type = value_type*;
        using const_iterator_type = const value_type*;

    private:
        collection_type m_todo = {};
        std::size_t m_count_completed = 0;

    public:
        matrix_mask() noexcept { }

        matrix_mask(size_type height, size_type width) noexcept
        {
            size_type sz = height * width;
            this->m_todo.reserve(sz);

            for (size_type i = 0; i < height; ++i)
            {
                for (size_type j = 0; j < width; ++j) this->m_todo.emplace_back(i, j);
            } // for (...)
        } // matrix_mask(...)

        std::size_t count_completed() const noexcept { return this->m_count_completed; }
        std::size_t count_remaining() const noexcept { return this->m_todo.size() - this->m_count_completed; }

        std::size_t size() const noexcept { return this->count_remaining(); }
        bool empty() const noexcept { return this->m_todo.empty(); }

        const collection_type& all() const { return this->m_todo; }

        iterator_type begin() noexcept { return this->m_todo.data(); }
        iterator_type end()   noexcept { return this->m_todo.data() + this->count_remaining(); }

        const_iterator_type begin()  const noexcept { return this->m_todo.data(); }
        const_iterator_type end()    const noexcept { return this->m_todo.data() + this->count_remaining(); }
        const_iterator_type cbegin() const noexcept { return this->m_todo.data(); }
        const_iterator_type cend()   const noexcept { return this->m_todo.data() + this->count_remaining(); }

        /** @brief Places all completed elements to the end of the list and increments completion counter accordingly. */
        void commit() noexcept
        {
            std::size_t first_completed = this->count_remaining();
            std::size_t offset = 0;
            for (std::size_t i = 0; i < first_completed; ++i)
            {
                if (!this->m_todo[i - offset].value()) continue; // Skip un-marked cells.

                std::swap(this->m_todo[i - offset], this->m_todo[first_completed - offset - 1]);
                ++this->m_count_completed;
                ++offset;
            } // for (...)
        } // commit(...)

        /** @brief Marks all elements as un-marked. */
        void wipe() noexcept
        {
            for (value_type& x : this->m_todo) x.unset();
            this->m_count_completed = 0;
        } // wipe(...)
    }; // struct matrix_mask
} // namespace ropufu::aftermath::algebra

#endif // ROPUFU_AFTERMATH_ALGEBRA_MATRIX_MASK_HPP_INCLUDED
