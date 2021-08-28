#ifndef _PATTERN_ENTRY_H_
#define _PATTERN_ENTRY_H_

#include <algorithm>
#include <cstdint>
#include <iostream>

struct PatternEntry {

    using Inst = uint8_t;

    class Note {
      public:
        enum class Type : uint8_t { empty = 253, note_off = 254, note_cut = 255 };
        enum class Name : uint8_t {
            c_natural = 0,
            c_sharp,
            d_natural,
            d_sharp,
            e_natural,
            f_natural,
            f_sharp,
            g_natural,
            g_sharp,
            a_natural,
            a_sharp,
            b_natural
        };

      public:
        Note() : _type(Type::empty) {}

        Note(int raw_value) : _value(static_cast<uint8_t>(clamp_to_playable(raw_value))) {}
        explicit Note(int index, int octave) : _value(static_cast<uint8_t>(octave * 12 + index)) {}
        explicit Note(Name name, int octave) : Note(static_cast<int>(name), octave) {}
        explicit Note(Type type) : _type(type) {}

        static int clamp_to_playable(int v)
        {
            return std::clamp(v, static_cast<int>(Note{Name::c_natural, 0}._value),
                              static_cast<int>(Note{Name::b_natural, 9}._value));
        }

        Name name() const { return static_cast<Name>(index()); }
        int index() const { return _value % 12; }
        int octave() const { return _value / 12; }
        bool is_empty() const { return _type == Type::empty; }
        bool is_note_cut() const { return _type == Type::note_cut; }
        bool is_note_off() const { return _type == Type::note_off; }
        bool is_playable() const
        {
            return _value >= Note{Name::c_natural, 0}._value &&
                   _value <= Note{Name::b_natural, 9}._value;
        }

        bool operator==(const Note& other) const { return _value == other._value; }
        Note operator+(const int offset) const { return Note{_value + offset}; }

      private:
        union {
            uint8_t _value;
            Type _type;
        };
    };

    enum class Command : uint8_t {
        none,
        set_speed,
        jump_to_order,
        break_to_row,
        volume_slide,
        pitch_slide_down,
        pitch_slide_up,
        portamento_to_note,
        vibrato,
        vibrato_and_volume_slide,
        portamento_to_and_volume_slide,
        set_sample_offset,
        set_tempo,
        set_volume
    };

    struct Effect {
        Effect(Command comm = Command::none, int data = 0)
            : comm(comm), data(static_cast<uint8_t>(data))
        {
        }

        bool operator==(const Effect& rhs) const { return comm == rhs.comm && data == rhs.data; }

        Command comm;
        uint8_t data;
    };

    PatternEntry(Note note = Note(), int inst = 0, Effect vol = Effect(), Effect effect = Effect())
        : _note(note), _inst(static_cast<Inst>(inst)), _volume_effect(vol), _effect(effect)
    {
    }

    bool operator==(const PatternEntry& rhs) const
    {
        return _note == rhs._note && _inst == rhs._inst && _volume_effect == rhs._volume_effect &&
               _effect == rhs._effect;
    }

    Note _note;
    Inst _inst = 0;
    Effect _volume_effect;
    Effect _effect;
};

extern std::ostream& operator<<(std::ostream& os, const PatternEntry::Note& note);
extern std::ostream& operator<<(std::ostream& os, const PatternEntry& pe);
extern PatternEntry parse_pattern_entry(const std::string& text);
extern PatternEntry parse_pattern_entry(std::string::const_iterator& curr,
                                        const std::string::const_iterator& last);
extern void skip_whitespace(std::string::const_iterator& curr);

#endif