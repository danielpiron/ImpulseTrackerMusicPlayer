#ifndef _PLAYER_PATTERN_H_
#define _PLAYER_PATTERN_H_

#include <player/PatternEntry.h>

#include <iostream>
#include <string>
#include <vector>

class Pattern {
  public:
    using Entry = PatternEntry;
    class Channel {
      public:
        Channel(size_t row_count) : _rows(row_count) {}
        Entry& row(size_t r) { return _rows[r]; }
        const std::vector<Entry>& rows() const { return _rows; }

        bool operator==(const Channel& rhs) const { return _rows == rhs._rows; }

      private:
        std::vector<Entry> _rows;
    };

  public:
    Pattern(size_t row_count) : _channels(1, Channel(row_count)) {}
    bool operator==(const Pattern& rhs) const
    {
        return _channels == rhs._channels;
    }

    Channel& channel(size_t c) { return _channels[c]; }
    const Channel& channel(size_t c) const { return _channels[c]; }

  private:
    std::vector<Channel> _channels;
};

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

#endif