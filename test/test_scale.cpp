#include <gtest/gtest.h>
#include <string>
#include <cmath>

using String = std::string;

#define FLOWRATE_AVG_WINDOW 20
#define FLOWRATE_SAMPLE_WINDOW 10

class TestFlowRate {
public:
    TestFlowRate() : lastWeight(0), lastTime(0), flowRate(0), bufferIndex(0), bufferCount(0),
        timerAveragingActive(false), timerFlowRateSum(0), timerFlowRateSamples(0), 
        timerAverageFlowRate(0), hasValidTimerAverage(false), calculationPaused(false) {
        for (int i = 0; i < FLOWRATE_AVG_WINDOW; ++i) flowRateBuffer[i] = 0;
    }
    
    void update(float currentWeight, unsigned long currentTime) {
        if (calculationPaused) return;
        if (lastTime > 0) {
            float deltaWeight = currentWeight - lastWeight;
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            if (deltaTime >= 0.15f) {
                if (fabs(deltaWeight) < 0.08f) deltaWeight = 0.0f;
                if (deltaTime > 0) {
                    float instantRate = deltaWeight / deltaTime;
                    flowRateBuffer[bufferIndex] = instantRate;
                    bufferIndex = (bufferIndex + 1) % FLOWRATE_AVG_WINDOW;
                    if (bufferCount < FLOWRATE_AVG_WINDOW) bufferCount++;
                    flowRate = calculateStableAverage(false);
                    if (timerAveragingActive && flowRate > 0.1f) {
                        timerFlowRateSum += flowRate;
                        timerFlowRateSamples++;
                    }
                    if (fabs(flowRate) < 0.08f) flowRate = 0.0f;
                }
            }
            lastWeight = currentWeight;
            lastTime = currentTime;
        } else {
            lastWeight = currentWeight;
            lastTime = currentTime;
        }
    }
    
    float calculateStableAverage(bool isWeightRemoval) {
        if (bufferCount == 0) return 0.0f;
        if (isWeightRemoval) {
            int samplesToUse = std::min(5, bufferCount);
            float sum = 0.0f;
            for (int i = 0; i < samplesToUse; i++) {
                int index = (bufferIndex - 1 - i + FLOWRATE_AVG_WINDOW) % FLOWRATE_AVG_WINDOW;
                sum += flowRateBuffer[index];
            }
            return sum / samplesToUse;
        } else {
            float weightedSum = 0.0f;
            float totalWeight = 0.0f;
            int samplesToUse = std::min(bufferCount, FLOWRATE_AVG_WINDOW);
            for (int i = 0; i < samplesToUse; i++) {
                int index = (bufferIndex - 1 - i + FLOWRATE_AVG_WINDOW) % FLOWRATE_AVG_WINDOW;
                float weight = 1.0f + (0.05f * (samplesToUse - i));
                weightedSum += flowRateBuffer[index] * weight;
                totalWeight += weight;
            }
            return weightedSum / totalWeight;
        }
    }
    
    float getFlowRate() const { return flowRate; }
    void startTimerAveraging() { timerAveragingActive = true; timerFlowRateSum = 0; timerFlowRateSamples = 0; hasValidTimerAverage = false; }
    void stopTimerAveraging() { 
        if (timerAveragingActive && timerFlowRateSamples > 0) {
            timerAverageFlowRate = timerFlowRateSum / timerFlowRateSamples;
            hasValidTimerAverage = true;
        } else { timerAverageFlowRate = 0; hasValidTimerAverage = false; }
        timerAveragingActive = false;
    }
    void resetTimerAveraging() { timerAveragingActive = false; timerFlowRateSum = 0; timerFlowRateSamples = 0; timerAverageFlowRate = 0; hasValidTimerAverage = false; }
    bool hasTimerAverage() const { return hasValidTimerAverage; }
    void pauseCalculation() { calculationPaused = true; }
    void resumeCalculation() { calculationPaused = false; lastTime = 0; }
    void clearFlowRateBuffer() { for (int i = 0; i < FLOWRATE_AVG_WINDOW; i++) flowRateBuffer[i] = 0; bufferIndex = 0; bufferCount = 0; flowRate = 0; lastWeight = 0; lastTime = 0; }
    void addFlowSample(float flowRateValue) { flowSamples[flowSampleIndex] = flowRateValue; flowSampleTimestamps[flowSampleIndex] = 0; flowSampleIndex = (flowSampleIndex + 1) % FLOWRATE_SAMPLE_WINDOW; if (flowSampleCount < FLOWRATE_SAMPLE_WINDOW) flowSampleCount++; }
    void clearFlowSamples() { for (int i = 0; i < FLOWRATE_SAMPLE_WINDOW; i++) { flowSamples[i] = 0; flowSampleTimestamps[i] = 0; } flowSampleIndex = 0; flowSampleCount = 0; }
    int getFlowSampleCount() const { return flowSampleCount; }
    void getFlowSamples(float samples[], int maxSamples) const { int count = std::min(flowSampleCount, maxSamples); for (int i = 0; i < count; i++) { int index = (flowSampleIndex - count + i + FLOWRATE_SAMPLE_WINDOW) % FLOWRATE_SAMPLE_WINDOW; samples[i] = flowSamples[index]; } }
    
private:
    float lastWeight;
    unsigned long lastTime;
    float flowRate;
    float flowRateBuffer[FLOWRATE_AVG_WINDOW];
    int bufferIndex;
    int bufferCount;
    bool timerAveragingActive;
    float timerFlowRateSum;
    int timerFlowRateSamples;
    float timerAverageFlowRate;
    bool hasValidTimerAverage;
    bool calculationPaused;
    float flowSamples[FLOWRATE_SAMPLE_WINDOW];
    unsigned long flowSampleTimestamps[FLOWRATE_SAMPLE_WINDOW];
    int flowSampleIndex;
    int flowSampleCount;
};

