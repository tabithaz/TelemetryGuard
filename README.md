# TelemetryGuard

TelemetryGuard is a C++17 telemetry health-monitoring project inspired by aerospace vehicle operations. It evaluates synthetic channel data, classifies telemetry quality, summarizes vehicle health, and exposes reusable diagnostics for timing, integrity, and signal-behavior faults.

All scenarios are synthetic and contain no operational data.

## What it demonstrates

- Defensive validation of telemetry values, timestamps, and channel configuration
- Warning and critical limit evaluation with freshness-aware status classification
- Stable per-channel diagnostic reason codes for root-cause automation
- GO, MONITOR, and HOLD vehicle dispositions with script-friendly exit codes
- Baseline comparisons that identify channel status regressions and recoveries
- Structured transition-event streams for alerting and incident pipelines
- Health score, availability, degradation, and priority-channel reporting
- Configurable health-score SLO gates for CI and deployment automation
- Configurable telemetry-availability SLO gates for deployment decisions
- Minimum channel-count gates that detect incomplete telemetry snapshots
- Required-channel identity gates that catch missing mission-critical signals
- Native GitHub Actions warnings and errors for degraded telemetry channels
- Self-contained HTML health reports for review and attachment
- SARIF 2.1.0 findings for standardized analysis and code-scanning pipelines
- Per-channel limit margins and portfolio-level minimum warning headroom
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
for a missing reading. Fields may use standard CSV double-quote escaping, so channel
names and units can contain commas and literal quotes. A malformed or unterminated quoted
field returns exit code 3 with a line number; GO, MONITOR, and HOLD still return
0, 1, and 2 respectively.
Channel names must be unique and cannot be blank or padded with whitespace;
invalid input is rejected before any report or metrics are emitted.

Compare a current telemetry snapshot with a previous snapshot using `--baseline`.
Both files must contain the same channel identities. Human, JSON, NDJSON, and
Prometheus outputs report regressions, recoveries, and unchanged channels.
Units, warning and critical limits, and freshness thresholds must also match;
configuration drift is rejected before output so status changes are not
calculated across incompatible telemetry definitions:

```bash
./build/telemetry_guard --csv current.csv --baseline previous.csv --json --fail-on never
```

Use `--events` with a baseline to emit only actionable status changes as
newline-delimited JSON. Unchanged channels are suppressed, each transition
records its direction and previous/current state, and a final summary reports
the regression and recovery totals:

```bash
./build/telemetry_guard --csv current.csv --baseline previous.csv --events --fail-on never
```

Add `--margin-drop-percent` to surface predictive events before a channel's
status changes. TelemetryGuard compares normalized warning-limit headroom and
emits a `margin_regression` event when the drop reaches the configured
percentage-point threshold. Status transitions take precedence, so each
channel emits at most one event per comparison:

```bash
./build/telemetry_guard --csv current.csv --baseline previous.csv --events \
  --margin-drop-percent 25 --fail-on never
```

Use `--csv -` to read the same format from standard input. This supports direct
pipeline integration without an intermediate file and works with every output mode:

```bash
cat examples/readings.csv | ./build/telemetry_guard --csv - --json
```

Emit a standard JUnit XML report for CI systems that can publish test results:

```bash
./build/telemetry_guard --csv examples/readings.csv --junit --fail-on never \
  > telemetry-health.xml
```

Each channel becomes a test case, every non-nominal status becomes a failure,
and suite properties record the health score, health band, availability, and
final disposition. The existing `--fail-on` policy still controls the process
exit code independently of the report.

Use `--github-annotations` in GitHub Actions to surface degraded channels
directly in the workflow log. Warning and aging states create warning
annotations, HOLD-triggering states create errors, and a final notice records
the disposition, health score, and blocking-issue count. Workflow-command
characters in channel names and units are escaped safely:

```bash
./build/telemetry_guard --csv examples/readings.csv --github-annotations
```

Generate a polished, self-contained HTML health report that can be opened in a
browser or attached to an incident record without JavaScript or external assets:

```bash
./build/telemetry_guard --csv examples/readings.csv --html --fail-on never \
  > telemetry-health.html
```

The report includes the disposition, health score, availability, blocking-issue
count, priority channel, and a color-coded channel table. The existing
`--fail-on` policy still controls the process exit code.

Emit SARIF 2.1.0 for code-scanning and standardized static-analysis tooling:

```bash
./build/telemetry_guard --csv examples/readings.csv --sarif --fail-on never \
  > telemetry-health.sarif
```

Each degraded channel becomes a finding with a stable TelemetryGuard rule ID,
warning or error severity, status, value, unit, and age. Nominal channels are
omitted, while the run records the overall disposition and policy exit code.

Add `--json` to emit machine-readable channel results and the complete health
summary instead of the formatted report:

```bash
./build/telemetry_guard --csv examples/readings.csv --json
```

