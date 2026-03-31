#pragma once

#include <cstdint>

// Abstract interface for HX711 load cell
class IHX711 {
public:
    virtual ~IHX711() = default;
    virtual bool is_ready() = 0;
    virtual long read() = 0;
    virtual long read_average(uint8_t times = 1) = 0;
    virtual float get_units(uint8_t times = 1) = 0;
    virtual void set_scale(float scale = -705.7f) = 0;
    virtual void tare(uint8_t times = 10) = 0;
    virtual void power_down() = 0;
    virtual void power_up() = 0;
};

// Mock implementation for testing
class MockHX711 : public IHX711 {
public:
    MockHX711() : ready(true), mockValue(0), mockScale(-705.7f), tareOffset(0) {}

    bool is_ready() override {
        return ready;
    }

    long read() override {
        return mockValue + tareOffset;
    }

    long read_average(uint8_t times = 1) override {
        return mockValue + tareOffset;
    }

    float get_units(uint8_t times = 1) override {
        return (mockValue + tareOffset) / mockScale;
    }

    void set_scale(float scale = -705.7f) override {
        mockScale = scale;
    }

    void tare(uint8_t times = 10) override {
        tareOffset = -mockValue;
    }

    void power_down() override {
        poweredDown = true;
    }

    void power_up() override {
        poweredDown = false;
    }

    // Test control methods
    void set_ready(bool value) { ready = value; }
    void set_read_value(long value) { mockValue = value; }
    void set_scale_factor(float scale) { mockScale = scale; }
    bool is_powered_down() const { return poweredDown; }

private:
    bool ready;
    bool poweredDown = false;
    long mockValue;
    float mockScale;
    long tareOffset;
};