#include <algorithm>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "limit_margin.hpp"

#ifndef TELEMETRY_GUARD_VERSION
#define TELEMETRY_GUARD_VERSION "development"
#endif

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

bool hasMatchingConfiguration(const TelemetryReading& current,
                              const TelemetryReading& baseline) {
    return current.unit == baseline.unit &&
           current.warningMinimum == baseline.warningMinimum &&
           current.warningMaximum == baseline.warningMaximum &&
           current.criticalMinimum == baseline.criticalMinimum &&
           current.criticalMaximum == baseline.criticalMaximum &&
           current.warningAgeSeconds == baseline.warningAgeSeconds &&
           current.maxAgeSeconds == baseline.maxAgeSeconds;
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

std::string diagnosticReason(const TelemetryReading& reading,
                             TelemetryStatus status) {
    switch (status) {
        case TelemetryStatus::InvalidConfiguration:
            return "invalid_configuration";
        case TelemetryStatus::MissingData:
            return "missing_value";
        case TelemetryStatus::InvalidTimestamp:
            return "invalid_age";
        case TelemetryStatus::Stale:
            return "age_above_maximum";
        case TelemetryStatus::Critical:
            return reading.value < reading.criticalMinimum
                ? "value_below_critical_minimum"
                : "value_above_critical_maximum";
        case TelemetryStatus::Warning:
            return reading.value < reading.warningMinimum
                ? "value_below_warning_minimum"
                : "value_above_warning_maximum";
        case TelemetryStatus::Aging:
            return "age_above_warning";
        case TelemetryStatus::Nominal:
            return "within_limits";
    }
    return "unknown";
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

int policyExitCode(const std::string& disposition, const std::string& failOn) {
    if (failOn == "never") return 0;
    if (failOn == "hold" && disposition == "MONITOR") return 0;
    return dispositionExitCode(disposition);
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

std::optional<LimitMargin> readingLimitMargin(const TelemetryReading& reading) {
    try {
        return calculateLimitMargin(
            reading.value,
            reading.warningMinimum,
            reading.warningMaximum,
            reading.criticalMinimum,
            reading.criticalMaximum);
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
}

std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> cells;
    std::string cell;
    bool insideQuotes = false;
    bool quotedFieldClosed = false;

    for (std::size_t index = 0; index < line.size(); ++index) {
        const char character = line[index];
        if (insideQuotes) {
            if (character == '"') {
                if (index + 1 < line.size() && line[index + 1] == '"') {
                    cell.push_back('"');
                    ++index;
                } else {
                    insideQuotes = false;
                    quotedFieldClosed = true;
                }
            } else {
                cell.push_back(character);
            }
        } else if (quotedFieldClosed) {
            if (character != ',')
                throw std::invalid_argument("unexpected character after quoted field");
            cells.push_back(cell);
            cell.clear();
            quotedFieldClosed = false;
        } else if (character == ',') {
            cells.push_back(cell);
            cell.clear();
        } else if (character == '"') {
            if (!cell.empty())
                throw std::invalid_argument("unexpected quote in unquoted field");
            insideQuotes = true;
        } else {
            cell.push_back(character);
        }
    }

    if (insideQuotes) throw std::invalid_argument("unterminated quoted field");
    cells.push_back(cell);
    return cells;
}

std::vector<TelemetryReading> readCsv(std::istream& input,
                                      std::size_t maximumChannels) {
    const std::string header = "channel,value,warning_min,warning_max,critical_min,critical_max,unit,age_seconds,warning_age_seconds,max_age_seconds";
    std::string line;
    if (!std::getline(input, line)) throw std::runtime_error("invalid CSV header");
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != header) throw std::runtime_error("invalid CSV header");
    std::vector<TelemetryReading> readings;
    std::set<std::string> channelNames;
    std::size_t lineNumber = 1;
    while (std::getline(input, line)) {
        ++lineNumber;
        try {
            if (readings.size() >= maximumChannels)
                throw std::invalid_argument(
                    "channel count exceeds configured maximum of " +
                    std::to_string(maximumChannels));
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<std::string> cells = parseCsvRow(line);
            if (cells.size() != 10 || cells[0].empty() || cells[6].empty())
                throw std::invalid_argument("expected ten nonempty fields");
            if (cells[0].find_first_not_of(" \t") == std::string::npos)
                throw std::invalid_argument("channel name cannot be blank");
            if (cells[0].find_first_not_of(" \t") != 0 ||
                cells[0].find_last_not_of(" \t") != cells[0].size() - 1)
                throw std::invalid_argument("channel name has surrounding whitespace");
            if (!channelNames.insert(cells[0]).second)
                throw std::invalid_argument("duplicate channel: " + cells[0]);
            readings.push_back({cells[0], parseNumber(cells[1]), parseNumber(cells[2]),
                parseNumber(cells[3]), parseNumber(cells[4]), parseNumber(cells[5]),
                cells[6], parseNumber(cells[7]), parseNumber(cells[8]), parseNumber(cells[9])});
        } catch (const std::exception& error) {
            throw std::runtime_error("CSV line " + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (input.bad()) throw std::runtime_error("failed while reading CSV input");
    if (readings.empty()) throw std::runtime_error("CSV contains no readings");
    return readings;
}

std::vector<TelemetryReading> readCsvFile(const std::string& path,
                                          std::size_t maximumChannels) {
    if (path == "-") return readCsv(std::cin, maximumChannels);
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open input file: " + path);
    return readCsv(file, maximumChannels);
}

std::vector<std::string> readRequiredChannelsFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open required channels file: " + path);
    std::vector<std::string> channels;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.front() == '#') continue;
        if (line.find_first_not_of(" \t") == std::string::npos ||
            line.find_first_not_of(" \t") != 0 ||
            line.find_last_not_of(" \t") != line.size() - 1)
            throw std::runtime_error("required channels file line " +
                std::to_string(lineNumber) +
                ": channel must be nonblank without surrounding whitespace");
        channels.push_back(line);
    }
    if (file.bad()) throw std::runtime_error("failed while reading required channels file: " + path);
    if (channels.empty()) throw std::runtime_error("required channels file contains no channels");
    return channels;
}

struct TelemetryPolicy {
    std::optional<int> minimumHealthScore;
    std::optional<double> minimumAvailabilityPercent;
    std::optional<int> minimumChannelCount;
    std::optional<int> maximumRegressions;
    std::optional<std::string> failOn;
    std::vector<std::string> requiredChannels;
};

std::string trimPolicyValue(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    return value.substr(first, value.find_last_not_of(" \t") - first + 1);
}

TelemetryPolicy readPolicyFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open policy file: " + path);

    TelemetryPolicy policy;
    std::set<std::string> scalarKeys;
    std::set<std::string> requiredChannels;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::string normalized = trimPolicyValue(line);
        if (normalized.empty() || normalized.front() == '#') continue;
        const std::size_t separator = normalized.find('=');
        if (separator == std::string::npos)
            throw std::runtime_error("policy file line " + std::to_string(lineNumber) +
                                     ": expected key=value");
        const std::string key = trimPolicyValue(normalized.substr(0, separator));
        const std::string value = trimPolicyValue(normalized.substr(separator + 1));
        if (key.empty() || value.empty())
            throw std::runtime_error("policy file line " + std::to_string(lineNumber) +
                                     ": key and value must be nonblank");

        try {
            if (key == "required_channel") {
                if (!requiredChannels.insert(value).second)
                    throw std::invalid_argument("duplicate required channel: " + value);
                policy.requiredChannels.push_back(value);
                continue;
            }
            if (!scalarKeys.insert(key).second)
                throw std::invalid_argument("duplicate policy key: " + key);
            std::size_t consumed = 0;
            if (key == "min_health_score") {
                const int threshold = std::stoi(value, &consumed);
                if (consumed != value.size() || threshold < 0 || threshold > 100)
                    throw std::invalid_argument("min_health_score must be an integer from 0 to 100");
                policy.minimumHealthScore = threshold;
            } else if (key == "min_availability") {
                const double threshold = std::stod(value, &consumed);
                if (consumed != value.size() || !std::isfinite(threshold) ||
                    threshold < 0.0 || threshold > 100.0)
                    throw std::invalid_argument("min_availability must be a number from 0 to 100");
                policy.minimumAvailabilityPercent = threshold;
            } else if (key == "min_channels") {
                const int minimum = std::stoi(value, &consumed);
                if (consumed != value.size() || minimum <= 0)
                    throw std::invalid_argument("min_channels must be a positive integer");
                policy.minimumChannelCount = minimum;
            } else if (key == "max_regressions") {
                const int maximum = std::stoi(value, &consumed);
                if (consumed != value.size() || maximum < 0)
                    throw std::invalid_argument("max_regressions must be a non-negative integer");
                policy.maximumRegressions = maximum;
            } else if (key == "fail_on") {
                if (value != "monitor" && value != "hold" && value != "never")
                    throw std::invalid_argument("fail_on must be monitor, hold, or never");
                policy.failOn = value;
            } else {
                throw std::invalid_argument("unknown policy key: " + key);
            }
        } catch (const std::exception& error) {
            throw std::runtime_error("policy file line " + std::to_string(lineNumber) +
                                     ": " + error.what());
        }
    }
    if (file.bad()) throw std::runtime_error("failed while reading policy file: " + path);
    return policy;
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

std::string csvField(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t");
    std::string safe = value;
    if (first != std::string::npos &&
        (value[first] == '=' || value[first] == '+' ||
         value[first] == '-' || value[first] == '@'))
        safe.insert(safe.begin(), '\'');

    if (safe.find_first_of(",\"\r\n") == std::string::npos) return safe;
    std::string escaped = "\"";
    for (const char character : safe) {
        if (character == '\"') escaped += "\"\"";
        else escaped += character;
    }
    escaped += '\"';
    return escaped;
}

std::string policyJson(const TelemetryPolicy& policy) {
    std::ostringstream output;
    output << std::setprecision(15)
           << "{\"valid\":true,\"policy\":{"
           << "\"min_health_score\":";
    if (policy.minimumHealthScore.has_value()) output << *policy.minimumHealthScore;
    else output << "null";
    output << ",\"min_availability\":";
    if (policy.minimumAvailabilityPercent.has_value())
        output << *policy.minimumAvailabilityPercent;
    else output << "null";
    output << ",\"min_channels\":";
    if (policy.minimumChannelCount.has_value()) output << *policy.minimumChannelCount;
    else output << "null";
    output << ",\"max_regressions\":";
    if (policy.maximumRegressions.has_value()) output << *policy.maximumRegressions;
    else output << "null";
    output << ",\"fail_on\":";
    if (policy.failOn.has_value()) output << '"' << jsonEscape(*policy.failOn) << '"';
    else output << "null";
    output << ",\"required_channels\":[";
    for (std::size_t index = 0; index < policy.requiredChannels.size(); ++index) {
        if (index > 0) output << ',';
        output << '"' << jsonEscape(policy.requiredChannels[index]) << '"';
    }
    output << "]}}";
    return output.str();
}

std::string xmlEscape(const std::string& value) {
    std::ostringstream escaped;
    for (const unsigned char character : value) {
        switch (character) {
            case '&': escaped << "&amp;"; break;
            case '<': escaped << "&lt;"; break;
            case '>': escaped << "&gt;"; break;
            case '"': escaped << "&quot;"; break;
            case '\'': escaped << "&apos;"; break;
            default:
                if (character == '\t' || character == '\n' || character == '\r' ||
                    character >= 0x20)
                    escaped << character;
        }
    }
    return escaped.str();
}

std::string htmlStatusClass(TelemetryStatus status) {
    if (status == TelemetryStatus::Nominal) return "nominal";
    if (status == TelemetryStatus::Warning || status == TelemetryStatus::Aging)
        return "warning";
    return "critical";
}

std::string sarifRuleId(TelemetryStatus status) {
    switch (status) {
        case TelemetryStatus::Warning: return "TG001";
        case TelemetryStatus::Critical: return "TG002";
        case TelemetryStatus::Aging: return "TG003";
        case TelemetryStatus::Stale: return "TG004";
        case TelemetryStatus::MissingData: return "TG005";
        case TelemetryStatus::InvalidTimestamp: return "TG006";
        case TelemetryStatus::InvalidConfiguration: return "TG007";
        case TelemetryStatus::Nominal: return "TG000";
    }
    return "TG000";
}

std::string sarifLevel(TelemetryStatus status) {
    return requiresHold(status) ? "error" : "warning";
}

std::string githubCommandEscape(const std::string& value, bool property) {
    std::ostringstream escaped;
    for (const char character : value) {
        if (character == '%') escaped << "%25";
        else if (character == '\r') escaped << "%0D";
        else if (character == '\n') escaped << "%0A";
        else if (property && character == ':') escaped << "%3A";
        else if (property && character == ',') escaped << "%2C";
        else escaped << character;
    }
    return escaped.str();
}

void printHelp(std::ostream& output) {
    output
        << "TelemetryGuard " << TELEMETRY_GUARD_VERSION << "\n\n"
        << "Validate telemetry health and enforce deployment gates.\n\n"
        << "Usage:\n"
        << "  telemetry_guard [options]\n"
        << "  telemetry_guard --check-policy PATH\n"
        << "  telemetry_guard --version\n\n"
        << "Input:\n"
        << "  --csv PATH                    Read current telemetry CSV (use - for stdin)\n"
        << "  --baseline PATH               Compare against a baseline CSV\n"
        << "  --max-input-channels COUNT    Bound channels loaded per CSV (default: 10000)\n"
        << "  --policy PATH                 Load a version-controlled gate policy\n"
        << "  --check-policy PATH           Validate and normalize a policy, then exit\n\n"
        << "Output (choose at most one):\n"
        << "  --json | --ndjson | --prometheus | --events | --junit\n"
        << "  --github-annotations | --html | --sarif | --report-csv\n"
        << "  --output PATH                 Write the selected report to a file\n\n"
        << "Gates:\n"
        << "  --fail-on monitor|hold|never  Set the disposition that causes failure\n"
        << "  --min-health-score 0-100      Require a minimum weighted health score\n"
        << "  --min-availability 0-100      Require a minimum availability percentage\n"
        << "  --min-channels COUNT          Require a minimum channel count\n"
        << "  --require-channel NAME        Require a channel (repeatable)\n"
        << "  --require-channels-file PATH  Load required channels from a file\n"
        << "  --max-regressions COUNT       Bound regressions versus the baseline\n"
        << "  --margin-drop-percent 0-100   Emit predictive margin regression events\n\n"
        << "Exit codes:\n"
        << "  0  Gate passed or --fail-on never\n"
        << "  1  MONITOR disposition met the configured failure policy\n"
        << "  2  HOLD disposition met the configured failure policy\n"
        << "  3  Invalid input, policy, option, or output destination\n";
}

int main(int argc, char* argv[]) {
    if (argc == 2 && (std::string(argv[1]) == "--help" ||
                      std::string(argv[1]) == "-h")) {
        printHelp(std::cout);
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "TelemetryGuard " << TELEMETRY_GUARD_VERSION << '\n';
        return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "--check-policy") {
        try {
            std::cout << policyJson(readPolicyFile(argv[2])) << '\n';
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Policy validation failed: " << error.what() << '\n';
            return 3;
        }
    }
    bool jsonOutput = false;
    bool ndjsonOutput = false;
    bool prometheusOutput = false;
    bool eventsOutput = false;
    bool junitOutput = false;
    bool githubOutput = false;
    bool htmlOutput = false;
    bool sarifOutput = false;
    bool csvReportOutput = false;
    std::string csvPath;
    std::string baselinePath;
    std::string outputPath;
    std::string failOn = "monitor";
    bool failOnProvided = false;
    std::optional<int> minimumHealthScore;
    std::optional<double> minimumAvailabilityPercent;
    std::optional<int> minimumChannelCount;
    std::optional<int> maximumRegressions;
    std::optional<double> marginDropPercent;
    std::size_t maximumInputChannels = 10000;
    bool maximumInputChannelsProvided = false;
    std::vector<std::string> requiredChannels;
    std::string requiredChannelsPath;
    std::string policyPath;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--json" && !jsonOutput) {
            jsonOutput = true;
        } else if (argument == "--ndjson" && !ndjsonOutput) {
            ndjsonOutput = true;
        } else if (argument == "--prometheus" && !prometheusOutput) {
            prometheusOutput = true;
        } else if (argument == "--events" && !eventsOutput) {
            eventsOutput = true;
        } else if (argument == "--junit" && !junitOutput) {
            junitOutput = true;
        } else if (argument == "--github-annotations" && !githubOutput) {
            githubOutput = true;
        } else if (argument == "--html" && !htmlOutput) {
            htmlOutput = true;
        } else if (argument == "--sarif" && !sarifOutput) {
            sarifOutput = true;
        } else if (argument == "--report-csv" && !csvReportOutput) {
            csvReportOutput = true;
        } else if (argument == "--csv" && csvPath.empty() && index + 1 < argc) {
            csvPath = argv[++index];
        } else if (argument == "--baseline" && baselinePath.empty() && index + 1 < argc) {
            baselinePath = argv[++index];
        } else if (argument == "--output" && outputPath.empty() && index + 1 < argc) {
            outputPath = argv[++index];
        } else if (argument == "--fail-on" && !failOnProvided && index + 1 < argc) {
            failOn = argv[++index];
            failOnProvided = true;
        } else if (argument == "--min-health-score" &&
                   !minimumHealthScore.has_value() && index + 1 < argc) {
            const std::string value = argv[++index];
            try {
                std::size_t consumed = 0;
                const int threshold = std::stoi(value, &consumed);
                if (consumed != value.size() || threshold < 0 || threshold > 100)
                    throw std::invalid_argument("out of range");
                minimumHealthScore = threshold;
            } catch (const std::exception&) {
                std::cerr << "Input error: --min-health-score must be an integer from 0 to 100\n";
                return 3;
            }
        } else if (argument == "--min-availability" &&
                   !minimumAvailabilityPercent.has_value() && index + 1 < argc) {
            const std::string value = argv[++index];
            try {
                std::size_t consumed = 0;
                const double threshold = std::stod(value, &consumed);
                if (consumed != value.size() || !std::isfinite(threshold) ||
                    threshold < 0.0 || threshold > 100.0)
                    throw std::invalid_argument("out of range");
                minimumAvailabilityPercent = threshold;
            } catch (const std::exception&) {
                std::cerr << "Input error: --min-availability must be a number from 0 to 100\n";
                return 3;
            }
        } else if (argument == "--min-channels" &&
                   !minimumChannelCount.has_value() && index + 1 < argc) {
            const std::string value = argv[++index];
            try {
                std::size_t consumed = 0;
                const int minimum = std::stoi(value, &consumed);
                if (consumed != value.size() || minimum <= 0)
                    throw std::invalid_argument("out of range");
                minimumChannelCount = minimum;
            } catch (const std::exception&) {
                std::cerr << "Input error: --min-channels must be a positive integer\n";
                return 3;
            }
        } else if (argument == "--require-channel" && index + 1 < argc) {
            const std::string channel = argv[++index];
            if (channel.empty() || channel.find_first_not_of(" \t") == std::string::npos ||
                channel.find_first_not_of(" \t") != 0 ||
                channel.find_last_not_of(" \t") != channel.size() - 1) {
                std::cerr << "Input error: --require-channel must be a nonblank channel name without surrounding whitespace\n";
                return 3;
            }
            if (std::find(requiredChannels.begin(), requiredChannels.end(), channel) !=
                requiredChannels.end()) {
                std::cerr << "Input error: duplicate --require-channel: " << channel << '\n';
                return 3;
            }
            requiredChannels.push_back(channel);
        } else if (argument == "--require-channels-file" &&
                   requiredChannelsPath.empty() && index + 1 < argc) {
            requiredChannelsPath = argv[++index];
            if (requiredChannelsPath.empty()) {
                std::cerr << "Input error: --require-channels-file needs a path\n";
                return 3;
            }
        } else if (argument == "--policy" && policyPath.empty() && index + 1 < argc) {
            policyPath = argv[++index];
            if (policyPath.empty()) {
                std::cerr << "Input error: --policy needs a path\n";
                return 3;
            }
        } else if (argument == "--margin-drop-percent" &&
                   !marginDropPercent.has_value() && index + 1 < argc) {
            const std::string value = argv[++index];
            try {
                std::size_t consumed = 0;
                const double threshold = std::stod(value, &consumed);
                if (consumed != value.size() || !std::isfinite(threshold) ||
                    threshold <= 0.0 || threshold > 100.0)
                    throw std::invalid_argument("out of range");
                marginDropPercent = threshold;
            } catch (const std::exception&) {
                std::cerr << "Input error: --margin-drop-percent must be a number "
                             "greater than 0 and at most 100\n";
                return 3;
            }
        } else if (argument == "--max-regressions" &&
                   !maximumRegressions.has_value() && index + 1 < argc) {
            const std::string value = argv[++index];
            try {
                std::size_t consumed = 0;
                const int maximum = std::stoi(value, &consumed);
                if (consumed != value.size() || maximum < 0)
                    throw std::invalid_argument("out of range");
                maximumRegressions = maximum;
            } catch (const std::exception&) {
                std::cerr << "Input error: --max-regressions must be a non-negative integer\n";
                return 3;
            }
        } else if (argument == "--max-input-channels" &&
                   !maximumInputChannelsProvided && index + 1 < argc) {
            const std::string value = argv[++index];
            try {
                if (value.empty() ||
                    value.find_first_not_of("0123456789") != std::string::npos)
                    throw std::invalid_argument("not an unsigned integer");
                std::size_t consumed = 0;
                const unsigned long long maximum = std::stoull(value, &consumed);
                if (consumed != value.size() || maximum == 0 ||
                    maximum > std::numeric_limits<std::size_t>::max())
                    throw std::invalid_argument("out of range");
                maximumInputChannels = static_cast<std::size_t>(maximum);
                maximumInputChannelsProvided = true;
            } catch (const std::exception&) {
                std::cerr << "Input error: --max-input-channels must be a positive integer\n";
                return 3;
            }
        } else {
            std::cerr << "Input error: unknown or incomplete option: "
                      << argument << "\nRun telemetry_guard --help for usage.\n";
            return 3;
        }
    }
    if (!policyPath.empty()) {
        try {
            const TelemetryPolicy policy = readPolicyFile(policyPath);
            if (!minimumHealthScore.has_value())
                minimumHealthScore = policy.minimumHealthScore;
            if (!minimumAvailabilityPercent.has_value())
                minimumAvailabilityPercent = policy.minimumAvailabilityPercent;
            if (!minimumChannelCount.has_value())
                minimumChannelCount = policy.minimumChannelCount;
            if (!maximumRegressions.has_value())
                maximumRegressions = policy.maximumRegressions;
            if (!failOnProvided && policy.failOn.has_value())
                failOn = *policy.failOn;
            for (const auto& channel : policy.requiredChannels) {
                if (std::find(requiredChannels.begin(), requiredChannels.end(), channel) !=
                    requiredChannels.end())
                    throw std::runtime_error("duplicate required channel: " + channel);
                requiredChannels.push_back(channel);
            }
        } catch (const std::exception& error) {
            std::cerr << "Input error: " << error.what() << '\n';
            return 3;
        }
    }
    const int outputModes = static_cast<int>(jsonOutput) +
                            static_cast<int>(ndjsonOutput) +
                            static_cast<int>(prometheusOutput) +
                            static_cast<int>(eventsOutput) +
                            static_cast<int>(junitOutput) +
                            static_cast<int>(githubOutput) +
                            static_cast<int>(htmlOutput) +
                            static_cast<int>(sarifOutput) +
                            static_cast<int>(csvReportOutput);
    if (outputModes > 1) {
        std::cerr << "Input error: output modes are mutually exclusive\n";
        return 3;
    }
    if (failOn != "monitor" && failOn != "hold" && failOn != "never") {
        std::cerr << "Input error: --fail-on must be monitor, hold, or never\n";
        return 3;
    }
    if (csvPath == "-" && baselinePath == "-") {
        std::cerr << "Input error: current and baseline CSV cannot both use standard input\n";
        return 3;
    }
    if (eventsOutput && baselinePath.empty()) {
        std::cerr << "Input error: --events requires --baseline\n";
        return 3;
    }
    if (marginDropPercent.has_value() && !eventsOutput) {
        std::cerr << "Input error: --margin-drop-percent requires --events\n";
        return 3;
    }
    if (maximumRegressions.has_value() && baselinePath.empty()) {
        std::cerr << "Input error: --max-regressions requires --baseline\n";
        return 3;
    }
    if (!requiredChannelsPath.empty()) {
        try {
            const auto fileChannels = readRequiredChannelsFile(requiredChannelsPath);
            for (const auto& channel : fileChannels) {
                if (std::find(requiredChannels.begin(), requiredChannels.end(), channel) !=
                    requiredChannels.end())
                    throw std::runtime_error("duplicate required channel: " + channel);
                requiredChannels.push_back(channel);
            }
        } catch (const std::exception& error) {
            std::cerr << "Input error: " << error.what() << '\n';
            return 3;
        }
    }
    const bool structuredOutput = jsonOutput || ndjsonOutput ||
                                  prometheusOutput || eventsOutput || junitOutput ||
                                  githubOutput || htmlOutput || sarifOutput ||
                                  csvReportOutput;
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
    std::map<std::string, TelemetryStatus> baselineStatuses;
    std::map<std::string, TelemetryReading> baselineReadings;
    try {
        readings = csvPath.empty() ? sample :
            readCsvFile(csvPath, maximumInputChannels);
        if (!baselinePath.empty()) {
            const std::vector<TelemetryReading> baseline =
                readCsvFile(baselinePath, maximumInputChannels);
            for (const auto& reading : baseline)
                baselineReadings.emplace(reading.channel, reading);
            for (const auto& reading : baseline)
                baselineStatuses.emplace(reading.channel, evaluateReading(reading));
            if (baselineStatuses.size() != readings.size())
                throw std::runtime_error("baseline channels do not match current channels");
            for (const auto& reading : readings) {
                if (baselineStatuses.find(reading.channel) == baselineStatuses.end())
                    throw std::runtime_error("baseline channels do not match current channels");
                if (!hasMatchingConfiguration(
                        reading, baselineReadings.at(reading.channel)))
                    throw std::runtime_error(
                        "baseline configuration differs for channel: " +
                        reading.channel);
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Input error: " << error.what() << '\n';
        return 3;
    }

    std::ofstream outputFile;
    std::streambuf* standardOutput = nullptr;
    if (!outputPath.empty()) {
        outputFile.open(outputPath, std::ios::out | std::ios::trunc);
        if (!outputFile) {
            std::cerr << "Output error: cannot open file: " << outputPath << '\n';
            return 3;
        }
        standardOutput = std::cout.rdbuf(outputFile.rdbuf());
    }
    if (!structuredOutput)
        std::cout << "TelemetryGuard - Vehicle Health Check\n\n";

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
    int regressionCount = 0;
    int recoveryCount = 0;
    int unchangedCount = 0;
    int marginChannelCount = 0;
    std::optional<double> minimumWarningHeadroom;
    std::string priorityChannel = "None";
    TelemetryStatus priorityStatus = TelemetryStatus::Nominal;
    std::vector<TelemetryStatus> statuses;
    std::vector<std::optional<LimitMargin>> margins;

    for (const auto& reading : readings) {
        const TelemetryStatus status = evaluateReading(reading);
        statuses.push_back(status);
        const std::optional<LimitMargin> margin = readingLimitMargin(reading);
        margins.push_back(margin);
        if (margin.has_value()) {
            ++marginChannelCount;
            if (!minimumWarningHeadroom.has_value() ||
                margin->warningHeadroomPercent < *minimumWarningHeadroom)
                minimumWarningHeadroom = margin->warningHeadroomPercent;
        }
        if (!baselineStatuses.empty()) {
            const TelemetryStatus previousStatus = baselineStatuses.at(reading.channel);
            if (statusPriority(status) > statusPriority(previousStatus)) ++regressionCount;
            else if (statusPriority(status) < statusPriority(previousStatus)) ++recoveryCount;
            else ++unchangedCount;
        }
        if (!structuredOutput) {
            std::cout << std::left << std::setw(18) << reading.channel << std::setw(10);
            if (status == TelemetryStatus::MissingData) std::cout << "N/A";
            else std::cout << reading.value;
            std::cout << std::setw(8) << reading.unit << std::setw(14) << statusLabel(status)
                      << "age=" << reading.ageSeconds << "s reason="
                      << diagnosticReason(reading, status);
            if (!baselineStatuses.empty())
                std::cout << " previous=" << statusLabel(baselineStatuses.at(reading.channel));
            if (margin.has_value())
                std::cout << " warning_margin=" << margin->nearestWarningMargin
                          << reading.unit;
            std::cout << '\n';
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
    const std::string channelDisposition = vehicleDisposition(
        warningCount, criticalCount, agingCount, staleCount, missingDataCount,
        invalidTimestampCount, configurationErrorCount);
    const bool healthScoreGateMet = !minimumHealthScore.has_value() ||
                                    healthScore >= *minimumHealthScore;
    const bool availabilityGateMet = !minimumAvailabilityPercent.has_value() ||
                                     availability >= *minimumAvailabilityPercent;
    const bool channelCountGateMet = !minimumChannelCount.has_value() ||
                                     totalReadings >= *minimumChannelCount;
    std::set<std::string> observedChannels;
    for (const auto& reading : readings) observedChannels.insert(reading.channel);
    std::vector<std::string> missingRequiredChannels;
    for (const auto& channel : requiredChannels) {
        if (observedChannels.find(channel) == observedChannels.end())
            missingRequiredChannels.push_back(channel);
    }
    const bool requiredChannelsGateMet = missingRequiredChannels.empty();
    const bool regressionGateMet = !maximumRegressions.has_value() ||
                                   regressionCount <= *maximumRegressions;
    const std::string disposition = healthScoreGateMet && availabilityGateMet &&
                                    channelCountGateMet && requiredChannelsGateMet &&
                                    regressionGateMet
        ? channelDisposition
        : "HOLD";

    if (csvReportOutput) {
        std::cout << "record_type,channel,value,unit,age_seconds,status,reason,"
                     "previous_status,nearest_warning_margin,"
                     "warning_headroom_percent,health_score,availability_percent,"
                     "blocking_issues,disposition\n";
        std::cout << std::setprecision(15);
        for (std::size_t index = 0; index < readings.size(); ++index) {
            const auto& reading = readings[index];
            std::cout << "channel," << csvField(reading.channel) << ',';
            if (std::isfinite(reading.value)) std::cout << reading.value;
            std::cout << ',' << csvField(reading.unit) << ',';
            if (std::isfinite(reading.ageSeconds)) std::cout << reading.ageSeconds;
            std::cout << ',' << csvField(statusLabel(statuses[index]))
                      << ',' << csvField(diagnosticReason(reading, statuses[index]))
                      << ',';
            if (!baselineStatuses.empty())
                std::cout << csvField(
                    statusLabel(baselineStatuses.at(reading.channel)));
            std::cout << ',';
            if (margins[index].has_value())
                std::cout << margins[index]->nearestWarningMargin;
            std::cout << ',';
            if (margins[index].has_value())
                std::cout << margins[index]->warningHeadroomPercent;
            std::cout << ",,,,\n";
        }
        std::cout << "summary,,,,,,,,,," << healthScore << ','
                  << availability << ',' << blockingIssueCount << ','
                  << csvField(disposition) << '\n';
    } else if (sarifOutput) {
        std::cout << "{\"$schema\":\"https://json.schemastore.org/sarif-2.1.0.json\","
                     "\"version\":\"2.1.0\",\"runs\":[{\"tool\":{\"driver\":{"
                     "\"name\":\"TelemetryGuard\",\"version\":\"0.1.0\","
                     "\"informationUri\":\"https://github.com/tabithaz/TelemetryGuard\","
                     "\"rules\":["
                     "{\"id\":\"TG001\",\"name\":\"WarningLimit\",\"shortDescription\":{\"text\":\"Telemetry value crossed a warning limit\"}},"
                     "{\"id\":\"TG002\",\"name\":\"CriticalLimit\",\"shortDescription\":{\"text\":\"Telemetry value crossed a critical limit\"}},"
                     "{\"id\":\"TG003\",\"name\":\"AgingTelemetry\",\"shortDescription\":{\"text\":\"Telemetry is approaching its freshness limit\"}},"
                     "{\"id\":\"TG004\",\"name\":\"StaleTelemetry\",\"shortDescription\":{\"text\":\"Telemetry exceeded its freshness limit\"}},"
                     "{\"id\":\"TG005\",\"name\":\"MissingData\",\"shortDescription\":{\"text\":\"Telemetry data is missing\"}},"
                     "{\"id\":\"TG006\",\"name\":\"InvalidTimestamp\",\"shortDescription\":{\"text\":\"Telemetry timestamp is invalid\"}},"
                     "{\"id\":\"TG007\",\"name\":\"InvalidConfiguration\",\"shortDescription\":{\"text\":\"Telemetry limits or freshness configuration is invalid\"}}]}},";
        std::cout << "\"automationDetails\":{\"id\":\"TelemetryGuard/"
                  << jsonEscape(disposition) << "\"},\"invocations\":[{"
                  << "\"executionSuccessful\":true,\"exitCode\":"
                  << policyExitCode(disposition, failOn) << "}],\"results\":[";
        bool wroteResult = false;
        for (std::size_t index = 0; index < readings.size(); ++index) {
            if (statuses[index] == TelemetryStatus::Nominal) continue;
            if (wroteResult) std::cout << ',';
            wroteResult = true;
            const auto& reading = readings[index];
            std::cout << "{\"ruleId\":\"" << sarifRuleId(statuses[index])
                      << "\",\"level\":\"" << sarifLevel(statuses[index])
                      << "\",\"message\":{\"text\":\""
                      << jsonEscape(reading.channel + ": " + statusLabel(statuses[index]))
                      << "\"},\"properties\":{\"channel\":\""
                      << jsonEscape(reading.channel) << "\",\"status\":\""
                      << statusLabel(statuses[index]) << "\",\"value\":";
            if (std::isfinite(reading.value)) std::cout << reading.value;
            else std::cout << "null";
            std::cout << ",\"unit\":\"" << jsonEscape(reading.unit)
                      << "\",\"ageSeconds\":";
            if (std::isfinite(reading.ageSeconds)) std::cout << reading.ageSeconds;
            else std::cout << "null";
            std::cout << ",\"reason\":\""
                      << diagnosticReason(reading, statuses[index]) << "\"}}";
        }
        std::cout << "]}]}\n";
    } else if (htmlOutput) {
        const std::string dispositionClass = disposition == "GO" ? "nominal" :
            (disposition == "MONITOR" ? "warning" : "critical");
        std::cout << "<!doctype html>\n<html lang=\"en\">\n<head>\n"
                  << "  <meta charset=\"utf-8\">\n"
                  << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                  << "  <title>TelemetryGuard Health Report</title>\n"
                  << "  <style>\n"
                  << "    :root{color-scheme:dark;--bg:#07111f;--panel:#101d2e;--line:#26364b;"
                     "--text:#eef5ff;--muted:#9fb0c5;--good:#4ade80;--warn:#fbbf24;--bad:#fb7185}\n"
                  << "    *{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at top right,#17365b 0,#07111f 42%);"
                     "color:var(--text);font-family:Inter,ui-sans-serif,system-ui,sans-serif;min-height:100vh}\n"
                  << "    main{width:min(1080px,calc(100% - 32px));margin:0 auto;padding:52px 0 64px}"
                     "header{display:flex;justify-content:space-between;gap:24px;align-items:end;margin-bottom:30px}\n"
                  << "    h1{font-size:clamp(2rem,5vw,3.5rem);letter-spacing:-.04em;margin:0}"
                     ".eyebrow{color:#7dd3fc;font-size:.75rem;font-weight:800;letter-spacing:.18em;text-transform:uppercase;margin:0 0 10px}"
                     ".subtitle{color:var(--muted);margin:8px 0 0}\n"
                  << "    .hero-status{font-weight:850;font-size:1.05rem;border:1px solid currentColor;border-radius:999px;padding:10px 16px}"
                     ".summary{display:grid;grid-template-columns:repeat(4,1fr);gap:14px;margin-bottom:24px}\n"
                  << "    .card,.table-wrap{background:rgba(16,29,46,.88);border:1px solid var(--line);box-shadow:0 20px 55px rgba(0,0,0,.2);"
                     "border-radius:18px}.card{padding:20px}.label{color:var(--muted);font-size:.75rem;font-weight:700;letter-spacing:.08em;text-transform:uppercase}"
                     ".value{font-size:1.7rem;font-weight:800;margin-top:8px}\n"
                  << "    .table-wrap{overflow:hidden}table{border-collapse:collapse;width:100%}th,td{padding:16px 18px;text-align:left;border-bottom:1px solid var(--line)}"
                     "th{color:var(--muted);font-size:.72rem;letter-spacing:.1em;text-transform:uppercase}tbody tr:last-child td{border-bottom:0}"
                     "tbody tr:hover{background:rgba(125,211,252,.04)}\n"
                  << "    .status{display:inline-block;border-radius:999px;padding:5px 9px;font-size:.72rem;font-weight:850;letter-spacing:.06em}"
                     ".nominal{color:var(--good)}.warning{color:var(--warn)}.critical{color:var(--bad)}"
                     ".status.nominal{background:rgba(74,222,128,.12)}.status.warning{background:rgba(251,191,36,.12)}.status.critical{background:rgba(251,113,133,.12)}\n"
                  << "    footer{color:var(--muted);font-size:.8rem;margin-top:18px;text-align:right}"
                     "@media(max-width:760px){header{align-items:start;flex-direction:column}.summary{grid-template-columns:repeat(2,1fr)}"
                     ".table-wrap{overflow-x:auto}th,td{white-space:nowrap}}\n"
                  << "  </style>\n</head>\n<body>\n<main>\n"
                  << "  <header><div><p class=\"eyebrow\">Telemetry health</p>"
                     "<h1>Mission Readiness</h1><p class=\"subtitle\">TelemetryGuard channel assessment</p></div>"
                  << "<div class=\"hero-status " << dispositionClass << "\">"
                  << xmlEscape(disposition) << "</div></header>\n"
                  << "  <section class=\"summary\" aria-label=\"Health summary\">\n"
                  << "    <article class=\"card\"><div class=\"label\">Health score</div><div class=\"value\">"
                  << healthScore << "/100</div></article>\n"
                  << "    <article class=\"card\"><div class=\"label\">Availability</div><div class=\"value\">"
                  << std::fixed << std::setprecision(1) << availability << "%</div></article>\n"
                  << "    <article class=\"card\"><div class=\"label\">Blocking issues</div><div class=\"value\">"
                  << blockingIssueCount << "</div></article>\n"
                  << "    <article class=\"card\"><div class=\"label\">Priority channel</div><div class=\"value\">"
                  << xmlEscape(priorityChannel) << "</div></article>\n"
                  << "  </section>\n  <section class=\"table-wrap\">\n"
                  << "    <table><thead><tr><th>Channel</th><th>Value</th><th>Unit</th><th>Age</th><th>Status</th><th>Reason</th></tr></thead><tbody>\n";
        for (std::size_t index = 0; index < readings.size(); ++index) {
            const auto& reading = readings[index];
            std::cout << "      <tr><td>" << xmlEscape(reading.channel) << "</td><td>";
            if (std::isfinite(reading.value)) std::cout << reading.value;
            else std::cout << "N/A";
            std::cout << "</td><td>" << xmlEscape(reading.unit) << "</td><td>";
            if (std::isfinite(reading.ageSeconds)) std::cout << reading.ageSeconds << " s";
            else std::cout << "N/A";
            std::cout << "</td><td><span class=\"status "
                      << htmlStatusClass(statuses[index]) << "\">"
                      << xmlEscape(statusLabel(statuses[index])) << "</span></td><td>"
                      << xmlEscape(diagnosticReason(reading, statuses[index]))
                      << "</td></tr>\n";
        }
        std::cout << "    </tbody></table>\n  </section>\n"
                  << "  <footer>" << totalReadings << " channels evaluated · Priority status: "
                  << xmlEscape(statusLabel(priorityStatus)) << "</footer>\n"
                  << "</main>\n</body>\n</html>\n";
    } else if (githubOutput) {
        for (std::size_t index = 0; index < readings.size(); ++index) {
            if (statuses[index] == TelemetryStatus::Nominal) continue;
            const auto& reading = readings[index];
            const char* level = requiresHold(statuses[index]) ? "error" : "warning";
            const std::string title = "TelemetryGuard: " + reading.channel;
            std::cout << "::" << level << " title="
                      << githubCommandEscape(title, true) << "::Channel status "
                      << statusLabel(statuses[index]) << "; value=";
            if (std::isfinite(reading.value)) std::cout << reading.value;
            else std::cout << "null";
            std::cout << "; unit=" << githubCommandEscape(reading.unit, false)
                      << "; age_seconds=" << reading.ageSeconds
                      << "; reason=" << diagnosticReason(reading, statuses[index]) << '\n';
        }
        std::ostringstream summary;
        summary << "Disposition " << disposition << ", health score "
                << healthScore << "/100, blocking issues " << blockingIssueCount;
        if (maximumRegressions.has_value())
            summary << ", regressions " << regressionCount << "/"
                    << *maximumRegressions;
        if (minimumAvailabilityPercent.has_value())
            summary << ", availability " << std::fixed << std::setprecision(1)
                    << availability << "%/" << *minimumAvailabilityPercent << "%";
        if (minimumChannelCount.has_value())
            summary << ", channels " << totalReadings << "/"
                    << *minimumChannelCount;
        if (!requiredChannels.empty())
            summary << ", required channels "
                    << (requiredChannelsGateMet ? "present" : "missing");
        std::cout << "::notice title=TelemetryGuard summary::"
                  << githubCommandEscape(summary.str(), false) << '\n';
    } else if (junitOutput) {
        const int failureCount = totalReadings - nominalCount;
        std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                  << "<testsuite name=\"TelemetryGuard\" tests=\"" << totalReadings
                  << "\" failures=\"" << failureCount
                  << "\" errors=\"0\" skipped=\"0\">\n"
                  << "  <properties>\n"
                  << "    <property name=\"health_score\" value=\"" << healthScore
                  << "\"/>\n"
                  << "    <property name=\"health_band\" value=\""
                  << xmlEscape(healthBand(healthScore)) << "\"/>\n"
                  << "    <property name=\"disposition\" value=\""
                  << xmlEscape(disposition) << "\"/>\n"
                  << "    <property name=\"availability_percent\" value=\""
                  << std::fixed << std::setprecision(1) << availability << "\"/>\n"
                  << "    <property name=\"availability_gate_met\" value=\""
                  << (availabilityGateMet ? "true" : "false") << "\"/>\n"
                  << "    <property name=\"minimum_availability_percent\" value=\"";
        if (minimumAvailabilityPercent.has_value())
            std::cout << *minimumAvailabilityPercent;
        else std::cout << "not_configured";
        std::cout << "\"/>\n"
                  << "    <property name=\"channel_count\" value=\""
                  << totalReadings << "\"/>\n"
                  << "    <property name=\"minimum_channel_count\" value=\"";
        if (minimumChannelCount.has_value()) std::cout << *minimumChannelCount;
        else std::cout << "not_configured";
        std::cout << "\"/>\n"
                  << "    <property name=\"channel_count_gate_met\" value=\""
                  << (channelCountGateMet ? "true" : "false") << "\"/>\n"
                  << "    <property name=\"required_channels_gate_met\" value=\""
                  << (requiredChannelsGateMet ? "true" : "false") << "\"/>\n"
                  << "    <property name=\"missing_required_channel_count\" value=\""
                  << missingRequiredChannels.size() << "\"/>\n"
                  << "    <property name=\"regression_gate_met\" value=\""
                  << (regressionGateMet ? "true" : "false") << "\"/>\n"
                  << "  </properties>\n";
        for (std::size_t index = 0; index < readings.size(); ++index) {
            const auto& reading = readings[index];
            const std::string status = statusLabel(statuses[index]);
            std::cout << "  <testcase classname=\"TelemetryGuard.Channel\" name=\""
                      << xmlEscape(reading.channel) << "\" time=\"0\">\n";
            if (statuses[index] != TelemetryStatus::Nominal) {
                std::cout << "    <failure type=\"" << xmlEscape(status)
                          << "\" message=\"Channel status: " << xmlEscape(status)
                          << "\">value=";
                if (std::isfinite(reading.value)) std::cout << reading.value;
                else std::cout << "null";
                std::cout << " unit=" << xmlEscape(reading.unit)
                          << " age_seconds=" << reading.ageSeconds
                          << " reason=" << diagnosticReason(reading, statuses[index])
                          << "</failure>\n";
            }
            std::cout << "  </testcase>\n";
        }
        std::cout << "</testsuite>\n";
    } else if (eventsOutput) {
        int marginRegressionCount = 0;
        for (std::size_t index = 0; index < readings.size(); ++index) {
            const auto& reading = readings[index];
            const TelemetryStatus previous = baselineStatuses.at(reading.channel);
            const int currentPriority = statusPriority(statuses[index]);
            const int previousPriority = statusPriority(previous);
            if (currentPriority != previousPriority) {
                std::cout << "{\"type\":\"status_transition\",\"transition\":\""
                          << (currentPriority > previousPriority ? "regression" : "recovery")
                          << "\",\"channel\":\"" << jsonEscape(reading.channel)
                          << "\",\"previous_status\":\"" << statusLabel(previous)
                          << "\",\"current_status\":\"" << statusLabel(statuses[index])
                          << "\",\"previous_reason\":\""
                          << diagnosticReason(baselineReadings.at(reading.channel), previous)
                          << "\",\"current_reason\":\""
                          << diagnosticReason(reading, statuses[index])
                          << "\",\"value\":";
                if (std::isfinite(reading.value)) std::cout << reading.value;
                else std::cout << "null";
                std::cout << ",\"unit\":\"" << jsonEscape(reading.unit) << "\"}\n";
                continue;
            }
            if (!marginDropPercent.has_value() || !margins[index].has_value()) continue;
            const auto previousMargin = readingLimitMargin(
                baselineReadings.at(reading.channel));
            if (!previousMargin.has_value()) continue;
            const double drop = previousMargin->warningHeadroomPercent -
                                margins[index]->warningHeadroomPercent;
            if (drop < *marginDropPercent) continue;
            ++marginRegressionCount;
            std::cout << "{\"type\":\"margin_regression\",\"channel\":\""
                      << jsonEscape(reading.channel) << "\",\"status\":\""
                      << statusLabel(statuses[index]) << "\",\"reason\":\""
                      << diagnosticReason(reading, statuses[index])
                      << "\",\"previous_warning_headroom_percent\":"
                      << previousMargin->warningHeadroomPercent
                      << ",\"current_warning_headroom_percent\":"
                      << margins[index]->warningHeadroomPercent
                      << ",\"drop_percent\":" << drop << ",\"value\":";
            if (std::isfinite(reading.value)) std::cout << reading.value;
            else std::cout << "null";
            std::cout << ",\"unit\":\"" << jsonEscape(reading.unit) << "\"}\n";
        }
        std::cout << "{\"type\":\"transition_summary\",\"regressions\":"
                  << regressionCount << ",\"recoveries\":" << recoveryCount
                  << ",\"unchanged\":" << unchangedCount
                  << ",\"margin_regressions\":" << marginRegressionCount
                  << ",\"margin_drop_threshold_percent\":";
        if (marginDropPercent.has_value()) std::cout << *marginDropPercent;
        else std::cout << "null";
        std::cout
                  << ",\"minimum_health_score\":";
        if (minimumHealthScore.has_value()) std::cout << *minimumHealthScore;
        else std::cout << "null";
        std::cout << ",\"health_score_gate_met\":"
                  << (healthScoreGateMet ? "true" : "false")
                  << ",\"minimum_availability_percent\":";
        if (minimumAvailabilityPercent.has_value())
            std::cout << *minimumAvailabilityPercent;
        else std::cout << "null";
        std::cout << ",\"availability_gate_met\":"
                  << (availabilityGateMet ? "true" : "false")
                  << ",\"minimum_channel_count\":";
        if (minimumChannelCount.has_value()) std::cout << *minimumChannelCount;
        else std::cout << "null";
        std::cout << ",\"channel_count_gate_met\":"
                  << (channelCountGateMet ? "true" : "false")
                  << ",\"required_channels_gate_met\":"
                  << (requiredChannelsGateMet ? "true" : "false")
                  << ",\"missing_required_channels\":[";
        for (std::size_t index = 0; index < missingRequiredChannels.size(); ++index) {
            if (index > 0) std::cout << ',';
            std::cout << '"' << jsonEscape(missingRequiredChannels[index]) << '"';
        }
        std::cout << "]"
                  << ",\"maximum_regressions\":";
        if (maximumRegressions.has_value()) std::cout << *maximumRegressions;
        else std::cout << "null";
        std::cout << ",\"regression_gate_met\":"
                  << (regressionGateMet ? "true" : "false")
                  << ",\"disposition\":\"" << disposition << "\"}\n";
    } else if (jsonOutput) {
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
            std::cout << ",\"status\":\"" << statusLabel(statuses[index])
                      << "\",\"reason\":\""
                      << diagnosticReason(reading, statuses[index]) << "\"";
            if (!baselineStatuses.empty())
                std::cout << ",\"previous_status\":\""
                          << statusLabel(baselineStatuses.at(reading.channel)) << "\"";
            std::cout << ",\"nearest_warning_margin\":";
            if (margins[index].has_value())
                std::cout << margins[index]->nearestWarningMargin;
            else std::cout << "null";
            std::cout << ",\"nearest_critical_margin\":";
            if (margins[index].has_value())
                std::cout << margins[index]->nearestCriticalMargin;
            else std::cout << "null";
            std::cout << ",\"warning_headroom_percent\":";
            if (margins[index].has_value())
                std::cout << margins[index]->warningHeadroomPercent;
            else std::cout << "null";
            std::cout << '}';
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
                  << "\",\"minimum_health_score\":";
        if (minimumHealthScore.has_value()) std::cout << *minimumHealthScore;
        else std::cout << "null";
        std::cout << ",\"health_score_gate_met\":"
                  << (healthScoreGateMet ? "true" : "false")
                  << ",\"minimum_availability_percent\":";
        if (minimumAvailabilityPercent.has_value())
            std::cout << *minimumAvailabilityPercent;
        else std::cout << "null";
        std::cout << ",\"availability_gate_met\":"
                  << (availabilityGateMet ? "true" : "false")
                  << ",\"minimum_channel_count\":";
        if (minimumChannelCount.has_value()) std::cout << *minimumChannelCount;
        else std::cout << "null";
        std::cout << ",\"channel_count_gate_met\":"
                  << (channelCountGateMet ? "true" : "false")
                  << ",\"required_channels_gate_met\":"
                  << (requiredChannelsGateMet ? "true" : "false")
                  << ",\"missing_required_channels\":[";
        for (std::size_t index = 0; index < missingRequiredChannels.size(); ++index) {
            if (index > 0) std::cout << ',';
            std::cout << '"' << jsonEscape(missingRequiredChannels[index]) << '"';
        }
        std::cout << "]"
                  << ",\"maximum_regressions\":";
        if (maximumRegressions.has_value()) std::cout << *maximumRegressions;
        else std::cout << "null";
        std::cout << ",\"regression_gate_met\":"
                  << (regressionGateMet ? "true" : "false")
                  << ",\"comparison_enabled\":"
                  << (baselineStatuses.empty() ? "false" : "true")
                  << ",\"regressions\":" << regressionCount
                  << ",\"recoveries\":" << recoveryCount
                  << ",\"unchanged\":" << unchangedCount
                  << ",\"margin_channels\":" << marginChannelCount
                  << ",\"minimum_warning_headroom_percent\":";
        if (minimumWarningHeadroom.has_value()) std::cout << *minimumWarningHeadroom;
        else std::cout << "null";
        std::cout << ",\"disposition\":\"" << disposition << "\"}}\n";
    } else if (ndjsonOutput) {
        for (std::size_t index = 0; index < readings.size(); ++index) {
            const auto& reading = readings[index];
            std::cout << "{\"type\":\"channel\",\"channel\":\""
                      << jsonEscape(reading.channel) << "\",\"value\":";
            if (std::isfinite(reading.value)) std::cout << reading.value;
            else std::cout << "null";
            std::cout << ",\"unit\":\"" << jsonEscape(reading.unit)
                      << "\",\"age_seconds\":";
            if (std::isfinite(reading.ageSeconds)) std::cout << reading.ageSeconds;
            else std::cout << "null";
            std::cout << ",\"status\":\"" << statusLabel(statuses[index])
                      << "\",\"reason\":\""
                      << diagnosticReason(reading, statuses[index]) << "\"";
            if (!baselineStatuses.empty())
                std::cout << ",\"previous_status\":\""
                          << statusLabel(baselineStatuses.at(reading.channel)) << "\"";
            std::cout << ",\"nearest_warning_margin\":";
            if (margins[index].has_value())
                std::cout << margins[index]->nearestWarningMargin;
            else std::cout << "null";
            std::cout << ",\"nearest_critical_margin\":";
            if (margins[index].has_value())
                std::cout << margins[index]->nearestCriticalMargin;
            else std::cout << "null";
            std::cout << ",\"warning_headroom_percent\":";
            if (margins[index].has_value())
                std::cout << margins[index]->warningHeadroomPercent;
            else std::cout << "null";
            std::cout << "}\n";
        }
        std::cout << "{\"type\":\"summary\",\"total_readings\":" << totalReadings
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
                  << "\",\"minimum_health_score\":";
        if (minimumHealthScore.has_value()) std::cout << *minimumHealthScore;
        else std::cout << "null";
        std::cout << ",\"health_score_gate_met\":"
                  << (healthScoreGateMet ? "true" : "false")
                  << ",\"minimum_availability_percent\":";
        if (minimumAvailabilityPercent.has_value())
            std::cout << *minimumAvailabilityPercent;
        else std::cout << "null";
        std::cout << ",\"availability_gate_met\":"
                  << (availabilityGateMet ? "true" : "false")
                  << ",\"minimum_channel_count\":";
        if (minimumChannelCount.has_value()) std::cout << *minimumChannelCount;
        else std::cout << "null";
        std::cout << ",\"channel_count_gate_met\":"
                  << (channelCountGateMet ? "true" : "false")
                  << ",\"required_channels_gate_met\":"
                  << (requiredChannelsGateMet ? "true" : "false")
                  << ",\"missing_required_channels\":[";
        for (std::size_t index = 0; index < missingRequiredChannels.size(); ++index) {
            if (index > 0) std::cout << ',';
            std::cout << '"' << jsonEscape(missingRequiredChannels[index]) << '"';
        }
        std::cout << "]"
                  << ",\"maximum_regressions\":";
        if (maximumRegressions.has_value()) std::cout << *maximumRegressions;
        else std::cout << "null";
        std::cout << ",\"regression_gate_met\":"
                  << (regressionGateMet ? "true" : "false")
                  << ",\"comparison_enabled\":"
                  << (baselineStatuses.empty() ? "false" : "true")
                  << ",\"regressions\":" << regressionCount
                  << ",\"recoveries\":" << recoveryCount
                  << ",\"unchanged\":" << unchangedCount
                  << ",\"margin_channels\":" << marginChannelCount
                  << ",\"minimum_warning_headroom_percent\":";
        if (minimumWarningHeadroom.has_value()) std::cout << *minimumWarningHeadroom;
        else std::cout << "null";
        std::cout << ",\"disposition\":\"" << disposition << "\"}\n";
    } else if (prometheusOutput) {
        std::cout << "# HELP telemetry_guard_health_score Composite telemetry health score.\n"
                  << "# TYPE telemetry_guard_health_score gauge\n"
                  << "telemetry_guard_health_score " << healthScore << '\n'
                  << "# HELP telemetry_guard_availability_percent Percentage of available channels.\n"
                  << "# TYPE telemetry_guard_availability_percent gauge\n"
                  << "telemetry_guard_availability_percent " << std::fixed
                  << std::setprecision(1) << availability << '\n'
                  << "# HELP telemetry_guard_degradation_percent Percentage of degraded channels.\n"
                  << "# TYPE telemetry_guard_degradation_percent gauge\n"
                  << "telemetry_guard_degradation_percent " << degradation << '\n'
                  << "# HELP telemetry_guard_channels Number of channels by status.\n"
                  << "# TYPE telemetry_guard_channels gauge\n";
        const std::vector<std::pair<std::string, int>> counts = {
            {"nominal", nominalCount}, {"warning", warningCount},
            {"critical", criticalCount}, {"aging", agingCount},
            {"stale", staleCount}, {"missing", missingDataCount},
            {"invalid_timestamp", invalidTimestampCount},
            {"configuration_error", configurationErrorCount}
        };
        for (const auto& count : counts)
            std::cout << "telemetry_guard_channels{status=\"" << count.first
                      << "\"} " << count.second << '\n';
        std::cout << "# HELP telemetry_guard_disposition Current disposition as a one-hot gauge.\n"
                  << "# TYPE telemetry_guard_disposition gauge\n";
        for (const char* label : {"GO", "MONITOR", "HOLD"})
            std::cout << "telemetry_guard_disposition{disposition=\"" << label
                      << "\"} " << (disposition == label ? 1 : 0) << '\n';
        std::cout << "# HELP telemetry_guard_blocking_issues Number of HOLD-triggering channels.\n"
                  << "# TYPE telemetry_guard_blocking_issues gauge\n"
                  << "telemetry_guard_blocking_issues " << blockingIssueCount << '\n';
        std::cout << "# HELP telemetry_guard_margin_channels Channels with valid limit margins.\n"
                  << "# TYPE telemetry_guard_margin_channels gauge\n"
                  << "telemetry_guard_margin_channels " << marginChannelCount << '\n';
        if (minimumWarningHeadroom.has_value())
            std::cout << "# HELP telemetry_guard_min_warning_headroom_percent Minimum warning-limit headroom.\n"
                      << "# TYPE telemetry_guard_min_warning_headroom_percent gauge\n"
                      << "telemetry_guard_min_warning_headroom_percent "
                      << *minimumWarningHeadroom << '\n';
        if (minimumHealthScore.has_value())
            std::cout << "# HELP telemetry_guard_minimum_health_score Configured health score gate.\n"
                      << "# TYPE telemetry_guard_minimum_health_score gauge\n"
                      << "telemetry_guard_minimum_health_score "
                      << *minimumHealthScore << '\n'
                      << "# HELP telemetry_guard_health_score_gate_met Whether the health score gate passed.\n"
                      << "# TYPE telemetry_guard_health_score_gate_met gauge\n"
                      << "telemetry_guard_health_score_gate_met "
                      << (healthScoreGateMet ? 1 : 0) << '\n';
        if (minimumAvailabilityPercent.has_value())
            std::cout << "# HELP telemetry_guard_minimum_availability_percent Configured availability SLO.\n"
                      << "# TYPE telemetry_guard_minimum_availability_percent gauge\n"
                      << "telemetry_guard_minimum_availability_percent "
                      << *minimumAvailabilityPercent << '\n'
                      << "# HELP telemetry_guard_availability_gate_met Whether the availability SLO passed.\n"
                      << "# TYPE telemetry_guard_availability_gate_met gauge\n"
                      << "telemetry_guard_availability_gate_met "
                      << (availabilityGateMet ? 1 : 0) << '\n';
        if (minimumChannelCount.has_value())
            std::cout << "# HELP telemetry_guard_minimum_channel_count Configured telemetry completeness floor.\n"
                      << "# TYPE telemetry_guard_minimum_channel_count gauge\n"
                      << "telemetry_guard_minimum_channel_count "
                      << *minimumChannelCount << '\n'
                      << "# HELP telemetry_guard_channel_count_gate_met Whether the completeness gate passed.\n"
                      << "# TYPE telemetry_guard_channel_count_gate_met gauge\n"
                      << "telemetry_guard_channel_count_gate_met "
                      << (channelCountGateMet ? 1 : 0) << '\n';
        if (!requiredChannels.empty()) {
            std::cout << "# HELP telemetry_guard_required_channels Configured required channel identities.\n"
                      << "# TYPE telemetry_guard_required_channels gauge\n"
                      << "telemetry_guard_required_channels " << requiredChannels.size() << '\n'
                      << "# HELP telemetry_guard_missing_required_channels Required channels absent from the snapshot.\n"
                      << "# TYPE telemetry_guard_missing_required_channels gauge\n"
                      << "telemetry_guard_missing_required_channels "
                      << missingRequiredChannels.size() << '\n'
                      << "# HELP telemetry_guard_required_channels_gate_met Whether all required channels are present.\n"
                      << "# TYPE telemetry_guard_required_channels_gate_met gauge\n"
                      << "telemetry_guard_required_channels_gate_met "
                      << (requiredChannelsGateMet ? 1 : 0) << '\n';
        }
        if (!baselineStatuses.empty()) {
            std::cout << "# HELP telemetry_guard_transitions Number of channel status transitions.\n"
                      << "# TYPE telemetry_guard_transitions gauge\n"
                      << "telemetry_guard_transitions{transition=\"regression\"} "
                      << regressionCount << '\n'
                      << "telemetry_guard_transitions{transition=\"recovery\"} "
                      << recoveryCount << '\n'
                      << "telemetry_guard_transitions{transition=\"unchanged\"} "
                      << unchangedCount << '\n';
            if (maximumRegressions.has_value())
                std::cout << "# HELP telemetry_guard_maximum_regressions Configured regression budget.\n"
                          << "# TYPE telemetry_guard_maximum_regressions gauge\n"
                          << "telemetry_guard_maximum_regressions "
                          << *maximumRegressions << '\n'
                          << "# HELP telemetry_guard_regression_gate_met Whether the regression budget passed.\n"
                          << "# TYPE telemetry_guard_regression_gate_met gauge\n"
                          << "telemetry_guard_regression_gate_met "
                          << (regressionGateMet ? 1 : 0) << '\n';
        }
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

    if (!structuredOutput) {
        std::cout << "Channels with limit margins: " << marginChannelCount << '\n'
                  << "Minimum warning headroom: ";
        if (minimumWarningHeadroom.has_value())
            std::cout << *minimumWarningHeadroom << "%\n";
        else std::cout << "N/A\n";
        if (minimumHealthScore.has_value())
            std::cout << "Minimum health score: " << *minimumHealthScore << "\n"
                      << "Health score gate: "
                      << (healthScoreGateMet ? "PASS" : "FAIL") << '\n';
        if (minimumAvailabilityPercent.has_value())
            std::cout << "Minimum availability: " << *minimumAvailabilityPercent << "%\n"
                      << "Availability gate: "
                      << (availabilityGateMet ? "PASS" : "FAIL") << '\n';
        if (minimumChannelCount.has_value())
            std::cout << "Minimum channel count: " << *minimumChannelCount << "\n"
                      << "Channel count gate: "
                      << (channelCountGateMet ? "PASS" : "FAIL") << '\n';
        if (!requiredChannels.empty()) {
            std::cout << "Required channel gate: "
                      << (requiredChannelsGateMet ? "PASS" : "FAIL") << '\n';
            if (!missingRequiredChannels.empty()) {
                std::cout << "Missing required channels:";
                for (const auto& channel : missingRequiredChannels)
                    std::cout << ' ' << channel;
                std::cout << '\n';
            }
        }
        if (maximumRegressions.has_value())
            std::cout << "Maximum regressions: " << *maximumRegressions << "\n"
                      << "Regression gate: "
                      << (regressionGateMet ? "PASS" : "FAIL") << '\n';
    }

    if (!structuredOutput && !baselineStatuses.empty())
        std::cout << "Status regressions: " << regressionCount << '\n'
                  << "Status recoveries: " << recoveryCount << '\n'
                  << "Unchanged channels: " << unchangedCount << '\n';

    const int exitCode = policyExitCode(disposition, failOn);
    if (standardOutput != nullptr) {
        std::cout.flush();
        const bool writeFailed = !outputFile;
        std::cout.rdbuf(standardOutput);
        outputFile.close();
        if (writeFailed || !outputFile) {
            std::cerr << "Output error: cannot write file: " << outputPath << '\n';
            return 3;
        }
    }
    return exitCode;
}
