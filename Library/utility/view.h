#ifndef SLIM_UTILITY_VIEW_H
#define SLIM_UTILITY_VIEW_H

#include <vector>

namespace slim {

    template <typename T, typename Container = std::vector<T>> class View;
    template <typename T, typename Container = std::vector<T>> class ViewIterator;

    // ----------------------------------------------------------

    template <typename T, typename Container>
    class ViewIterator {
    public:
        using Iterator = typename Container::iterator;
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;  // or also value_type*
        using reference         = T&;  // or also value_type&

        explicit ViewIterator(const View<T, Container>* view, Iterator iterator, uint32_t index)
            : view(view), iterator(iterator), index(index) {
            // initialize basic properties
        }

        // post increment
        ViewIterator& operator++() {
            // for empty ranges
            if (!view)
                return *this;

            // check if this iterator reaches the end of the range
            if (++iterator == view->ranges[index].second) {
                if (++index < view->ranges.size()) {
                    iterator = view->ranges[index].first;
                }
            }
            return *this;
        }

        reference operator*() const {
            return *iterator;
        }

        pointer operator->() {
            return iterator;
        }

        friend bool operator==(const ViewIterator& a, const ViewIterator& b) {
            return a.iterator == b.iterator;
        }

        friend bool operator!=(const ViewIterator& a, const ViewIterator& b) {
            return !(a == b);
        }

    private:
        const View<T, Container>* view;
        Iterator iterator;
        uint32_t index;
    };

    // ----------------------------------------------------------

    template <typename T, typename Container>
    class View final {
        friend class ViewIterator<T, Container>;
    public:
        using Iterator = typename Container::iterator;
        using Range = std::pair<Iterator, Iterator>;

        explicit View() = default;
        virtual ~View() = default;

        void Clear() {
            ranges.clear();
        }

        void Concat(const Iterator& begin, const Iterator& end) {
            ranges.push_back(std::make_pair(begin, end));
        }

        ViewIterator<T, Container> begin() const {
            return ViewIterator<T, Container>(this, ranges[0].first, 0);
        }

        ViewIterator<T, Container> end() const {
            return ViewIterator<T, Container>(this, ranges.back().second, ranges.size() - 1);
        }

    private:
        std::vector<Range> ranges;
    };

} // end of namespace slim

#endif // SLIM_UTILITY_VIEW_H
