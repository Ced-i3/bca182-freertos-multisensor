# BCA182 FreeRTOS Multisensor

STM32F103C8 (Blue Pill) multitasking multisensor system using FreeRTOS,
STM32 HAL, and PlatformIO. Reads temperature, humidity, light level, and
motion; drives an SSD1306 OLED display and buzzer alarm; navigates display
pages via a rotary encoder.

## Hardware

| Component | Wokwi Part | Purpose |
|-----------|-----------|---------|
| STM32 Blue Pill | `board-stm32-bluepill` | MCU (STM32F103C8T6, 72 MHz) |
| DHT22 | `wokwi-dht22` | Temperature and humidity |
| Photoresistor | `wokwi-photoresistor-sensor` | Ambient light (LDR) |
| PIR sensor | `wokwi-pir-motion-sensor` | Motion detection |
| Rotary encoder | `wokwi-ky-040` | Display page navigation |
| SSD1306 OLED | `wokwi-ssd1306-i2c` | 128x64 display |
| Buzzer | `wokwi-buzzer` | Temperature alarm |

## Pin Mapping

| Signal | Pin | Peripheral | Direction |
|--------|-----|-----------|-----------|
| LDR analog out | PA0 | ADC1 CH0 | Input |
| DHT22 data | PA1 | GPIO (open-drain) | Bidirectional |
| PIR output | PB0 | GPIO input (pull-down) | Input |
| Buzzer | PB1 | GPIO output (push-pull) | Output |
| OLED SCL | PB6 | I2C1 SCL | Output |
| OLED SDA | PB7 | I2C1 SDA | Bidirectional |
| Encoder CLK | PB10 | GPIO input (pull-up) | Input |
| Encoder DT | PB11 | GPIO input (pull-up) | Input |
| UART1 TX | PA9 | USART1 TX | Output |
| UART1 RX | PA10 | USART1 RX | Input |
| Onboard LED | PC13 | GPIO output (active-low) | Output |

## FreeRTOS Architecture

Eight application tasks using FreeRTOS preemptive multitasking with shared
sensor data distributed through a queue.

### FreeRTOS Kernel Configuration

| Parameter | Value |
|-----------|-------|
| CPU clock | 72 MHz (HSE 8 MHz x PLL9) |
| Tick rate | 1000 Hz (1 ms) |
| Preemption | Enabled |
| Time slicing | Enabled |
| Max priorities | 5 |
| Minimal stack size | 128 words |
| Total heap | 14 KB |
| Stack overflow check | Method 2 |
| Task notifications | Enabled (1 entry) |
| Mutexes | Enabled |
| Timers | Enabled (priority 2, stack 128 words) |

### Tasks

| Task | Priority | Stack | Period | Behavior |
|------|----------|-------|--------|----------|
| **SensorTask** | 2 | 256 words | 2000 ms | Reads DHT22 + LDR, sends `SensorData` to queue via `xQueueSend` (non-blocking). Uses `vTaskDelayUntil` for drift-free timing. |
| **StateTask** | 2 | 256 words | 15 s timeout | Blocks on `ulTaskNotifyTake`. Sets ACTIVE on notification, INACTIVE on timeout. |
| **MotionTask** | 3 | 256 words | 100 ms | Polls PIR pin, calls `SystemState_NotifyMotionDetected()` (`xTaskNotifyGive`). |
| **InputTask** | 3 | 256 words | 1 ms | Polls rotary encoder CLK/DT, debounces (10 ms), rotates `DisplayMode` through four pages. Encoder is gated to ACTIVE state only. |
| **DisplayTask** | 2 | 352 words | 200 ms | Peeks latest `SensorData` from queue (`xQueuePeek`, non-destructive). Renders mode header, sensor value, and ACTIVE/INACTIVE state on SSD1306 OLED. Uses `vTaskDelayUntil`. |
| **AlarmTask** | 2 | 256 words | 500 ms | Peeks `SensorData`, evaluates `EvaluateTemperature()`. Toggles buzzer at ~2 Hz when alarm is active; silences otherwise. |
| **TaskA** | 2 | 256 words | 1000 ms | Toggles onboard LED (PC13), prints diagnostic message via UART. |
| **TaskB** | 1 | 256 words | 1500 ms | Prints diagnostic message via UART. |

## Inter-Task Communication and Synchronization

### Sensor Queue

