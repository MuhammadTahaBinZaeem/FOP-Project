#include "pocket_engineer/engine.hpp"

#include <cctype>
#include <map>
#include <stdexcept>

namespace pocket_engineer {
namespace {
// Small strict reader for the flat, string-valued request protocol. It does not
// search for key substrings inside user input, execute escapes, or accept broken
// JSON. Bounded before parsing so JNI and WebAssembly have the same limits.
class RequestReader {
public:
    explicit RequestReader(std::string_view source) : source_(source) {
        if (source.size() > 32768) fail("Request exceeds 32768 bytes");
    }

    ProblemSpec read() {
        expect('{');
        std::map<std::string, std::string> fields;
        whitespace();
        if (!take('}')) {
            do {
                const auto key = string();
                if (key != "domain" && key != "topic" && key != "input" && key != "payload")
                    fail("Unknown request field");
                expect(':');
                if (!fields.emplace(key, string()).second) fail("Duplicate request field");
                whitespace();
                if (take('}')) break;
                expect(',');
            } while (true);
        }
        whitespace();
        if (cursor_ != source_.size()) fail("Unexpected data after request");
        if (!fields.contains("input")) fail("Request requires a string input field");
        return {fields.contains("domain") ? fields.at("domain") : "algebra",
                fields.contains("topic") ? fields.at("topic") : "simplify",
                fields.at("input"), fields.contains("payload") ? fields.at("payload") : ""};
    }

private:
    [[noreturn]] static void fail(const char* message) { throw std::runtime_error(message); }
    void whitespace() {
        while (cursor_ < source_.size() && (source_[cursor_] == ' ' || source_[cursor_] == '\n' ||
               source_[cursor_] == '\r' || source_[cursor_] == '\t')) ++cursor_;
    }
    bool take(char value) {
        if (cursor_ < source_.size() && source_[cursor_] == value) { ++cursor_; return true; }
        return false;
    }
    void expect(char value) {
        whitespace();
        if (!take(value)) fail("Malformed JSON request");
    }
    unsigned hex4() {
        unsigned value = 0;
        for (unsigned i = 0; i < 4; ++i) {
            if (cursor_ == source_.size()) fail("Incomplete Unicode escape");
            const char digit = source_[cursor_++];
            value <<= 4;
            if (digit >= '0' && digit <= '9') value += static_cast<unsigned>(digit - '0');
            else if (digit >= 'a' && digit <= 'f') value += static_cast<unsigned>(digit - 'a' + 10);
            else if (digit >= 'A' && digit <= 'F') value += static_cast<unsigned>(digit - 'A' + 10);
            else fail("Invalid Unicode escape");
        }
        return value;
    }
    static void utf8(std::string& out, unsigned code) {
        if (code == 0) fail("NUL is not allowed in request fields");
        if (code < 0x80) out.push_back(static_cast<char>(code));
        else if (code < 0x800) {
            out.push_back(static_cast<char>(0xc0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 63)));
        } else if (code < 0x10000) {
            out.push_back(static_cast<char>(0xe0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 63)));
            out.push_back(static_cast<char>(0x80 | (code & 63)));
        } else {
            out.push_back(static_cast<char>(0xf0 | (code >> 18)));
            out.push_back(static_cast<char>(0x80 | ((code >> 12) & 63)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 63)));
            out.push_back(static_cast<char>(0x80 | (code & 63)));
        }
    }
    std::string string() {
        expect('"');
        std::string out;
        while (cursor_ < source_.size()) {
            const auto value = static_cast<unsigned char>(source_[cursor_++]);
            if (value == '"') return out;
            if (value < 32) fail("Unescaped control character in JSON string");
            if (value != '\\') { out.push_back(static_cast<char>(value)); continue; }
            if (cursor_ == source_.size()) fail("Unfinished escape");
            switch (source_[cursor_++]) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned code = hex4();
                    if (code >= 0xd800 && code <= 0xdbff) {
                        if (!take('\\') || !take('u')) fail("Missing low surrogate");
                        const auto low = hex4();
                        if (low < 0xdc00 || low > 0xdfff) fail("Invalid low surrogate");
                        code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                    } else if (code >= 0xdc00 && code <= 0xdfff) fail("Unpaired low surrogate");
                    utf8(out, code);
                    break;
                }
                default: fail("Invalid JSON escape");
            }
        }
        fail("Unterminated JSON string");
    }
    std::string_view source_;
    std::size_t cursor_{};
};
} // namespace

ProblemSpec parse_request(std::string_view json) { return RequestReader(json).read(); }
} // namespace pocket_engineer