#define BATTERY_FULL 4.2f
#define BATTERY_GOOD 4.0f
#define BATTERY_NOMINAL 3.8f
#define BATTERY_LOW 3.6f
#define BATTERY_CRITICAL 3.4f
#define BATTERY_EMPTY 3.2f

class TestBatteryMonitor {
public:
    TestBatteryMonitor() : lastVoltage(0.0f), calibrationOffset(0.0f) {}
    float getBatteryVoltage() const { return lastVoltage; }
    int getBatteryPercentage() const {
        float voltage = getBatteryVoltage();
        int percentage;
        if (voltage >= BATTERY_FULL) percentage = 100;
        else if (voltage >= BATTERY_GOOD) percentage = 75 + (int)((voltage - BATTERY_GOOD) / (BATTERY_FULL - BATTERY_GOOD) * 25);
        else if (voltage >= BATTERY_NOMINAL) percentage = 50 + (int)((voltage - BATTERY_NOMINAL) / (BATTERY_GOOD - BATTERY_NOMINAL) * 25);
        else if (voltage >= BATTERY_LOW) percentage = 25 + (int)((voltage - BATTERY_LOW) / (BATTERY_NOMINAL - BATTERY_LOW) * 25);
        else if (voltage >= BATTERY_CRITICAL) percentage = 5 + (int)((voltage - BATTERY_CRITICAL) / (BATTERY_LOW - BATTERY_CRITICAL) * 20);
        else if (voltage >= BATTERY_EMPTY) percentage = (int)((voltage - BATTERY_EMPTY) / (BATTERY_CRITICAL - BATTERY_EMPTY) * 5);
        else percentage = 0;
        if (percentage < 0) percentage = 0;
        if (percentage > 100) percentage = 100;
        return percentage;
    }
    String getBatteryStatus() const {
        float voltage = getBatteryVoltage();
        if (voltage >= BATTERY_FULL) return "Full";
        if (voltage >= BATTERY_GOOD) return "Good";
        if (voltage >= BATTERY_NOMINAL) return "Fair";
        if (voltage >= BATTERY_LOW) return "Low";
        if (voltage >= BATTERY_CRITICAL) return "Critical";
        return "Empty";
    }
    bool isLowBattery() const { return getBatteryVoltage() < BATTERY_LOW; }
    bool isCriticalBattery() const { return getBatteryVoltage() < BATTERY_CRITICAL; }
    bool isCharging() const { return false; }
    float getCalibrationOffset() const { return calibrationOffset; }
    void calibrateVoltage(float) { calibrationOffset = 0.0f; }
    int getBatterySegments() const { int percentage = getBatteryPercentage(); if (percentage >= 75) return 3; if (percentage >= 50) return 2; if (percentage >= 25) return 1; return 0; }
private:
    float lastVoltage;
    float calibrationOffset;
};

