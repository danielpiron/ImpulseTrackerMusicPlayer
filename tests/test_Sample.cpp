#include <gtest/gtest.h>

#include <player/Sample.h>

#include <vector>

TEST(Sample, CanAcceptInitializerList)
{
    Sample sample({0, 1.0f, 0, 1.0f}, 1);

    ASSERT_EQ(sample.length(), 4UL);
    EXPECT_EQ(sample[0UL], 0);
    EXPECT_EQ(sample[1UL], 1.0f);
    EXPECT_EQ(sample[2UL], 0);
    EXPECT_EQ(sample[3UL], 1.0f);
}

TEST(Sample, CanAcceptFloatIndiciesAndLerp)
{
    Sample sample({0, 1.0f, 0.80f}, 1);

    EXPECT_EQ(sample[0.5f], 0.5f);
    EXPECT_EQ(sample[0.25f], 0.25f);
    EXPECT_EQ(sample[0.75f], 0.75f);
    EXPECT_EQ(sample[1.5f], 0.90f);
}

TEST(Sample, CanLerpPastLastIndex)
{
    Sample sample({0, 1.0f}, 1);
    EXPECT_EQ(sample[1.5f], 0.5f);
}

TEST(Sample, HasDefaultLoopParams)
{
    Sample sample({0, 1.0f}, 1);
    EXPECT_EQ(sample.loopBegin(), 0UL);
    EXPECT_EQ(sample.loopEnd(), 2UL);
}

TEST(Sample, CanSpecifyLoopParamsInConstructor)
{
    std::vector<float> sample_data{0, 0.25f, 0.5f, 0.75f, 1.0f};
    Sample sample(sample_data.begin(), sample_data.end(), 1,
                  {Sample::LoopParams::Type::forward_looping, 1, 4});

    ASSERT_EQ(sample.length(), 5UL);
    EXPECT_EQ(sample[0UL], 0);
    EXPECT_EQ(sample[1UL], 0.25f);
    EXPECT_EQ(sample[2UL], 0.5f);
    EXPECT_EQ(sample[3UL], 0.75f);
    EXPECT_EQ(sample[4UL], 1.0f);

    EXPECT_EQ(sample.loopBegin(), 1UL);
    EXPECT_EQ(sample.loopEnd(), 4UL);
}

TEST(Sample, CanLerpToLoopBegin)
{
    std::vector<float> sample_data{0, 0.25f, 0.5f, 0.75f, 1.0f};
    Sample sample(sample_data.begin(), sample_data.end(), 1,
                  {Sample::LoopParams::Type::forward_looping, 2});

    EXPECT_EQ(sample[4.5f], 0.75f);
}

TEST(Sample, CanLerpPastLoopEnd)
{
    std::vector<float> sample_data{0, 0.25f, 0.5f, 0.75f, 1.0f};
    Sample sample(sample_data.begin(), sample_data.end(), 1,
                  {Sample::LoopParams::Type::forward_looping, 1, 4});

    EXPECT_EQ(sample[3.5f], (0.75f - 0.25f) / 2 + 0.25f);
}

TEST(Sample, NonLoopingSamplesLerpToZeroAtEnd)
{
    Sample sample({1.0f}, 1, {Sample::LoopParams::Type::non_looping});
    EXPECT_EQ(sample[0.5f], 0.5f);
}