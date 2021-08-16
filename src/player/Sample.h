#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include <vector>

class Sample {
    struct Loop {
        enum class Type { none };
        size_t begin;
        size_t end;
    };

  public:
    struct LoopParams {
        enum class Type { non_looping, forward_looping };

        LoopParams(Type t = Type::forward_looping, size_t b = 0, size_t e = 0)
            : type(t), begin(b), end(e)
        {
        }
        Type type;
        size_t begin;
        size_t end;
    };

  public:
    template <typename Iterator>
    Sample(Iterator b, Iterator e, size_t playbackRate, LoopParams loopParams = LoopParams())
        : _data(b, e),
          _playbackRate(playbackRate),
          _loop{loopParams.type, loopParams.begin, loopParams.end ? loopParams.end : _data.size()}
    {
    }
    Sample(std::initializer_list<float> il, size_t playbackRate,
           LoopParams loopParams = LoopParams())
        : _data(il),
          _playbackRate(playbackRate),
          _loop{loopParams.type, loopParams.begin, loopParams.end ? loopParams.end : _data.size()}
    {
    }

    Sample(const Sample&& other)
        : _data(other._data), _playbackRate(other._playbackRate), _loop(other._loop)
    {
    }

    inline float operator[](float i) const
    {
        auto wholeI = static_cast<size_t>(i);
        float t = i - static_cast<float>(wholeI);
        size_t nextIndex = wholeI + 1;
        if (nextIndex >= loopEnd()) {
            nextIndex -= loopLength();
        }
        float v0 = _data[wholeI];
        float v1 = _data[nextIndex];
        return v0 + t * (v1 - v0);
    }
    inline float operator[](size_t i) const { return _data[i]; }
    inline size_t length() const { return _data.size(); }
    inline LoopParams::Type loopType() const { return _loop.type; }
    inline size_t loopBegin() const { return _loop.begin; }
    inline size_t loopEnd() const { return _loop.end; }
    inline size_t loopLength() const { return loopEnd() - loopBegin(); }
    inline size_t playbackRate() const { return _playbackRate; }

  private:
    std::vector<float> _data;
    size_t _playbackRate;
    LoopParams _loop;
};

#endif