Missing or non-finite values are represented as JSON `null`. The command keeps
the same GO, MONITOR, HOLD, and input-error exit codes, which makes the JSON mode
usable in CI checks and monitoring pipelines without parsing presentation text.
Every channel also includes a stable `reason` such as
`value_above_critical_maximum`, `age_above_maximum`, or `missing_value`, so
automation can distinguish the cause of two channels with the same status.

Use `--ndjson` for streaming pipelines and log shippers. It writes one JSON
record per channel followed by a final summary record, so consumers can process
results incrementally without buffering the full report:

```bash
cat examples/readings.csv | ./build/telemetry_guard --csv - --ndjson --fail-on never
```

Write any report mode directly to an artifact with `--output`. The report is
kept off standard output, input failures do not replace the destination, and
the process still returns the configured GO, MONITOR, or HOLD policy exit code:

```bash
./build/telemetry_guard --csv examples/readings.csv --html \
  --output telemetry-health.html --fail-on never
```

Choose how health states affect automation with `--fail-on monitor|hold|never`.
The default, `monitor`, preserves the strict exit codes in the table below.
`hold` allows MONITOR reports to exit successfully while HOLD still fails, and
`never` returns success for every valid health report. Input errors always return
exit code 3 regardless of this policy.

```bash
./build/telemetry_guard --csv examples/readings.csv --json --fail-on hold
```

Enforce a minimum acceptable health score with `--min-health-score 0-100`.
If the calculated score falls below the threshold, the reported disposition is
promoted to HOLD and the normal `--fail-on` policy determines the exit code.
Structured outputs include the configured threshold and whether the gate passed;
Prometheus output exports both values as gauges:

```bash
./build/telemetry_guard --csv examples/readings.csv --json --min-health-score 90
```

Enforce a minimum percentage of available channels with `--min-availability`.
Stale, missing, invalid-timestamp, and configuration-error channels count as
unavailable. A failed SLO promotes the disposition to HOLD, while `--fail-on`
controls the exit code. JSON, NDJSON, transition-event, JUnit, GitHub annotation,
Prometheus, and human reports expose the configured target and gate result.

```bash
./build/telemetry_guard --csv examples/readings.csv --json \
  --min-availability 99.9 --fail-on hold
```

Detect silently truncated snapshots with `--min-channels`. If fewer channels
arrive than the configured floor, TelemetryGuard promotes the disposition to
HOLD even when every received value is nominal. Human, JSON, NDJSON,
transition-event, JUnit, GitHub annotation, and Prometheus reports expose the
expected count and whether the completeness gate passed.

```bash
./build/telemetry_guard --csv examples/readings.csv --json \
    --min-channels 8 --fail-on hold
```

Require specific critical signals with repeatable `--require-channel` options.
This catches a missing named channel even when the snapshot still satisfies its
minimum count. A missing requirement promotes the disposition to HOLD and is
reported in JSON, NDJSON, JUnit, GitHub annotations, Prometheus, and human output.

```bash
./build/telemetry_guard --csv examples/readings.csv --json \
  --require-channel "Altitude" --require-channel "Battery Voltage"
```

For larger deployments, store the required signal inventory in source control and
load it with `--require-channels-file`. The manifest uses one exact channel name
per line; blank lines and lines beginning with `#` are ignored. File entries can
be combined with `--require-channel`, and duplicates are rejected so the policy
remains unambiguous.

```text
# flight-critical channels
Altitude
Battery Voltage
Guidance Quality
```

```bash
./build/telemetry_guard --csv examples/readings.csv --json \
  --require-channels-file config/required-channels.txt --fail-on hold
```

When comparing a baseline, enforce a channel-regression budget with
`--max-regressions`. Exceeding the budget promotes the disposition to HOLD,
while `--fail-on` still controls the process exit policy. JSON, NDJSON, event,
JUnit, GitHub annotation, and Prometheus outputs expose the gate result:

```bash
./build/telemetry_guard --csv current.csv --baseline previous.csv --json \
  --max-regressions 0
```

Use `--prometheus` instead of `--json` to emit Prometheus exposition text for
direct ingestion by monitoring infrastructure:

```bash
./build/telemetry_guard --csv examples/readings.csv --prometheus
```

The metrics include health score, availability, degradation, per-status channel
counts, blocking issues, minimum warning-limit headroom, and a one-hot
GO/MONITOR/HOLD disposition. This mode
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

The command-line application assigns the most severe applicable state to each channel. Configuration errors and unavailable data are checked before normal limit evaluation so an invalid reading cannot be mistaken for nominal telemetry. For finite readings with valid limits, it also runs the reusable limit-margin diagnostic and reports distance to the nearest warning and critical boundary plus normalized warning headroom. The summary then aggregates those states into availability, degradation, health score, minimum headroom, priority channel, and final disposition.

The reusable analyzers are intentionally small and dependency-free. This keeps them easy to test in isolation and makes their behavior explicit enough for systems-oriented code review.

## Next milestones

- Integrate selected diagnostics into a configurable monitoring pipeline
- Emit structured event logs for downstream analysis
