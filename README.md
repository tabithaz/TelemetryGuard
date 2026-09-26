# TelemetryGuard

TelemetryGuard is a C++17 telemetry health-monitoring project inspired by aerospace vehicle operations. It evaluates synthetic channel data, classifies telemetry quality, summarizes vehicle health, and exposes reusable diagnostics for timing, integrity, and signal-behavior faults.

All scenarios are synthetic and contain no operational data.

## What it demonstrates

- Defensive validation of telemetry values, timestamps, and channel configuration
- Warning and critical limit evaluation with freshness-aware status classification
- GO, MONITOR, and HOLD vehicle dispositions with script-friendly exit codes
- Health score, availability, degradation, and priority-channel reporting
- Small header-only diagnostic components with focused regression tests
- Portable CMake builds and CI across GCC, Clang, and Apple Clang
- AddressSanitizer and UndefinedBehaviorSanitizer validation in CI

## Diagnostic coverage

TelemetryGuard includes focused analyzers for:

| Area | Diagnostics |
| --- | --- |
| Value limits | limit margin, saturation, slew rate, rate of change, step change |
| Signal behavior | bias shift, variance shift, noise floor, oscillation, flatline, frozen signal, deadband, quantization, spikes, outlier runs |
| Timing and delivery | freshness, jitter, packet gaps, dropout, timestamp drift, timestamp monotonicity |
| Data integrity | checksum integrity, sequence integrity, counter regression, stuck bits |
| Redundancy and range | sensor agreement, sensor drift, range utilization |

Each analyzer has a matching executable regression test registered with CTest.

## Build

Requirements:

- CMake 3.16 or newer
- A C++17 compiler

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Run the synthetic vehicle health check

```bash
./build/telemetry_guard
```

Run your own synthetic snapshot with `./build/telemetry_guard --csv examples/readings.csv`.
The CSV header must match the example exactly. Each row has a channel name, reading,
warning bounds, critical bounds, unit, age, warning age, and maximum age. Use `NA`
for a missing reading. Fields are unquoted and cannot contain commas. A malformed
file returns exit code 3 with a line number; GO, MONITOR, and HOLD still return
0, 1, and 2 respectively.
Channel names must be unique and cannot be blank or padded with whitespace;
invalid input is rejected before any report or metrics are emitted.

Use `--csv -` to read the same format from standard input. This supports direct
pipeline integration without an intermediate file and works with every output mode:

```bash
cat examples/readings.csv | ./build/telemetry_guard --csv - --json
```

Add `--json` to emit machine-readable channel results and the complete health
summary instead of the formatted report:

```bash
./build/telemetry_guard --csv examples/readings.csv --json
```

Missing or non-finite values are represented as JSON `null`. The command keeps
the same GO, MONITOR, HOLD, and input-error exit codes, which makes the JSON mode
usable in CI checks and monitoring pipelines without parsing presentation text.

Use `--ndjson` for streaming pipelines and log shippers. It writes one JSON
record per channel followed by a final summary record, so consumers can process
results incrementally without buffering the full report:

```bash
cat examples/readings.csv | ./build/telemetry_guard --csv - --ndjson --fail-on never
```

Choose how health states affect automation with `--fail-on monitor|hold|never`.
The default, `monitor`, preserves the strict exit codes in the table below.
`hold` allows MONITOR reports to exit successfully while HOLD still fails, and
`never` returns success for every valid health report. Input errors always return
exit code 3 regardless of this policy.

```bash
./build/telemetry_guard --csv examples/readings.csv --json --fail-on hold
```

Use `--prometheus` instead of `--json` to emit Prometheus exposition text for
direct ingestion by monitoring infrastructure:

```bash
./build/telemetry_guard --csv examples/readings.csv --prometheus
```

The metrics include health score, availability, degradation, per-status channel
counts, blocking issues, and a one-hot GO/MONITOR/HOLD disposition. This mode
keeps the same disposition exit codes and can feed Prometheus alerts or Grafana
dashboards without parsing the human-readable report.

The built-in scenario reports channel statuses followed by a mission-style summary:

```text
Nominal readings: 2
Warnings: 1
Critical alerts: 1
Aging readings: 1
Stale readings: 1
Missing readings: 1
Invalid timestamps: 1
Vehicle disposition: HOLD
```

The executable returns an exit code matching the final disposition:

| Disposition | Exit code | Meaning |
| --- | ---: | --- |
| `GO` | 0 | All monitored telemetry is nominal |
| `MONITOR` | 1 | At least one warning or aging condition is present |
| `HOLD` | 2 | A blocking condition is present |

Blocking conditions include critical values, stale or missing data, invalid timestamps, and invalid channel configuration.

## Run the test suite

```bash
ctest --test-dir build --output-on-failure
```

The suite covers the end-to-end synthetic health check plus every reusable diagnostic component. The GitHub Actions workflow builds and tests release configurations on Linux and macOS, treats compiler warnings as errors, and runs the Linux suite with address and undefined-behavior sanitizers.

## Project structure

```text
.
├── src/
│   ├── main.cpp                 # synthetic health-check application
│   └── *.hpp                    # reusable telemetry diagnostics
├── tests/
│   ├── *_test.cpp               # focused diagnostic regression tests
│   └── synthetic_health_check.cmake
├── .github/workflows/build.yml  # compiler matrix and sanitizer CI
└── CMakeLists.txt
```

## Design notes

The command-line application assigns the most severe applicable state to each channel. Configuration errors and unavailable data are checked before normal limit evaluation so an invalid reading cannot be mistaken for nominal telemetry. The summary then aggregates those states into availability, degradation, health score, priority channel, and final disposition.

The reusable analyzers are intentionally small and dependency-free. This keeps them easy to test in isolation and makes their behavior explicit enough for systems-oriented code review.

## Next milestones

- Integrate selected diagnostics into a configurable monitoring pipeline
- Emit structured event logs for downstream analysis
