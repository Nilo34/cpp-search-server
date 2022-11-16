//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎

#include "request_queue.h"

#include <vector>
#include <string>
#include <deque>

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
{
        // напишите реализацию
        QueryResult dlya_inic;
        dlya_inic.kolvo_empty_request = 0;
        dlya_inic.nalichie_docov = false;
        requests_.push_back(dlya_inic);
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    // напишите реализацию
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(result);
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    // напишите реализацию
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
    AddRequest(result);
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    // напишите реализацию
    return requests_.back().kolvo_empty_request;
}

void RequestQueue::AddRequest(const std::vector<Document>& documents) {
    QueryResult dlya_dobavleniya;
    dlya_dobavleniya.nalichie_docov = documents.empty();
    dlya_dobavleniya.kolvo_empty_request = requests_.back().kolvo_empty_request + dlya_dobavleniya.nalichie_docov;

    if (requests_.size() < min_in_day_) {
        requests_.push_back(dlya_dobavleniya);
        return void();
    }
    if (requests_.front().nalichie_docov) {
        dlya_dobavleniya.kolvo_empty_request--;
    }
    requests_.pop_front();
    requests_.push_back(dlya_dobavleniya);
}