#pragma once

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <execution>
#include <string_view>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double COMPARISON_LIMIT = 1e-6;

using Match_Document = std::tuple<std::vector<std::string_view>, DocumentStatus>;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(const std::string_view& stop_words_text);
    
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, 
                                           DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& execution_policy, 
                                           const std::string_view& raw_query, 
                                           DocumentPredicate document_predicate) const;
    
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, 
                                           DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& execution_policy, 
                                           const std::string_view& raw_query, 
                                           DocumentStatus status) const;
    
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& execution_policy, 
                                           const std::string_view& raw_query) const;
    

    int GetDocumentCount() const;
    
    Match_Document MatchDocument(const std::string_view& raw_query, 
                                 int document_id) const;
    Match_Document MatchDocument(const std::execution::sequenced_policy&,
                                 const std::string_view& raw_query, 
                                 int document_id) const;
    Match_Document MatchDocument(const std::execution::parallel_policy&,
                                 const std::string_view& raw_query, 
                                 int document_id) const;
    
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    
private:
    struct DocumentData {
        std::string content;
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    
    bool IsStopWord(const std::string_view word) const;
    static bool IsValidWord(const std::string_view word);
    
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    
    QueryWord ParseQueryWord(const std::string_view& text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    
    Query ParseQuery(const std::string_view& text, bool is_not_sort = false) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, 
                                           DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& execution_policy, 
                                           const Query& query, 
                                           DocumentPredicate document_predicate) const;
};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    using namespace std::string_literals;
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, 
                                                     DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}
template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& execution_policy, 
                                                     const std::string_view& raw_query, 
                                                     DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    
    auto matched_documents = FindAllDocuments(execution_policy, query, document_predicate);
    
    std::sort(execution_policy, 
         matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < COMPARISON_LIMIT) {
                 return lhs.rating > rhs.rating;
             }
             else {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    
    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& execution_policy, 
                                                     const std::string_view& raw_query, 
                                                     DocumentStatus status) const {
    return FindTopDocuments(execution_policy, 
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& execution_policy, 
                                                     const std::string_view& raw_query) const {
    return FindTopDocuments(execution_policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, 
                                                     DocumentPredicate document_predicate) const {
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}
template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& execution_policy, 
                                                     const Query& query, 
                                                     DocumentPredicate document_predicate) const {
    const size_t QUANTITY_BUKETS = 8;
    ConcurrentMap<int, double> document_to_relevance_concurrent_map(QUANTITY_BUKETS);
    
    const auto function_for_plus_words = [&] (std::string_view word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance_concurrent_map[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    };
    
    std::for_each(execution_policy, query.plus_words.begin(), 
                  query.plus_words.end(), function_for_plus_words);
    
    const auto function_for_minus_words = [&] (std::string_view word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance_concurrent_map.erase(document_id);
        }
    };
    
    std::for_each(execution_policy, query.minus_words.begin(), 
                  query.minus_words.end(), function_for_minus_words);
    
    std::map<int, double> document_to_relevance = document_to_relevance_concurrent_map.BuildOrdinaryMap();
    
    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}