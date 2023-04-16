#include "string_processing.h"

#include <string>
#include <vector>


std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    
    std::vector<std::string_view> words;
    const std::string_view space = " ";
    size_t slider = text.find_first_not_of(space);
    const size_t end_text = text.npos;
    
    while (slider != end_text) {
        size_t next_space_pos = text.find(space, slider);
        if (next_space_pos != end_text) {
            words.push_back(text.substr(slider, next_space_pos - slider));
        }
        else {
            words.push_back(text.substr(slider));
        }
        
        slider = text.find_first_not_of(space, next_space_pos);
    }
    
    return words;
}