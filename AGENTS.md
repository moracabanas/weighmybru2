# WeighMyBru2 - Agent Development Guide

## Project Overview

- **Type**: ESP32-S3 embedded firmware (C++/Arduino framework)
- **Build System**: PlatformIO
- **Hardware**: HX711 load cell, SSD1306 OLED, BLE, WiFi
- **Web UI**: Alpine.js + Chart.js on LittleFS
- **Repository**: github.com/031devstudios/weighmybru2

## IMPORTANT: ASK THE USER EVERY TIME FOR CONFIRMATION BEFORE COMMIT

**NEVER** run `git commit` without explicit user confirmation. Always present the staged changes and proposed commit message, wait for user approval, then commit only after they explicitly say to proceed.
## Build Commands

```bash
# Build firmware (select environment)
pio run -e esp32s3-supermini          # Build for Supermini
pio run -e esp32s3-xiao               # Build for XIAO

# Flash firmware to hardware
pio run -e esp32s3-supermini -t upload
pio run -e esp32s3-xiao -t upload

# Upload filesystem (required after firmware flash)
pio run -e esp32s3-supermini -t uploadfs
pio run -e esp32s3-xiao -t uploadfs

# Serial monitor
pio run -e esp32s3-supermini -t monitor

# Clean build
pio run -t clean

# Run unit tests (native)
pio test -e native
```

## Code Conventions

### Style
- K&R brace style, 4-space indent
- PascalCase for classes (e.g., `Scale`, `Display`, `WebServer`)
- camelCase for methods and variables (e.g., `getWeight()`, `currentWeight`)
- SCREAMING_SNAKE_CASE for constants (e.g., `MAX_SAMPLES`, `SCREEN_WIDTH`)

### File Structure
```
src/                    # C++ source files
  main.cpp             # Entry point, hardware initialization
  Scale.cpp            # HX711 load cell interface
  WebServer.cpp        # Async web server with REST API
  Display.cpp          # OLED display, timer management
  FlowRate.cpp         # Real-time flow rate calculation
  BluetoothScale.cpp   # BLE GATT server
  ...
include/               # Header files
  *.h                  # Class declarations
data/                   # LittleFS filesystem (web UI)
  index.html           # Main dashboard
  settings.html        # Configuration page
  calibration.html     # Calibration page
  js/                  # JavaScript (Alpine.js)
  css/                 # Stylesheets
test/                   # PlatformIO unit tests
```

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        main.cpp                             │
│  setup(): Hardware init sequence                            │
│  loop(): 100Hz task scheduler                               │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│    Scale      │   │   Display     │   │ BluetoothScale│
│  (HX711)     │   │   (OLED)     │   │   (BLE)      │
└───────────────┘   └───────────────┘   └───────────────┘
        │                     │
        ▼                     ▼
┌───────────────┐   ┌───────────────┐
│   FlowRate    │   │  WebServer   │
│  (calculations)│   │  (REST API)  │
└───────────────┘   └───────────────┘
```

### Timer Management

The `Display` class owns the timer system:
- `Display::startTimer()` - Start timer, begin flow averaging
- `Display::stopTimer()` - Stop timer, stop flow averaging
- `Display::resetTimer()` - Reset timer to zero AND sets timerState to IDLE
- `Display::isTimerRunning()` - Check if timer active
- `Display::getElapsedTime()` - Get elapsed milliseconds
- `Display::getTimerState()` - Returns TimerState enum (IDLE, RUNNING, STOPPED)

**Timer States**: IDLE (waiting), RUNNING (timing), STOPPED (paused after brew)

### Scale Brewing States

`Scale` class tracks brewing detection:
- `STABLE` - No weight change, using average filter
- `BREWING` - Active weight change detected, using median filter
- `TRANSITIONING` - Between brewing and stable

### REST API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/dashboard` | Main polling endpoint (all data) |
| POST | `/api/timer/start` | Start timer |
| POST | `/api/timer/stop` | Stop timer |
| POST | `/api/timer/reset` | Reset timer |
| POST | `/api/tare` | Tare scale; if STOPPED resets timer, if RUNNING resets auto-brew only |
| GET | `/api/weight-fast` | Fast weight-only response |
| GET | `/api/brew/weight` | GaggiMate-compatible weight |
| GET | `/api/brew/status` | GaggiMate-compatible status |

### Preferences (NVS)

Settings stored via `Preferences` class:
- `scale` namespace - calibration, filter settings
- `display` namespace - decimal places
- `wifi` namespace - WiFi credentials

## Testing Strategy (GoogleTest + GMock)

