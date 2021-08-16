#include "Pattern.h"

bool parse_pattern(std::string::const_iterator& start, const std::string::const_iterator& last,
                   Pattern& pattern)
{
    size_t current_row = 0;
    size_t current_channel = 0;
    while (start != last && current_row < pattern.row_count()) {
        pattern.channel(current_channel++).row(current_row) = parse_pattern_entry(start, last);
        // Look for a newline to advance to next row
        while (std::isspace(*start)) {
            if (*start++ == '\n') {
                current_row++;
                current_channel = 0;
                break;
            }
        }
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
    for (size_t c = 1; c <= pattern.channel_count(); ++c) {
        os << "\n"
           << "Channel " << c << "\n";
        os << pattern.channel(c - 1);
    }
    return os;
}