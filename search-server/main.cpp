#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
        } else {
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
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        int words_quantity = words.size();
        for (const string& word : words) {
            if (documents_[word][document_id] == 0) {
                documents_[word][document_id] = static_cast<double>(count(words.begin(), words.end(), word)) / words_quantity;
            }
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const QueryWords query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    
    struct QueryWords {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    map<string, map<int, double>> documents_;
    
    int document_count_ = 0;

    set<string> stop_words_;

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

    QueryWords ParseQuery(const string& text) const {
        QueryWords query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                query_words.minus_words.insert(word.substr(1, word.size()));
            } else {
                query_words.plus_words.insert(word);
            }
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const QueryWords& query_words) const {
        vector<Document> matched_documents;
        const map<int, double> relevance = MatchDocument(documents_, document_count_, query_words);
        for (const auto& rel : relevance){
            if (rel.second > 0) {
                matched_documents.push_back({rel.first, rel.second});
            }
        }
        return matched_documents;
    }
    static map<int, double> MatchDocument(const map<string, map<int, double>>& content, const int document_count,
                             const QueryWords& query_words) {
        map<int, double> resalt;
        if (query_words.plus_words.empty()) {
            return resalt;
        }
        for (const string& plus_word : query_words.plus_words) {
            if (content.count(plus_word) == 0) {
                continue;
            } else {
                double IDF = log(static_cast<double>(document_count) / content.at(plus_word).size());
                for (const auto& [id_doc, TF] : content.at(plus_word)) {
                    resalt[id_doc] +=  IDF * TF;
                }
            }
        }
        if (!query_words.minus_words.empty()) {
            for (const string& minus_word : query_words.minus_words) {
                if (content.count(minus_word) == 0) {
                    continue;
                } else {
                    for (const auto& [id_doc, TF] : content.at(minus_word)) {
                        resalt[id_doc] =  0;
                    }
                }
            }
        }
        
        return resalt;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
