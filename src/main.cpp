#include <algorithm>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

struct TelemetryReading {
    std::string channel;
    double value;
    double warningMinimum;
    double warningMaximum;
    double criticalMinimum;
    double criticalMaximum;
    std::string unit;
    double ageSeconds;
    double warningAgeSeconds;
    double maxAgeSeconds;
};

enum class TelemetryStatus {
    Nominal,
    Warning,
    Critical,
    Aging,
    Stale,
    MissingData,
    InvalidTimestamp,
    InvalidConfiguration
};

bool hasValidConfiguration(const TelemetryReading& reading) {
    return std::isfinite(reading.warningMinimum) &&
           std::isfinite(reading.warningMaximum) &&
           std::isfinite(reading.criticalMinimum) &&
           std::isfinite(reading.criticalMaximum) &&
           reading.criticalMinimum <= reading.warningMinimum &&
           reading.warningMinimum <= reading.warningMaximum &&
           reading.warningMaximum <= reading.criticalMaximum &&
           std::isfinite(reading.warningAgeSeconds) &&
           reading.warningAgeSeconds >= 0.0 &&
           std::isfinite(reading.maxAgeSeconds) &&
           reading.warningAgeSeconds <= reading.maxAgeSeconds;
}

TelemetryStatus evaluateReading(const TelemetryReading& reading) {
    if (!hasValidConfiguration(reading)) {
        return TelemetryStatus::InvalidConfiguration;
    }
    if (!std::isfinite(reading.value)) {
        return TelemetryStatus::MissingData;
    }
    if (!std::isfinite(reading.ageSeconds) || reading.ageSeconds < 0.0) {
        return TelemetryStatus::InvalidTimestamp;
    }
    if (reading.ageSeconds > reading.maxAgeSeconds) {
        return TelemetryStatus::Stale;
    }
    if (reading.value < reading.criticalMinimum || reading.value > reading.criticalMaximum) {
        return TelemetryStatus::Critical;
    }
    if (reading.value < reading.warningMinimum || reading.value > reading.warningMaximum) {
        return TelemetryStatus::Warning;
    }
    if (reading.ageSeconds > reading.warningAgeSeconds) {
        return TelemetryStatus::Aging;
    }
    return TelemetryStatus::Nominal;
}

std::string statusLabel(TelemetryStatus status) {
    switch (status) {
        case TelemetryStatus::Nominal: return "NOMINAL";
        case TelemetryStatus::Warning: return "WARNING";
        case TelemetryStatus::Critical: return "CRITICAL";
        case TelemetryStatus::Aging: return "AGING";
        case TelemetryStatus::Stale: return "STALE";
        case TelemetryStatus::MissingData: return "NO DATA";
        case TelemetryStatus::InvalidTimestamp: return "BAD TIMESTAMP";
        case TelemetryStatus::InvalidConfiguration: return "CONFIG ERROR";
    }
    return "UNKNOWN";
}

int statusPriority(TelemetryStatus status) {
    switch (status) {
        case TelemetryStatus::InvalidConfiguration: return 7;
        case TelemetryStatus::MissingData: return 6;
        case TelemetryStatus::Critical: return 5;
        case TelemetryStatus::Stale: return 4;
        case TelemetryStatus::InvalidTimestamp: return 3;
        case TelemetryStatus::Warning: return 2;
        case TelemetryStatus::Aging: return 1;
        case TelemetryStatus::Nominal: return 0;
    }
    return 0;
}

bool requiresHold(TelemetryStatus status) {
    return status == TelemetryStatus::Critical ||
           status == TelemetryStatus::Stale ||
           status == TelemetryStatus::MissingData ||
           status == TelemetryStatus::InvalidTimestamp ||
           status == TelemetryStatus::InvalidConfiguration;
}

std::string vehicleDisposition(int warningCount, int criticalCount, int agingCount,
                               int staleCount, int missingDataCount,
                               int invalidTimestampCount, int configurationErrorCount) {
    const int blockingIssues = criticalCount + staleCount + missingDataCount +
                               invalidTimestampCount + configurationErrorCount;
    if (blockingIssues > 0) return "HOLD";
    if (warningCount > 0 || agingCount > 0) return "MONITOR";
    return "GO";
}

int dispositionExitCode(const std::string& disposition) {
    if (disposition == "HOLD") return 2;
    if (disposition == "MONITOR") return 1;
    return 0;
}

int vehicleHealthScore(int totalReadings, int warningCount, int criticalCount,
                       int agingCount, int staleCount, int missingDataCount,
                       int invalidTimestampCount, int configurationErrorCount) {
    if (totalReadings <= 0) return 100;

    const int penalty = warningCount * 8 + agingCount * 5 + criticalCount * 25 +
                        staleCount * 20 + missingDataCount * 25 +
                        invalidTimestampCount * 20 + configurationErrorCount * 25;
    return std::max(0, 100 - penalty);
}