A FreeRTOS queue of depth 4 carrying `SensorData` structs. SensorTask is the
sole producer (via `xQueueSend`, non-blocking with zero tick count -- samples
are dropped when full). DisplayTask and AlarmTask are consumers; both use
`xQueuePeek` for non-destructive reads so the latest sample is always
available without consuming the item.

```c
struct SensorData {
    float temperature;   // DHT22 reading (°C)
    float humidity;      // DHT22 reading (%)
    int   lightLevel;    // LDR ADC scaled 0-100
    bool  motionDetected;// PIR state snapshot
};
```

### Mutexes

| Mutex | Created In | Protects |
|-------|-----------|----------|
| `serialMutex` | `main.cpp` | `HAL_UART_Transmit` -- prevents multi-task serial output from interleaving. |
| `displayModeMutex` | `input.cpp` | `selectedDisplayMode` -- InputTask writes, DisplayTask reads via `Input_GetDisplayMode()`. |

### Task Notification

MotionTask -> StateTask via `xTaskNotifyGive` / `ulTaskNotifyTake`.
MotionTask calls `SystemState_NotifyMotionDetected()` each time the PIR pin
reads HIGH. StateTask blocks with `ulTaskNotifyTake(pdTRUE, 15000 ms)` -- a
notification sets ACTIVE, a 15-second timeout sets INACTIVE.

### Critical Sections

`taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` protect the shared
`activityState` and `motionDetected` variables in `SystemState_GetActivityState()`,
`SystemState_IsMotionDetected()`, and the `SetState()` helper. These short
critical sections disable interrupts to ensure atomic reads and writes on
the Cortex-M3.

## ACTIVE / INACTIVE System Behavior

- **ACTIVE**: System begins in this state. Encoder navigation is enabled. Alarm
  evaluates temperature against thresholds. Motion is tracked and displayed.
- **INACTIVE**: Entered after 15 seconds without a PIR motion notification.
  Encoder navigation is disabled. Alarm is silenced regardless of temperature.
  Display shows "State: INACTIVE".
- **Transition**: Any PIR motion event immediately returns the system to ACTIVE.

## Temperature Alarm

`EvaluateTemperature()` returns `true` when:

- Temperature < **18 °C** OR temperature > **30 °C**, **AND**
- System state is **ACTIVE**

When active, the buzzer toggles at approximately 2 Hz (500 ms period). When
inactive or temperature is within range, the buzzer is held LOW (silent).

## Rotary Encoder and OLED Display Navigation

The SSD1306 OLED (128x64, I2C at address 0x3C) shows four pages, cycled by
rotating the KY-040 encoder:

| Page | Header | Value Row |
|------|--------|-----------|
| 0 | `-- Temperature --` | `Temp: XX.X C` |
| 1 | `--- Humidity  ---` | `Hum:  XX.X %` |
| 2 | `--- Light     ---` | `Light: XX /100` |
| 3 | `--- Motion    ---` | `Motion: YES/NO` |

Row 5 always shows `State: ACTIVE` or `State: INACTIVE`.

Clockwise rotation calls `NextDisplayMode()`, counter-clockwise calls
`PreviousDisplayMode()`. Navigation wraps around: MOTION -> TEMPERATURE and
TEMPERATURE -> MOTION. The encoder input is gated to the ACTIVE state -- no
page changes occur while INACTIVE.

## Project Structure

```
+-- include/
|   +-- alarm.h
|   +-- display.h
|   +-- FreeRTOSConfig.h
|   +-- input.h
|   +-- motion.h
|   +-- sensors.h
|   +-- system_state.h
+-- src/
|   +-- alarm.cpp          # AlarmTask + EvaluateTemperature()
|   +-- display.cpp         # DisplayTask + SSD1306 driver
|   +-- hal_mock.cpp        # Test-only stubs (guarded by #ifdef UNIT_TEST)
|   +-- input.cpp           # InputTask + NextDisplayMode/PreviousDisplayMode
|   +-- main.cpp            # Entry point, hardware init, task creation
|   +-- motion.cpp          # MotionTask + PIR driver
|   +-- sensors.cpp         # DHT22 bit-bang + LDR ADC read
|   +-- system_state.cpp    # StateTask + EvaluateSystemState()
+-- test/
|   +-- test_temperature/   # 5 tests for EvaluateTemperature()
|   +-- test_system_state/  # 4 tests for EvaluateSystemState()
|   +-- test_display_mode/  # 4 tests for NextDisplayMode/PreviousDisplayMode
+-- lib/
|   +-- FreeRTOS/           # FreeRTOS kernel sources (ARM_CM3 port)
+-- docs/
|   +-- laboratory-report.pdf
+-- diagram.json            # Wokwi circuit diagram
+-- wokwi.toml              # Wokwi firmware paths
+-- platformio.ini          # Build configuration
+-- README.md
```

