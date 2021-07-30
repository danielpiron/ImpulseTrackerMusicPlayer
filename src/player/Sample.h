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
    Sample(Iterator b, Iterator e, size_t loopBegin_ = 0, size_t loopEnd_ = 0)
        : _data(b, e), _loop{LoopParams::Type::forward_looping, loopBegin_,
                             loopEnd_ ? loopEnd_ : _data.size()}
    {
    }
    Sample(std::initializer_list<float> il,
           LoopParams loopParams = LoopParams())
        : _data(il), _loop{loopParams.type, loopParams.begin,
                           loopParams.end ? loopParams.end : _data.size()}
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

  private:
    std::vector<float> _data;
    LoopParams _loop;
};

#endif