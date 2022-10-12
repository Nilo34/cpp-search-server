#include <algorithm>
#include <iostream>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <numeric>
#include <vector>

#include "search_server.h"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

/*const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename KeyMapper>
    vector<Document> FindTopDocuments(const string& raw_query, KeyMapper key_mapper) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key_mapper);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const float minimum_comparison_difference = 1e-6;
                if (abs(lhs.relevance - rhs.relevance) < minimum_comparison_difference) {
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

    vector<Document> FindTopDocuments(const string& raw_query,
        DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status] (int, DocumentStatus filter_status, int) {
            return filter_status == status; });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename KeyMapper>
    vector<Document> FindAllDocuments(const Query& query, KeyMapper key_mapper) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& doc_dat = documents_.at(document_id);
                if (key_mapper(document_id, doc_dat.status, doc_dat.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

};*/

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/

/*template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Func_Testa>
void RunTestImpl(Func_Testa func_testa, const string& func_testa_str) {
    func_testa();
    cerr << func_testa_str <<" OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)*/

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/

// тест проверяющий добавление документа

void ProverkaDobavleniyaFaila() {
    const int doc_id = 42;
    const string content = "cat city pushistiy blochastick"s;
    const vector<int> ratings = {1, 2, 3};
    
    
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("pushistiy cat blochastick"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, doc_id, "Incorrect document entry"s);
}

// тест проверяющий поддержку минус слов

void ProverkaRabotyMinusSlov() {
    const int doc_id = 42;
    const string content = "cat city pushistiy blochastick"s;
    const vector<int> ratings = {1, 2, 3};
    
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        
    const int doc_id2 = 31;
    const string content2 = "pushistiy blochastick orck zeleniy"s;
    const vector<int> ratings2 = {5, 2, 6};
        
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    
    auto found_docs = server.FindTopDocuments("pushistiy -cat blochastick"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, doc_id2, "Incorrect processing of minus words"s);
    
    found_docs = server.FindTopDocuments("pushistiy cat blochastick"s);
    ASSERT_EQUAL(found_docs.size(), 2u);
}

// тест проверяющий матчинг

void ProverkaMatchinga() {
    const int doc_id = 42;
    const string content = "cat city pushistiy blochastick"s;
    const vector<int> ratings = {1, 2, 3};
    
    
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    auto found_docs = server.MatchDocument("pushistiy cat nosorock blochastick"s, doc_id);
    vector<string> teor_itog = {"blochastick"s, "cat"s, "pushistiy"s};
    ASSERT_EQUAL_HINT(get<0>(found_docs), teor_itog, "Wrong Matching"s);
        
    found_docs = server.MatchDocument("pushistiy -cat nosorock blochastick"s, doc_id);
    teor_itog.clear();
    ASSERT_EQUAL_HINT(get<0>(found_docs), teor_itog, "Wrong Matching minus words"s);
}

// тест проверяющий сортировку

void ProverkaSortirovkiPoRelev() {
    
    const int doc_id2 = 32;
    const string content2 = "city pushistiy blochastick real pudel"s;
    const vector<int> ratings2 = {1, 2, 3};
    
    const int doc_id = 42;
    const string content = "cat city pushistiy blochastick"s;
    const vector<int> ratings = {5, 2, 3};
    
    SearchServer server;
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("pushistiy cat blochastick"s);
    ASSERT_EQUAL(found_docs.size(), 2u);
    const Document& doc0 = found_docs[0];
    const Document& doc1 = found_docs[1];
    ASSERT_EQUAL_HINT(doc0.id, doc_id, "Incorrect sorting by relevance"s);
    ASSERT_EQUAL_HINT(doc1.id, doc_id2, "Incorrect sorting by relevance"s);
}

// тест проверяющий вычисление рейтинга

void ProverkaRatinga() {
    const int doc_id = 42;
    const string content = "cat city pushistiy blochastick"s;
    const vector<int> ratings = {5, 2, 3};
    
    
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("pushistiy cat blochastick"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.rating, 3, "Incorrect rating calculation"s);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(ProverkaDobavleniyaFaila);
    RUN_TEST(ProverkaRabotyMinusSlov);
    RUN_TEST(ProverkaMatchinga);
    RUN_TEST(ProverkaSortirovkiPoRelev);
    RUN_TEST(ProverkaRatinga);
    //RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent); // проверка на предикат
    //RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent); // Поиск по статусу
    //RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent); // проверка релевантности
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