### Framework
- **Framework**: GoogleTest + GMock (native platform)
- **Test Location**: `test/unit/` for unit tests, `test/integration/` for integration
- **Mock Location**: `test/mocks/` for mock implementations
- **Execution**: `pio test -e native`

### Test Structure
```
test/
├── mocks/                   # Mock implementations (header-only)
│   └── MockHX711.h          # Mock HX711 interface
├── unit/                    # Unit tests (GoogleTest)
│   ├── test_flowrate.cpp    # FlowRate pure calculation tests
│   ├── test_scale.cpp       # Scale with mocked HX711
│   ├── test_battery.cpp      # BatteryMonitor tests
│   └── test_display_timer.cpp # Display timer state tests
└── integration/             # Future integration tests
```

### Mocking Strategy
- Use **interface-based mocking** with header-only mocks
- **Dependency injection** via constructor parameters
- **No heap allocation** in tests - use stack-allocated mocks
- Mock hardware interfaces (HX711, etc.) to test business logic

### Running Tests
```bash
# Run all unit tests
pio test -e native

# Run with verbose output
pio test -e native -v

# Run specific test file
pio test -e native -f test_flowrate
```

### Test Requirements (MANDATORY)
- [ ] All new classes MUST have unit tests before PR
- [ ] All bug fixes MUST have regression tests
- [ ] Tests MUST pass on native platform before merge
- [ ] Test coverage should be >80% for new code

### CI/CD Enforcement
- Unit tests run on EVERY push/PR (`.github/workflows/build-dev.yml`)
- **Tests are required for PR merge** (branch protection on `main`)
- Firmware builds only run AFTER unit tests pass
- `test-build` job has `needs: unit-tests` dependency

### Testable Architecture
Classes ordered by testability:
1. **FlowRate** ⭐⭐⭐ HIGH - Pure calculations, no hardware deps
2. **Scale** ⭐⭐ MODERATE - HX711 dependency, needs mock interface
3. **BatteryMonitor** ⭐⭐ MODERATE - Voltage formula testable
4. **Display** ⭐ LOW - Timer logic testable, OLED deps need mock

### Writing Testable Code
1. **Cut at the interface** - hardware access via abstract interface
2. **Constructor injection** - pass dependencies via constructor
3. **No bare-metal calls** - `digitalWrite()` → `hal_->digitalWrite()`
4. **Keep interfaces narrow** - mock only what you test
5. **Use `lib_ignore`** - exclude hardware files from native builds

### Hardware Testing
- Manual flash + test on physical hardware
- Test on BOTH board variants:
  - ESP32-S3 Supermini
  - XIAO ESP32S3

### Editor Integration
- **VSCode**: PlatformIO extension with native test support (`.vscode/settings.json`)
- **Zed**: Use tasks via `.zed/config.toml` (limited PlatformIO support)

## Feature Development Workflow

### Branch Naming
```
main              # Stable releases
AutoBrewTimer    # Feature: Auto-brew timer checkbox
```

### Development Process
1. Create feature branch from `main`
2. Implement changes following conventions
3. Build verification: `pio run -e esp32s3-supermini`
4. Test on hardware
5. Submit PR with code review checklist

## Code Review Checklist

- [ ] Code compiles without warnings
- [ ] Follows K&R brace style, 4-space indent
- [ ] PascalCase classes, camelCase methods
- [ ] No heap allocations in loops
- [ ] HX711 connection checked before operations
- [ ] API endpoints follow REST conventions
- [ ] Preferences keys documented with defaults
- [ ] Tested on ESP32-S3 Supermini AND XIAO
- [ ] Web UI follows Alpine.js patterns
- [ ] AGENTS.md updated if needed
- [ ] No secrets/keys committed

## Important Notes

### PlatformIO Installation
```bash
# Linux/macOS
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-init/master/install.sh | bash

# Verify
pio --version
```

### Filesystem Upload
After flashing firmware, MUST upload filesystem:
```bash
pio run -e esp32s3-supermini -t uploadfs
```

### Build Scripts
```bash
./build.sh              # Build both board variants
./build-and-release.sh  # Build + prepare website releases
```

## Future Features (See AGENTS.md in docs/)

- AutoBrewTimer: Checkbox to auto-start/stop timer based on weight detection

## TODO: Integration Tests

**Pending (Future PR)**: Add integration tests for:
- WebServer REST API endpoints with mocked AsyncWebServer
- BluetoothScale BLE protocol with mocked NimBLE
- WiFiManager with mocked WiFi
- Full hardware-in-the-loop tests on ESP32-S3
