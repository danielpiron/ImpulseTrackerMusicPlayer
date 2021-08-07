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
        const Entry& row(size_t r) const { return _rows[r]; }

        const std::vector<Entry>& rows() const { return _rows; }

        bool operator==(const Channel& rhs) const { return _rows == rhs._rows; }

      private:
        std::vector<Entry> _rows;
    };

  public:
    Pattern(size_t row_count)
        : _channels(1, Channel(row_count)), _row_count(row_count)
    {
    }
    bool operator==(const Pattern& rhs) const
    {
        return _channels == rhs._channels;
    }

    Channel& channel(size_t c) { return _channels[c]; }
    const Channel& channel(size_t c) const { return _channels[c]; }
    size_t row_count() const { return _row_count; }

  private:
    std::vector<Channel> _channels;
    size_t _row_count;
};

extern bool parse_pattern(std::string::const_iterator& start,
                          const std::string::const_iterator& last,
                          Pattern& pattern);
extern bool parse_pattern(const std::string& text, Pattern& pattern);
extern std::ostream& operator<<(std::ostream& os,
                                const Pattern::Channel& channel);
extern std::ostream& operator<<(std::ostream& os, const Pattern& pattern);

#endif
