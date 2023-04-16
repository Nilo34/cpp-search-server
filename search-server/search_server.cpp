#include "search_server.h"
#include "document.h"
#include "string_processing.h"

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <numeric>
#include <map>
#include <execution>
#include <utility>
#include <string_view>

#include <iostream>

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{}
SearchServer::SearchServer(const std::string_view& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{}


void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        using namespace std::string_literals;
        throw std::invalid_argument("Invalid document_id"s);
    }
    
    auto [doc_it, _] = documents_.emplace(document_id, DocumentData{std::string(document), ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
    
    const auto words = SplitIntoWordsNoStop(doc_it->second.content);
    
    const double inv_word_count = 1.0 / words.size();
    for (const auto& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, 
                                                     DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, 
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, 
                                                     const std::string_view& raw_query, 
                                                     DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, 
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, 
                                                     const std::string_view& raw_query, 
                                                     DocumentStatus status) const {
    return FindTopDocuments(std::execution::par, 
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, 
                                                     const std::string_view& raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, 
                                                     const std::string_view& raw_query) const {
    return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query,
                                                                   int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, 
                            const std::string_view& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    
    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { {}, documents_.at(document_id).status };
        }
    }
    
    std::vector<std::string_view> matched_words;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, 
                            const std::string_view& raw_query, int document_id) const {
    // PARALELKA
    const auto query = ParseQuery(raw_query, true);
    
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
                    [&] (const auto& minus_word) {
                        const auto it = word_to_document_freqs_.find(minus_word);
                        return it != word_to_document_freqs_.end() && it->second.count(document_id);
                    })) {
        return {{},documents_.at(document_id).status};
    }
    
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto it_matched_words_end =  std::copy_if(std::execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        [&](const auto& plus_word) {
            const auto it = word_to_document_freqs_.find(plus_word);
            return it != word_to_document_freqs_.end() && it->second.count(document_id);
        }
    );
    
    std::sort(std::execution::par, matched_words.begin(), it_matched_words_end);
    matched_words.erase(std::unique(std::execution::par,matched_words.begin(), it_matched_words_end), matched_words.end());
    
    return { matched_words, documents_.at(document_id).status };
}


std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}


std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}


const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    const auto it = document_to_word_freqs_.find(document_id);
    if (it != document_to_word_freqs_.end()) {
        return it -> second;
    }
    
    static const std::map<std::string_view, double> void_map;
    return void_map;
}


void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}


void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    auto word_list_document = GetWordFrequencies(document_id);
    if (word_list_document.empty()) {
        return;
    }
    documents_.erase(document_id);
    for (auto& [word, _] : word_list_document) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
}


void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    
    const auto& it = document_to_word_freqs_.find(document_id);
    if (it == document_to_word_freqs_.end()) {
        return;
    }
    
    std::vector<const std::string_view*> deleted_word_of_document(it->second.size());
    
    std::transform(std::execution::par,
                  it->second.begin(), it->second.end(),
                  deleted_word_of_document.begin(),
                  [] (const auto& pair_word_freq_in_doc) {
                      return &pair_word_freq_in_doc.first;
                  });
    
    std::for_each(std::execution::par,
                  deleted_word_of_document.begin(), deleted_word_of_document.end(), 
                  [&] (const std::string_view* word) {
                      word_to_document_freqs_[*word].erase(document_id);
                  });
    
    documents_.erase(document_id);
    
    document_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
}


bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}
bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}


std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            using namespace std::string_literals;
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}


int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view& text) const {
    using namespace std::string_literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        word = text.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(word) + " is invalid"s);
    }

    return { word, is_minus, IsStopWord(word) };
}


SearchServer::Query SearchServer::ParseQuery(const std::string_view& text, bool do_not_sort) const {
    Query result;
    for (const std::string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    
    if (!do_not_sort) {
        std::sort(std::execution::par, result.minus_words.begin(), result.minus_words.end());
        std::sort(std::execution::par, result.plus_words.begin(), result.plus_words.end());
        
        result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
        result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
    }
    
    return result;
}


double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}