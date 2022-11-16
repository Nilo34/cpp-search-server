#pragma once

#include "document.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <string>
#include <cassert>

template <typename Iterator>
class IteratorRange {
public:

    IteratorRange(Iterator begining, Iterator ending);

    Iterator begin() const;

    Iterator end() const;

    size_t size() const;

private:
    Iterator begin_;
    Iterator end_;
    size_t size_ = 0;
};

template <typename Iterator>
class Paginator {
public:

    explicit Paginator(Iterator begining, Iterator ending, size_t page_size);

    auto begin() const;

    auto end() const;

    size_t size() const;

private:
    std::vector<IteratorRange<Iterator>> pages_;

};

//template <typename Iterator>
//std::ostream& operator<< (std::ostream& os, const IteratorRange<Iterator>& iterator_range);

template <typename Container>
auto Paginate(const Container& c, size_t page_size);

template <typename Iterator>
IteratorRange<Iterator>::IteratorRange(Iterator begining, Iterator ending)
    : begin_(begining),
end_(ending),
size_(distance(begining, ending))
{}

template <typename Iterator>
Iterator IteratorRange<Iterator>::begin() const {
    return begin_;
}

template <typename Iterator>
Iterator IteratorRange<Iterator>::end() const {
    return end_;
}

template <typename Iterator>
size_t IteratorRange<Iterator>::size() const {
    return size_;
}

template <typename Iterator>
Paginator<Iterator>::Paginator(Iterator begining, Iterator ending, size_t page_size) {
    if ((ending <= begining) || (page_size == 0)) {
        using namespace std::string_literals;
        throw std::invalid_argument("Incorrect page parameters"s);
    }
    
    for (size_t ostalos_elem = distance(begining, ending); ostalos_elem > 0;) {
        const size_t razmer_str = std::min(ostalos_elem, page_size);
        const Iterator konec_str = std::next(begining, razmer_str);
        pages_.push_back(IteratorRange{ begining, konec_str });
        ostalos_elem -= razmer_str;
        begining = konec_str;
    }
}

template <typename Iterator>
auto Paginator<Iterator>::begin() const {
    return pages_.begin();
}

template <typename Iterator>
auto Paginator<Iterator>::end() const {
    return pages_.end();
}

template <typename Iterator>
size_t Paginator<Iterator>::size() const {
    return pages_.size();
}

template <typename Iterator>
std::ostream& operator<< (std::ostream& os, const IteratorRange<Iterator>& iterator_range) {
    for (auto it = iterator_range.begin(); it != iterator_range.end(); ++it) {
        os << *it;
    }
    return os;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}