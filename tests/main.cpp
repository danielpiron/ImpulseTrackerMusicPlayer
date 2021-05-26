#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

/*
struct Channel {

    inline void render(char* buffer, unsigned long framesInBuffer, size_t rate) {

        while (framesInBuffer) {
            size_t samplesToRender = std::max(1UL, (sample->size() - index) / rate);
            if (framesInBuffer < samplesToRender) {
                samplesToRender = framesInBuffer;
            }
            framesInBuffer -= samplesToRender;
            while (samplesToRender--) {
                *buffer++ = (*sample)[index];
                index += rate;
            }

            if (index >= sample->size()) {
                index -= sample->size();
            }
        }
    }

    std::vector<char>* sample;
    size_t index = 0;
};
*/

class Sample {
    struct Loop {
        size_t begin;
        size_t end;
    };

public:
    template<typename Iterator>
    Sample(Iterator b, Iterator e, size_t loopBegin_ = 0, size_t loopEnd_ = 0)
            : _data(b, e), _loop{loopBegin_, loopEnd_ ? loopEnd_ : _data.size()} {}
    Sample(std::initializer_list<float> il) : _data(il), _loop{0, _data.size()}  {}

    inline float operator[](float i) const {
        auto wholeI = static_cast<size_t>(i);
        float t = i - wholeI;
        size_t nextIndex = wholeI + 1;
        if (nextIndex == loopEnd()) {
            nextIndex -= loopLength();
        }
        float v0 = _data[wholeI];
        float v1 = _data[nextIndex];
        return v0 + t * (v1 - v0); 
    }
    inline float operator[](size_t i) const {
        return _data[i];
    }
    inline size_t length() const {
        return _data.size();
    }
    inline size_t loopBegin() const {
        return _loop.begin;
    }
    inline size_t loopEnd() const {
        return _loop.end;
    }
    inline size_t loopLength() const {
        return loopEnd() - loopBegin();
    }

private:
    std::vector<float> _data;
    Loop _loop;
};

class Channel {

public:
    void set_sample(const Sample* sample_) {
        _sample = sample_;
        _sampleIndex = 0;
    }

    void set_frequency(const float freq) {
        _frequency = freq;
    }

    void render(float* outputBuffer, unsigned long framesPerBuffer, const unsigned int targetSampleRate) {

        float rate = _frequency / targetSampleRate;
        if (_sample == nullptr) {
            memset(outputBuffer, 0, framesPerBuffer * sizeof outputBuffer[0]);
            return;
        }
        while (framesPerBuffer--) {
            if (_sampleIndex >= _sample->loopEnd()) {
                _sampleIndex -= _sample->loopLength();
            }
            *outputBuffer++ = (*_sample)[_sampleIndex];
            _sampleIndex += rate;
        }
    }

private:
    const Sample* _sample = nullptr;
    float _sampleIndex = 0;
    float _frequency = 44100;
};