## Building the STM32 Firmware

Prerequisites: PlatformIO CLI and the `ststm32` platform package.

```bash
pio run
```

The firmware is compiled for the Blue Pill target (`bluepill_f103c8`) using
the STM32Cube framework. The clock is configured for 72 MHz (HSE 8 MHz with
PLL x9).

**Build result:** SUCCESS -- RAM 78.1% (15,988 / 20,480 bytes), Flash 28.1%
(18,428 / 65,536 bytes).

## Running the Native Unit Tests

Prerequisites: PlatformIO CLI with a native toolchain (MinGW on Windows,
GCC on Linux/macOS).

```bash
pio test -e native
```

The `native` environment compiles `alarm.cpp`, `system_state.cpp`,
`input.cpp`, and `hal_mock.cpp` for the host. FreeRTOS and HAL calls are
stubbed by `hal_mock.cpp` (compiled only when `UNIT_TEST` is defined). No
Arduino framework is used.

## Unit-Test Results

13 Unity tests across three suites, exercising production functions linked
from `src/`:

| Suite | Tests | Function Under Test |
|-------|-------|---------------------|
| `test_temperature` | 5 | `EvaluateTemperature(float, ActivityState)` -- below 18 °C, at 18 °C, in range (24 °C), at 30 °C, above 30 °C |
| `test_display_mode` | 4 | `NextDisplayMode(DisplayMode)`, `PreviousDisplayMode(DisplayMode)` -- forward navigation, backward navigation, wraparound in both directions |
| `test_system_state` | 4 | `EvaluateSystemState(bool)` -- notification->ACTIVE, timeout->INACTIVE, repeated notification, consecutive timeout |

**Result: 13/13 PASSED**

```
native         test_display_mode  PASSED    00:00:03.160
native         test_system_state  PASSED    00:00:01.944
native         test_temperature   PASSED    00:00:01.808
================= 13 test cases: 13 succeeded in 00:00:06.912 =================
```

## Static Analysis

```bash
pio check
```

Runs `cppcheck` against the `bluepill_f103c8` build.

**Result: 0 HIGH, 0 MEDIUM, 27 LOW -- PASSED**

All 27 low-severity findings are cppcheck style warnings: unused-function
reports (functions called only from `main.cpp`, which cppcheck does not
trace across translation units) and C-style pointer casts inherent to the
HAL API.

## Wokwi Setup

Open `diagram.json` in the [Wokwi](https://wokwi.com) simulator. The
`wokwi.toml` points to the PlatformIO firmware output:

| Artifact | Path |
|----------|------|
| Firmware binary | `.pio/build/bluepill_f103c8/firmware.bin` |
| ELF | `.pio/build/bluepill_f103c8/firmware.elf` |

To run:

1. Build the firmware: `pio run`
2. Open `diagram.json` in Wokwi (or use the Wokwi VS Code extension).
3. Click **Start** to simulate.

Wokwi simulates the STM32 Blue Pill, DHT22, LDR, PIR sensor, KY-040
rotary encoder, SSD1306 OLED, and buzzer using the pin connections defined
in `diagram.json`.

## Known Wokwi Limitations

- **Serial output** is not reliably observable due to simulation timing
  constraints. Runtime correctness of UART diagnostic messages was not
  verified through Wokwi.
- **OLED display output** and other visual outputs (buzzer sound, LED blink
  patterns) were not reliably verifiable in Wokwi simulation. Functional
  correctness is based on code review and static analysis.
- **LDR readings** are a relative 0-100 scale derived from the 12-bit ADC
  value; they are not calibrated to lux.
- **Native unit tests** use small HAL and FreeRTOS mocks (`stm32f1xx_hal.h`
  and `FreeRTOS.h` at the project root) that stub all hardware access. The
  mocks compile to empty on STM32 builds via `#ifdef UNIT_TEST` guards.
