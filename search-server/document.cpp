//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎

#include "document.h"

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

std::ostream& operator<< (std::ostream& os, const Document& document) {
    using namespace std::string_literals;
    os << "{ document_id = "s << document.id
        << ", relevance = "s << document.relevance
        << ", rating = "s << document.rating
        << " }"s;
    return os;
}