TEST(Channel, NullSampleResultsInSilence) {
    std::vector<float> buffer{1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> expected(buffer.size(), 0);

    Channel c;
    c.render(&buffer[0], buffer.size(), 1);

    ASSERT_EQ(buffer, expected);
}

TEST(Channel, CanSpecifyFrequency) {
    std::vector<float> expected{
        0.0f, 0.25f, 0.5f, 0.75f, 1.0f, // Render 1 - Frequency x1
        0.0, 0.5f, 1.0f,                // Render 2 - Frequency x2
        0.25, 0.375, 0.5, 0.625f        // Render 3 - Frequency x0.5 (with lerping)
        };
    std::vector<float> buffer(expected.size(), 0);
    Sample sample(expected.begin(), expected.begin() + 5);

    Channel c;
    c.set_sample(&sample);

    c.set_frequency(1.0);
    c.render(&buffer[0], 5, 1);
    // Double the frequency will skip values
    c.set_frequency(2.0);
    c.render(&buffer[5], 3, 1);
    // Half original frequency
    c.set_frequency(0.5);
    c.render(&buffer[8], 4, 1);

    EXPECT_EQ(buffer, expected);
}

TEST(Channel, CanSpecifySampleRateOnRender) {
    std::vector<float> expected {
        0.0f, 0.25f, 0.5f, 0.75f, 1.0f
    };
    std::vector<float> buffer(expected.size(), 0);
    Sample sample{0, 1.0f};

    Channel c;
    c.set_sample(&sample);

    // Our sample is only two samples long
    c.set_frequency(1.0);
    // Rendering with a sample rate of 5 will expand it to fit 5 samples
    c.render(&buffer[0], buffer.size(), 4); 

    EXPECT_EQ(buffer, expected);
}

TEST(Sample, CanAcceptInitializerList) {
    Sample sample{0, 1.0f, 0, 1.0f};

    ASSERT_EQ(sample.length(), 4UL);
    EXPECT_EQ(sample[0UL], 0);
    EXPECT_EQ(sample[1UL], 1.0f);
    EXPECT_EQ(sample[2UL], 0);
    EXPECT_EQ(sample[3UL], 1.0f);
}

TEST(Sample, CanAcceptFloatIndiciesAndLerp) {
    Sample sample{0, 1.0f, 0.80f};

    EXPECT_EQ(sample[0.5f], 0.5f);
    EXPECT_EQ(sample[0.25f], 0.25f);
    EXPECT_EQ(sample[0.75f], 0.75f);
    EXPECT_EQ(sample[1.5f], 0.90f);
}

TEST(Sample, CanLerpPastLastIndex) {
    Sample sample{0, 1.0f};
    EXPECT_EQ(sample[1.5f], 0.5f);
}

TEST(Sample, HasDefaultLoopParams) {
    Sample sample{0, 1.0f};
    EXPECT_EQ(sample.loopBegin(), 0UL);
    EXPECT_EQ(sample.loopEnd(), 2UL);
}

TEST(Sample, CanSpecifyLoopParamsInConstructor) {
    std::vector<float> sample_data{0, 0.25f, 0.5f, 0.75f, 1.0f};
    Sample sample(sample_data.begin(), sample_data.end(), 1, 4);

    ASSERT_EQ(sample.length(), 5UL);
    EXPECT_EQ(sample[0UL], 0);
    EXPECT_EQ(sample[1UL], 0.25f);
    EXPECT_EQ(sample[2UL], 0.5f);
    EXPECT_EQ(sample[3UL], 0.75f);
    EXPECT_EQ(sample[4UL], 1.0f);

    EXPECT_EQ(sample.loopBegin(), 1UL);
    EXPECT_EQ(sample.loopEnd(), 4UL);
}

TEST(Sample, CanLerpToLoopBegin) {
    std::vector<float> sample_data{0, 0.25f, 0.5f, 0.75f, 1.0f};
    Sample sample(sample_data.begin(), sample_data.end(), 2);

    EXPECT_EQ(sample[4.5f], 0.75f);
}

TEST(Sample, CanLerpPastLoopEnd) {
    std::vector<float> sample_data{0, 0.25f, 0.5f, 0.75f, 1.0f};
    Sample sample(sample_data.begin(), sample_data.end(), 1, 4);

    EXPECT_EQ(sample[3.5f], (0.75f - 0.25f) / 2 + 0.25f);
}

/*
TEST(Sample, CanSpecifyLoopEnd) {
    std::vector<float> sample_data{0, 0.25f, 0.5f, 0.75f, 1.0f};
    Sample sample(sample_data.begin(), sample_data.end(), 1, 3);
    */

/*
TEST(ChannelRender, CanHandleSmallSampleLoops) {
    std::vector<char> sample{'A', 'B'};
    std::vector<char> expected{'A', 'B', 'A', 'B', 'A'};

    std::vector<char> buffer(expected.size());

    Channel channel;
    channel.sample = &sample;
    channel.render(&buffer[0], 5, 1);

    ASSERT_EQ(buffer, expected);
}

TEST(ChannelRender, CanHandleLoops) {
    std::vector<char> sample{'A', 'B', 'C', 'D', 'E', 'F', 'G'};
    std::vector<char> expected{'A', 'C', 'E', 'G', 'B', 'D'};

    std::vector<char> buffer(expected.size());

    Channel channel;
    channel.sample = &sample;
    channel.render(&buffer[0], 2, 2);
    channel.render(&buffer[2], 2, 2);
    channel.render(&buffer[4], 2, 2);

    ASSERT_EQ(buffer, expected);
}
*/

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv); 
    return RUN_ALL_TESTS();
}