class TestDisplay {
public:
    enum class TimerState { IDLE, RUNNING, STOPPED };
    TestDisplay() : timerRunning(false), timerState(TimerState::IDLE), autoBrewTimerEnabled(false) {}
    void startTimer() { if (timerState == TimerState::IDLE) { timerState = TimerState::RUNNING; timerRunning = true; } }
    void stopTimer() { if (timerState == TimerState::RUNNING) { timerState = TimerState::STOPPED; timerRunning = false; } }
    void resetTimer() { timerState = TimerState::IDLE; timerRunning = false; }
    bool isTimerRunning() const { return timerRunning && timerState == TimerState::RUNNING; }
    TimerState getTimerState() const { return timerState; }
    void setAutoBrewTimerEnabled(bool enabled) { autoBrewTimerEnabled = enabled; }
    bool isAutoBrewTimerEnabled() const { return autoBrewTimerEnabled; }
private:
    bool timerRunning;
    TimerState timerState;
    bool autoBrewTimerEnabled;
};

class FlowRateTest : public ::testing::Test {
protected:
    void SetUp() override { flowRate = new TestFlowRate(); }
    void TearDown() override { delete flowRate; }
    TestFlowRate* flowRate;
};

TEST_F(FlowRateTest, ConstructorInitializesToZero) { EXPECT_FLOAT_EQ(flowRate->getFlowRate(), 0.0f); EXPECT_FALSE(flowRate->hasTimerAverage()); }
TEST_F(FlowRateTest, FirstUpdateDoesNotCalculate) { flowRate->update(100.0f, 0); EXPECT_FLOAT_EQ(flowRate->getFlowRate(), 0.0f); }
TEST_F(FlowRateTest, UpdateCalculatesCorrectFlowRate) { flowRate->update(0.0f, 1000); flowRate->update(10.0f, 2000); EXPECT_GE(flowRate->getFlowRate(), 0.0f); }
TEST_F(FlowRateTest, DeadbandIgnoresSmallChanges) { flowRate->update(0.0f, 1000); flowRate->update(0.05f, 1010); EXPECT_FLOAT_EQ(flowRate->getFlowRate(), 0.0f); }
TEST_F(FlowRateTest, CircularBufferWrapsCorrectly) { for (int i = 0; i < 25; i++) flowRate->update(static_cast<float>(i * 10), 1000 + i * 100); EXPECT_GE(flowRate->getFlowRate(), 0.0f); }
TEST_F(FlowRateTest, TimerAveragingStartStopReset) { flowRate->startTimerAveraging(); flowRate->update(0.0f, 1000); flowRate->update(5.0f, 2000); flowRate->stopTimerAveraging(); EXPECT_TRUE(flowRate->hasTimerAverage()); }
TEST_F(FlowRateTest, ResetTimerAveraging) { flowRate->startTimerAveraging(); flowRate->update(0.0f, 1000); flowRate->resetTimerAveraging(); EXPECT_FALSE(flowRate->hasTimerAverage()); }
TEST_F(FlowRateTest, PauseCalculation) { flowRate->update(0.0f, 1000); flowRate->update(10.0f, 2000); float rateBeforePause = flowRate->getFlowRate(); flowRate->pauseCalculation(); flowRate->update(100.0f, 3000); flowRate->update(200.0f, 4000); flowRate->resumeCalculation(); EXPECT_FLOAT_EQ(flowRate->getFlowRate(), rateBeforePause); }
TEST_F(FlowRateTest, ClearFlowRateBuffer) { flowRate->update(0.0f, 1000); flowRate->update(10.0f, 2000); flowRate->clearFlowRateBuffer(); EXPECT_FLOAT_EQ(flowRate->getFlowRate(), 0.0f); }
TEST_F(FlowRateTest, FlowSamplesCircularBuffer) { for (int i = 0; i < 15; i++) flowRate->addFlowSample(static_cast<float>(i)); EXPECT_EQ(flowRate->getFlowSampleCount(), 10); }
TEST_F(FlowRateTest, ClearFlowSamples) { flowRate->addFlowSample(5.0f); flowRate->addFlowSample(10.0f); flowRate->clearFlowSamples(); EXPECT_EQ(flowRate->getFlowSampleCount(), 0); }
TEST_F(FlowRateTest, GetFlowSamples) { flowRate->addFlowSample(1.0f); flowRate->addFlowSample(2.0f); flowRate->addFlowSample(3.0f); float samples[5]; flowRate->getFlowSamples(samples, 5); EXPECT_EQ(flowRate->getFlowSampleCount(), 3); }

