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
        return void();
    }
    
    std::vector<std::pair<std::set<std::string>, int>> list_of_line_sets;
    for (const int document_id : search_server) {
        std::set<std::string> string_sets;
        for (const auto& [word, _] : search_server.GetWordFrequencies(document_id)) {
            string_sets.insert(word);
        }
        
        list_of_line_sets.push_back({string_sets, document_id});
    }
    
    std::sort(list_of_line_sets.begin(), list_of_line_sets.end());
    
    std::vector<int> list_of_documents_for_deletion;
    std::set<std::string> set_of_lines_for_comparison;
    
    for (const auto& [string_sets, id] : list_of_line_sets) {
        if (set_of_lines_for_comparison == string_sets) {
            list_of_documents_for_deletion.push_back(id);
        } else {
            set_of_lines_for_comparison = string_sets;
        }
    }
    
    for (const int id_duplicate : list_of_documents_for_deletion) {
        using namespace std::string_literals;
        
        search_server.RemoveDocument(id_duplicate);
        std::cout << "Found duplicate document id "s << id_duplicate << std:: endl;
    }
}