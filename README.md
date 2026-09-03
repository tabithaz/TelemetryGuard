# TelemetryGuard

TelemetryGuard is a lightweight C++ telemetry monitoring tool inspired by aerospace vehicle health-monitoring systems. It processes simulated vehicle telemetry and flags measurements that fall outside configured operating limits.

The project uses synthetic data only and is intended as a small systems-programming and telemetry-validation project.

## Initial telemetry channels

- Altitude
- Velocity
- Temperature
- Pressure
- Battery voltage

## Build

```bash
cmake -S . -B build
cmake --build build
./build/telemetry_guard
```

## Roadmap

- Telemetry packet model
- Configurable operating limits
- Warning and critical alerts
- Missing/stale measurement detection
- CSV telemetry playback
- Event logging
- Unit tests
