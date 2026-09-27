#include <iostream>
#include <string>
#include <cctype>

// Returns true if a valid address was found, false otherwise.
// On success: outAddress holds the 32-bit value,
// and outPort holds the port number, or -1 if no port was present.
// On failure: outAddress is set to 0 and outPort is set to -1.
bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort);

// ---- helpers -------------------------------------------------------------

// A "token character" is one that could legally be part of an address/port:
// digits, '.', or ':'. Everything else is garbage and just gets skipped.
static bool isTokenChar(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == ':';
}

// Parses one numeric field (an octet or a port) starting at str[pos],
// stopping at 'end' or the first non-digit. maxDigits bounds how many
// digits are allowed to be part of the field (3 for an octet, 5 for a port);
// if a digit run is longer than that, the field is invalid (too many digits).
// On success, advances pos past the digits and returns true with *value set.
// On failure, pos is left unspecified and false is returned.
static bool parseField(const std::string& str, size_t& pos, size_t end,
                        int maxDigits, unsigned long maxValue,
                        unsigned long* value) {
    if (pos >= end || !std::isdigit(static_cast<unsigned char>(str[pos]))) {
        return false; // must start with a digit
    }

    size_t digitStart = pos;
    unsigned long val = 0;
    int digitCount = 0;

    while (pos < end && std::isdigit(static_cast<unsigned char>(str[pos])) &&
           digitCount < maxDigits) {
        val = val * 10 + static_cast<unsigned long>(str[pos] - '0');
        ++pos;
        ++digitCount;
    }

    // If there's still another digit right after, the field had too many
    // digits to begin with (e.g. a 4-digit octet) -- reject, don't truncate.
    if (pos < end && std::isdigit(static_cast<unsigned char>(str[pos]))) {
        return false;
    }

    // No leading zero unless the value is exactly "0" (a single digit).
    if (digitCount > 1 && str[digitStart] == '0') {
        return false;
    }

    if (val > maxValue) {
        return false;
    }

    *value = val;
    return true;
}

// Attempts to parse str[start..end) as a *complete* address[:port] token --
// every character in the range must be consumed, or the whole thing fails.
static bool tryParseToken(const std::string& str, size_t start, size_t end,
                           unsigned long& outAddress, int& outPort) {
    size_t pos = start;
    unsigned long octets[4];

    for (int i = 0; i < 4; ++i) {
        if (!parseField(str, pos, end, 3, 255, &octets[i])) {
            return false;
        }
        if (i < 3) {
            if (pos >= end || str[pos] != '.') {
                return false;
            }
            ++pos; // consume '.'
        }
    }

    int port = -1;

    if (pos < end) {
        // Whatever comes after the 4th octet must be a colon+port, in full.
        if (str[pos] != ':') {
            return false;
        }
        ++pos; // consume ':'

        unsigned long portVal = 0;
        if (!parseField(str, pos, end, 5, 65535, &portVal)) {
            return false;
        }
        port = static_cast<int>(portVal);
    }

    // The whole token range must be consumed -- no leftover garbage
    // (extra colon, trailing junk, etc.).
    if (pos != end) {
        return false;
    }

    outAddress = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    outPort = port;
    return true;
}

// ---- main extraction function --------------------------------------------

bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort) {
    size_t n = str.size();
    size_t i = 0;

    while (i < n) {
        if (!isTokenChar(str[i])) {
            ++i;
            continue;
        }

        // Found the start of a maximal run of token characters.
        size_t start = i;
        while (i < n && isTokenChar(str[i])) {
            ++i;
        }
        size_t end = i;

        // The address must match this entire run -- not a piece of it.
        if (tryParseToken(str, start, end, outAddress, outPort)) {
            return true;
        }
        // Otherwise this run is a dead end; keep scanning after it.
    }

    outAddress = 0;
    outPort = -1;
    return false;
}

// ---- driver ----------------------------------------------------------

int main() {
    std::string line;

    while (true) {
        std::cout << "Enter a string (or 'END' to quit): ";
        if (!std::getline(std::cin, line)) {
            break; // EOF on input
        }

        if (line == "END") {
            std::cout << "Program terminated." << std::endl;
            break;
        }

        unsigned long address = 0;
        int port = -1;

        if (extractIPv4(line, address, port)) {
            unsigned long a = (address >> 24) & 0xFF;
            unsigned long b = (address >> 16) & 0xFF;
            unsigned long c = (address >> 8) & 0xFF;
            unsigned long d = address & 0xFF;

            std::cout << "Extracted IPv4 address: "
                      << a << "." << b << "." << c << "." << d
                      << " (decimal value: " << address << ", port: ";

            if (port == -1) {
                std::cout << "none";
            } else {
                std::cout << port;
            }

            std::cout << ")" << std::endl;
        } else {
            std::cout << "Invalid input: no valid IPv4 address found" << std::endl;
        }
    }

    return 0;
}
