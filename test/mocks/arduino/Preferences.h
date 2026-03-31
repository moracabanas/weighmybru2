#pragma once

#include <cstdint>

class Preferences {
public:
    void begin(const char* namespace_, bool readOnly = false) {}
    void end() {}
    bool isKey(const char* key) { return false; }
    float getFloat(const char* key, float defaultValue = 0.0f) { return defaultValue; }
    unsigned long getULong(const char* key, unsigned long defaultValue = 0) { return defaultValue; }
    int getInt(const char* key, int defaultValue = 0) { return defaultValue; }
    void putFloat(const char* key, float value) {}
    void putULong(const char* key, unsigned long value) {}
    void putInt(const char* key, int value) {}
    void remove(const char* key) {}
    void clear() {}
};