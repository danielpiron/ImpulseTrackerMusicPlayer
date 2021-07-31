#include "Pattern.h"

bool parse_pattern(std::string::const_iterator& start,
                   const std::string::const_iterator& last, Pattern& pattern)
{
    size_t current_row = 0;
    while (start != last && current_row < 8) {
        std::cout << "Reading row " << current_row << "\n";
        std::cout << "Text Remaining: " << std::string(start, last) << "\n";
        pattern.channel(0).row(current_row++) =
            parse_pattern_entry(start, last);
        skip_whitespace(start);
    }
    return true;
}

bool parse_pattern(const std::string& text, Pattern& pattern)
{
    if (text.empty()) {
        return true;
    }
    auto start = text.begin();
    return parse_pattern(start, text.end(), pattern);
}

std::ostream& operator<<(std::ostream& os, const Pattern::Channel& channel)
{
    for (const auto entry : channel.rows()) {
        os << entry << "\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Pattern& pattern)
{
    os << "\n";
    os << pattern.channel(0);
    return os;
}