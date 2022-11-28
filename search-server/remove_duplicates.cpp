#include "remove_duplicates.h"
#include "search_server.h"

#include <string>
#include <iostream>
#include <set>
#include <vector>
#include <utility>
#include <algorithm>

void RemoveDuplicates(SearchServer& search_server) {
    
    if (search_server.begin() == search_server.end()) {
        return;
    }
    
    std::set<std::set<std::string>> list_of_line_sets;
    std::vector<int> list_of_documents_for_deletion;
    for (const int document_id : search_server) {
        std::set<std::string> string_sets;
        for (const auto& [word, _] : search_server.GetWordFrequencies(document_id)) {
            string_sets.insert(word);
        }
        if (list_of_line_sets.count(string_sets) > 0) {
            list_of_documents_for_deletion.push_back(document_id);
        }
        list_of_line_sets.insert(string_sets);
    }
    
    for (const int id_duplicate : list_of_documents_for_deletion) {
        using namespace std::string_literals;
        
        search_server.RemoveDocument(id_duplicate);
        std::cout << "Found duplicate document id "s << id_duplicate << std:: endl;
    }
}