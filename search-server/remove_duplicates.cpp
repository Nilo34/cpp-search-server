#include "remove_duplicates.h"
#include "search_server.h"

#include <string>
#include <iostream>
#include <set>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> list_of_documents;
    for (const int document_id : search_server) {
        std::set<int> list_of_dup_doc = search_server.IsDuplicate(document_id);
        list_of_documents.insert(list_of_dup_doc.begin(), list_of_dup_doc.end());
    }
    
    for (const int id_dup : list_of_documents) {
        using namespace std::string_literals;
        
        search_server.RemoveDocument(id_dup);
        std::cout << "Found duplicate document id "s << id_dup << std:: endl;
    }
}