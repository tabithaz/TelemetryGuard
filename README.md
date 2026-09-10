# TelemetryGuard

TelemetryGuard is a lightweight C++ telemetry monitoring tool inspired by aerospace vehicle health-monitoring systems. It evaluates synthetic vehicle telemetry against configurable value and freshness limits, then produces channel-level status and an overall vehicle health summary.

The project uses synthetic data only and is intended as a small systems-programming and telemetry-validation project.

## Current capabilities

- Configurable warning and critical value thresholds per telemetry channel
- Freshness thresholds for aging and stale telemetry
- Detection of missing values, invalid timestamps, and invalid threshold configurations
- Channel-level status classification with priority diagnostics
- Vehicle health score, health band, availability, and degradation metrics
- GO, MONITOR, and HOLD vehicle dispositions
- Script-friendly exit codes for downstream tooling
- CTest regression coverage for the synthetic health scenario

## Synthetic telemetry channels

The built-in scenario currently exercises channels such as altitude, velocity, temperature, pressure, battery voltage, fuel level, guidance quality, and navigation updates. These values and scenarios are synthetic and are not derived from operational systems.

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/telemetry_guard
```

The executable returns an exit code that matches the vehicle disposition:

| Disposition | Exit code | Meaning |
| --- | ---: | --- |
| `GO` | 0 | All monitored telemetry is nominal |
| `MONITOR` | 1 | One or more warning or aging conditions are present |
| `HOLD` | 2 | A blocking condition such as critical, stale, missing, invalid-timestamp, or configuration-error telemetry is present |

## Test

```bash
ctest --test-dir build --output-on-failure
```

The regression test validates the expected synthetic health summary and HOLD exit behavior.

## Roadmap

- Separate telemetry evaluation logic from the command-line application
- Load telemetry samples from CSV for playback
- Add structured event logging
- Add focused unit tests for individual status and health calculations