double telemetryAvailabilityPercent(int totalReadings, int staleCount,
                                    int missingDataCount, int invalidTimestampCount,
                                    int configurationErrorCount) {
    if (totalReadings <= 0) return 100.0;

    const int unavailableReadings = staleCount + missingDataCount +
                                    invalidTimestampCount + configurationErrorCount;
    const int availableReadings = std::max(0, totalReadings - unavailableReadings);
    return 100.0 * static_cast<double>(availableReadings) /
           static_cast<double>(totalReadings);
}

double telemetryDegradationPercent(int totalReadings, int nominalCount) {
    if (totalReadings <= 0) return 0.0;
    const int degradedReadings = std::max(0, totalReadings - nominalCount);
    return 100.0 * static_cast<double>(degradedReadings) /
           static_cast<double>(totalReadings);
}

std::string healthBand(int healthScore) {
    if (healthScore >= 90) return "GREEN";
    if (healthScore >= 70) return "AMBER";
    return "RED";
}

double parseNumber(const std::string& input) {
    if (input == "NA") return std::numeric_limits<double>::quiet_NaN();
    std::size_t consumed = 0;
    const double value = std::stod(input, &consumed);
    if (consumed != input.size()) throw std::invalid_argument("invalid number");
    return value;
}

std::vector<TelemetryReading> readCsv(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open input file: " + path);
    const std::string header = "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds";
    std::string line;
    if (!std::getline(file, line) || line != header) throw std::runtime_error("invalid CSV header");
    std::vector<TelemetryReading> readings;
    std::size_t lineNumber = 1;
    while (std::getline(file, line)) {
        ++lineNumber;
        try {
            std::vector<std::string> cells;
            std::istringstream row(line);
            std::string cell;
            while (std::getline(row, cell, ',')) cells.push_back(cell);
            if (cells.size() != 10 || cells[0].empty() || cells[6].empty())
                throw std::invalid_argument("expected ten nonempty fields");
            readings.push_back({cells[0], parseNumber(cells[1]), parseNumber(cells[2]),
                parseNumber(cells[3]), parseNumber(cells[4]), parseNumber(cells[5]),
                cells[6], parseNumber(cells[7]), parseNumber(cells[8]), parseNumber(cells[9])});
        } catch (const std::exception& error) {
            throw std::runtime_error("CSV line " + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (file.bad()) throw std::runtime_error("failed while reading input file");
    if (readings.empty()) throw std::runtime_error("CSV contains no readings");
    return readings;
}

std::string jsonEscape(const std::string& value) {
    std::ostringstream escaped;
    for (const unsigned char character : value) {
        switch (character) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (character < 0x20) {
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                            << static_cast<int>(character) << std::dec << std::setfill(' ');
                } else {
                    escaped << character;
                }
        }
    }
    return escaped.str();
}

int main(int argc, char* argv[]) {
    bool jsonOutput = false;
    std::string csvPath;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--json" && !jsonOutput) {
            jsonOutput = true;
        } else if (argument == "--csv" && csvPath.empty() && index + 1 < argc) {
            csvPath = argv[++index];
        } else {
            std::cerr << "Usage: telemetry_guard [--csv path] [--json]\n";
            return 3;
        }
    }
    const std::vector<TelemetryReading> sample = {
        {"Altitude", 18250.0, 0.0, 25000.0, -500.0, 27000.0, "m", 0.4, 1.5, 2.0},
        {"Velocity", 1240.0, 0.0, 1800.0, -100.0, 2000.0, "m/s", 0.7, 1.5, 2.0},
        {"Temperature", 91.5, -40.0, 85.0, -55.0, 100.0, "C", 0.3, 4.0, 5.0},
        {"Pressure", 238.0, 150.0, 300.0, 125.0, 325.0, "kPa", 6.2, 4.0, 5.0},
        {"Battery Voltage", 33.5, 24.0, 30.0, 22.0, 32.0, "V", 1.1, 4.0, 5.0},
        {"Fuel Level", std::numeric_limits<double>::quiet_NaN(), 0.0, 100.0, -1.0, 101.0, "%", 0.8, 4.0, 5.0},
        {"Guidance Quality", 98.0, 90.0, 100.0, 80.0, 105.0, "%", -0.2, 1.5, 2.0},
        {"Navigation Update", 97.0, 90.0, 100.0, 80.0, 105.0, "%", 1.7, 1.5, 2.0}
    };

    std::vector<TelemetryReading> readings;
    try {
        readings = csvPath.empty() ? sample : readCsv(csvPath);
    } catch (const std::exception& error) {
        std::cerr << "Input error: " << error.what() << '\n';
        return 3;
    }
    if (!jsonOutput) std::cout << "TelemetryGuard - Vehicle Health Check\n\n";

    int nominalCount = 0;
    int warningCount = 0;
    int criticalCount = 0;
    int agingCount = 0;
    int staleCount = 0;
    int missingDataCount = 0;
    int invalidTimestampCount = 0;
    int configurationErrorCount = 0;
    int blockingIssueCount = 0;
    int highestPriority = -1;
    std::string priorityChannel = "None";
    TelemetryStatus priorityStatus = TelemetryStatus::Nominal;
    std::vector<TelemetryStatus> statuses;

    for (const auto& reading : readings) {
        const TelemetryStatus status = evaluateReading(reading);
        statuses.push_back(status);
        if (!jsonOutput) {
            std::cout << std::left << std::setw(18) << reading.channel << std::setw(10);
            if (status == TelemetryStatus::MissingData) std::cout << "N/A";
            else std::cout << reading.value;
            std::cout << std::setw(8) << reading.unit << std::setw(14) << statusLabel(status)
                      << "age=" << reading.ageSeconds << "s\n";
        }

        if (status == TelemetryStatus::Nominal) ++nominalCount;
        else if (status == TelemetryStatus::Warning) ++warningCount;
        else if (status == TelemetryStatus::Critical) ++criticalCount;
        else if (status == TelemetryStatus::Aging) ++agingCount;
        else if (status == TelemetryStatus::Stale) ++staleCount;
        else if (status == TelemetryStatus::MissingData) ++missingDataCount;
        else if (status == TelemetryStatus::InvalidTimestamp) ++invalidTimestampCount;
        else if (status == TelemetryStatus::InvalidConfiguration) ++configurationErrorCount;

        if (requiresHold(status)) ++blockingIssueCount;
        const int priority = statusPriority(status);
        if (priority > highestPriority) {
            highestPriority = priority;
            priorityChannel = reading.channel;
            priorityStatus = status;
        }
    }

    const int totalReadings = static_cast<int>(readings.size());
    const int healthScore = vehicleHealthScore(
        totalReadings, warningCount, criticalCount, agingCount, staleCount,
        missingDataCount, invalidTimestampCount, configurationErrorCount);
    const double availability = telemetryAvailabilityPercent(
        totalReadings, staleCount, missingDataCount, invalidTimestampCount,
        configurationErrorCount);
    const double degradation = telemetryDegradationPercent(totalReadings, nominalCount);
    const std::string disposition = vehicleDisposition(
        warningCount, criticalCount, agingCount, staleCount, missingDataCount,
        invalidTimestampCount, configurationErrorCount);

    if (jsonOutput) {
        std::cout << "{\"channels\":[";
        for (std::size_t index = 0; index < readings.size(); ++index) {
            if (index > 0) std::cout << ',';
            const auto& reading = readings[index];
            std::cout << "{\"channel\":\"" << jsonEscape(reading.channel)
                      << "\",\"value\":";
            if (std::isfinite(reading.value)) std::cout << reading.value;
            else std::cout << "null";
            std::cout << ",\"unit\":\"" << jsonEscape(reading.unit)
                      << "\",\"age_seconds\":";
            if (std::isfinite(reading.ageSeconds)) std::cout << reading.ageSeconds;
            else std::cout << "null";
            std::cout << ",\"status\":\"" << statusLabel(statuses[index]) << "\"}";
        }
        std::cout << "],\"summary\":{\"total_readings\":" << totalReadings
                  << ",\"nominal\":" << nominalCount
                  << ",\"warnings\":" << warningCount
                  << ",\"critical\":" << criticalCount
                  << ",\"aging\":" << agingCount
                  << ",\"stale\":" << staleCount
                  << ",\"missing\":" << missingDataCount
                  << ",\"invalid_timestamps\":" << invalidTimestampCount
                  << ",\"configuration_errors\":" << configurationErrorCount
                  << ",\"blocking_issues\":" << blockingIssueCount
                  << ",\"priority_channel\":\"" << jsonEscape(priorityChannel)
                  << "\",\"priority_status\":\"" << statusLabel(priorityStatus)
                  << "\",\"availability_percent\":" << std::fixed << std::setprecision(1)
                  << availability << ",\"degradation_percent\":" << degradation
                  << ",\"health_score\":" << healthScore
                  << ",\"health_band\":\"" << healthBand(healthScore)
                  << "\",\"disposition\":\"" << disposition << "\"}}\n";
    } else std::cout << "\nNominal readings: " << nominalCount << '\n'
              << "Warnings: " << warningCount << '\n'
              << "Critical alerts: " << criticalCount << '\n'
              << "Aging readings: " << agingCount << '\n'
              << "Stale readings: " << staleCount << '\n'
              << "Missing readings: " << missingDataCount << '\n'
              << "Invalid timestamps: " << invalidTimestampCount << '\n'
              << "Configuration errors: " << configurationErrorCount << '\n'
              << "Blocking issues: " << blockingIssueCount << '\n'
              << "Priority channel: " << priorityChannel << " ("
              << statusLabel(priorityStatus) << ")\n"
              << "Telemetry availability: " << std::fixed << std::setprecision(1)
              << availability << "%\n"
              << "Telemetry degradation: " << degradation << "%\n"
              << "Vehicle health score: " << healthScore << "/100\n"
              << "Health band: " << healthBand(healthScore) << '\n'
              << "Vehicle disposition: " << disposition << '\n';

    return dispositionExitCode(disposition);
}
