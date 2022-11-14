#pragma once

#include "search_server.h"
#include "paginator.h"
#include "read_input_functions.h"

#include <vector>
#include <string>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        // определите, что должно быть в структуре
        int kolvo_empty_request;
        bool nalichie_docov;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;

    void AddRequest(const std::vector<Document>& documents);
};

template <typename DocumentPredicate>
std::vector<Document>  RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        // напишите реализацию
        std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
        AddRequest(result);
        return result;
}