class BatteryMonitorTest : public ::testing::Test {
protected:
    void SetUp() override { batteryMonitor = new TestBatteryMonitor(); }
    void TearDown() override { delete batteryMonitor; }
    TestBatteryMonitor* batteryMonitor;
};

TEST_F(BatteryMonitorTest, ConstructorInitializes) { EXPECT_NE(batteryMonitor, nullptr); }
TEST_F(BatteryMonitorTest, GetBatteryPercentageReturnsValidRange) { int percentage = batteryMonitor->getBatteryPercentage(); EXPECT_GE(percentage, 0); EXPECT_LE(percentage, 100); }
TEST_F(BatteryMonitorTest, GetBatteryStatusReturnsValidString) { String status = batteryMonitor->getBatteryStatus(); EXPECT_TRUE(status == "Full" || status == "Good" || status == "Fair" || status == "Low" || status == "Critical" || status == "Empty"); }
TEST_F(BatteryMonitorTest, IsLowBatteryInitiallyTrue) { EXPECT_TRUE(batteryMonitor->isLowBattery()); }
TEST_F(BatteryMonitorTest, IsCriticalBatteryInitiallyTrue) { EXPECT_TRUE(batteryMonitor->isCriticalBattery()); }
TEST_F(BatteryMonitorTest, CalibrateVoltageSetsOffset) { batteryMonitor->calibrateVoltage(4.1f); EXPECT_FLOAT_EQ(batteryMonitor->getCalibrationOffset(), 0.0f); }
TEST_F(BatteryMonitorTest, GetBatterySegmentsReturnsValidRange) { int segments = batteryMonitor->getBatterySegments(); EXPECT_GE(segments, 0); EXPECT_LE(segments, 3); }
TEST_F(BatteryMonitorTest, IsChargingReturnsFalse) { EXPECT_FALSE(batteryMonitor->isCharging()); }
TEST_F(BatteryMonitorTest, GetBatteryVoltageInitiallyZero) { EXPECT_FLOAT_EQ(batteryMonitor->getBatteryVoltage(), 0.0f); }

class DisplayTimerTest : public ::testing::Test {
protected:
    void SetUp() override { display = new TestDisplay(); }
    void TearDown() override { delete display; }
    TestDisplay* display;
};

TEST_F(DisplayTimerTest, InitialStateIsIdle) { EXPECT_EQ(display->getTimerState(), TestDisplay::TimerState::IDLE); }
TEST_F(DisplayTimerTest, StartTimerTransitionsToRunning) { display->startTimer(); EXPECT_EQ(display->getTimerState(), TestDisplay::TimerState::RUNNING); }
TEST_F(DisplayTimerTest, StartTimerWhenRunningIsNoOp) { display->startTimer(); display->startTimer(); EXPECT_EQ(display->getTimerState(), TestDisplay::TimerState::RUNNING); }
TEST_F(DisplayTimerTest, StopTimerTransitionsToStopped) { display->startTimer(); display->stopTimer(); EXPECT_EQ(display->getTimerState(), TestDisplay::TimerState::STOPPED); }
TEST_F(DisplayTimerTest, StopTimerWhenIdleIsNoOp) { display->stopTimer(); EXPECT_EQ(display->getTimerState(), TestDisplay::TimerState::IDLE); }
TEST_F(DisplayTimerTest, ResetTimerTransitionsToIdle) { display->startTimer(); display->stopTimer(); display->resetTimer(); EXPECT_EQ(display->getTimerState(), TestDisplay::TimerState::IDLE); }
TEST_F(DisplayTimerTest, ResetTimerFromIdleIsIdle) { display->resetTimer(); EXPECT_EQ(display->getTimerState(), TestDisplay::TimerState::IDLE); }
TEST_F(DisplayTimerTest, IsTimerRunningReturnsFalseInIdle) { EXPECT_FALSE(display->isTimerRunning()); }
TEST_F(DisplayTimerTest, IsTimerRunningReturnsTrueWhenRunning) { display->startTimer(); EXPECT_TRUE(display->isTimerRunning()); }
TEST_F(DisplayTimerTest, AutoBrewTimerEnabledCanBeSet) { display->setAutoBrewTimerEnabled(true); EXPECT_TRUE(display->isAutoBrewTimerEnabled()); }
TEST_F(DisplayTimerTest, AutoBrewTimerDisabledCanBeSet) { display->setAutoBrewTimerEnabled(false); EXPECT_FALSE(display->isAutoBrewTimerEnabled()); }

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}