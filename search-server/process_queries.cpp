#include "process_queries.h"

#include <algorithm>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, 
              queries.begin(), queries.end(),
              result.begin(),
              [&search_server] (const std::string s) {
                  return search_server.FindTopDocuments(s);
              });
    return result;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    std::list<Document> result;
    
    for (std::vector<Document> document : ProcessQueries(search_server, queries)) {
        result.insert(result.cend(), document.begin(), document.end());
    }
    
    return result;
}