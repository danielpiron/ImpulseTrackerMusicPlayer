#ifndef _PLAYER_PATTERN_H_
#define _PLAYER_PATTERN_H_

#include <player/PatternEntry.h>

#include <iostream>
#include <string>
#include <vector>

class Pattern {
  public:
    using PatternEntry = PatternEntry;
    class Channel {
      public:
        Channel(size_t row_count) : _rows(row_count) {}
        PatternEntry& row(size_t r) { return _rows[r]; }
        const std::vector<PatternEntry>& rows() const { return _rows; }

        bool operator==(const Channel& rhs) const { return _rows == rhs._rows; }

      private:
        std::vector<PatternEntry> _rows;
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

bool parse_pattern(const std::string& text, Pattern& pattern)
{
    if (!text.empty()) {
        pattern.channel(0).row(0) = parse_pattern_entry(text);
    }
    return true;
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