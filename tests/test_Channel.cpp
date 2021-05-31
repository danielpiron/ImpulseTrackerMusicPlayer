#include <gtest/gtest.h>

#include <Channel.h>

#include <vector>

TEST(Channel, NullSampleResultsInSilence) {
    std::vector<float> buffer{1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> expected(buffer.size(), 0);

    Channel c;
    c.render(&buffer[0], buffer.size(), 1);

    ASSERT_EQ(buffer, expected);
}

TEST(Channel, CanHandleSingleSampleSample) {
    std::vector<float> expected{1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<float> buffer(expected.size());

    Sample sample({1.0f});
    Channel c;
    c.set_sample(&sample);
    c.set_frequency(1.0);
    c.render(&buffer[0], 4, 1);

    EXPECT_EQ(buffer, expected);
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

TEST(Channel, CanSetVolume) {
    std::vector<float> expected {
        0, 0.5f, 1.0f, 0, 0.25f, 0.5f, 0, .125, .25f
    };
    std::vector<float> buffer(expected.size(), 0);

    Sample sample{0, 0.5f, 1.0f, 0, 0.5f, 1.0f, 0, 0.5f, 1.0f};

    Channel c;
    c.set_sample(&sample);
    c.set_frequency(1.0);

    c.set_volume(1.0);
    c.render(&buffer[0], 3, 1); 
    c.set_volume(0.5);
    c.render(&buffer[3], 3, 1); 
    c.set_volume(0.25);
    c.render(&buffer[6], 3, 1); 

    EXPECT_EQ(buffer, expected);
}

TEST(Channel, CanRenderNonLoopingSample) {
    // The sample plays then the channel goes silent
    std::vector<float> expected{-1.0f, 1.0f, 0, 0};
    std::vector<float> buffer(expected.size(), 0);

    // Given that the sample is only two frames long
    Sample sample({-1.0f, 1.0f}, {Sample::LoopParams::Type::non_looping});
    Channel c;

    c.set_sample(&sample);
    // And we are rendering 4 frames
    c.render(&buffer[0], 4, 1); 

    // We expect two frames with the sample data, and two at zero
    EXPECT_EQ(buffer, expected);
}