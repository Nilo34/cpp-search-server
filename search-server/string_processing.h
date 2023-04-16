#pragma once

#include <string>
#include <vector>
#include <set>
#include <string_view>


std::vector<std::string_view> SplitIntoWords(const std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    std::string inserted_string;
    for (const auto& str : strings) {
        inserted_string = str;
        if (!inserted_string.empty()) {
            non_empty_strings.insert(inserted_string);
        }
    }
    return non_empty_strings;
}