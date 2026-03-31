#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>
#include <stdio.h>
#include <string>

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

typedef std::string String;

unsigned long millis();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);

class __attribute__((visibility("default"))) SerialClass {
public:
    void begin(unsigned long baud) {}
    void printf(const char* fmt, ...) {}
    template<typename... Args>
    void printf(const char* fmt, Args... args) {}
    template<typename... Args>
    void print(const char* fmt, Args... args) {}
    template<typename... Args>
    void println(const char* fmt, Args... args) {}
    template<typename T>
    void println(T value) {}
    template<typename T>
    void print(T value) {}
    int available() { return 0; }
    int read() { return -1; }
};

extern SerialClass Serial;

#define String std::string

template<typename T>
struct __attribute__((visibility("default"))) IPrint {
    virtual void print(T) = 0;
    virtual void println(T) = 0;
};

template<>
struct __attribute__((visibility("default"))) IPrint<const char*> {
    virtual void print(const char*) = 0;
    virtual void println(const char*) = 0;
};