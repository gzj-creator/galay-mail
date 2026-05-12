#include "galay-mail/protocol/pop3_protocol.h"
#include <cstdlib>
#include <iostream>

using namespace galay::mail::protocol;

static void require(bool value, const char* message)
{
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main()
{
    Pop3Parser parser;
    {
        auto result = parser.parse("+OK ready\r\n", false);
        require(result.has_value(), "single-line +OK parses");
        require(result->second.status == Pop3Status::Ok, "+OK status");
        require(result->second.message == "ready", "+OK message");
    }
    {
        auto result = parser.parse("-ERR bad login\r\n", false);
        require(result.has_value(), "single-line -ERR parses");
        require(result->second.status == Pop3Status::Error, "-ERR status");
    }
    {
        auto result = parser.parse("+OK capabilities\r\nTOP\r\nUIDL\r\n.\r\n", true);
        require(result.has_value(), "multiline parses");
        require(result->second.lines.size() == 2, "multiline count");
        require(result->second.lines[0] == "TOP", "multiline first");
    }
    {
        auto result = parser.parse("+OK msg\r\n..hello\r\n.\r\n", true);
        require(result.has_value(), "dot unstuff parses");
        require(result->second.lines[0] == ".hello", "dot unstuff");
    }
    {
        auto result = parser.parse("+OK msg\r\nline\r\n", true);
        require(!result.has_value() && result.error() == Pop3ParseError::Incomplete, "incomplete multiline");
    }

    Pop3Encoder encoder;
    require(encoder.capa() == "CAPA\r\n", "CAPA encoding");
    require(encoder.stls() == "STLS\r\n", "STLS encoding");
    require(encoder.user("alice") == "USER alice\r\n", "USER encoding");
    require(encoder.pass("secret") == "PASS secret\r\n", "PASS encoding");
    require(encoder.authPlain("payload") == "AUTH PLAIN payload\r\n", "AUTH PLAIN encoding");
    require(encoder.stat() == "STAT\r\n", "STAT encoding");
    require(encoder.list(3) == "LIST 3\r\n", "LIST encoding");
    require(encoder.retr(4) == "RETR 4\r\n", "RETR encoding");
    require(encoder.dele(5) == "DELE 5\r\n", "DELE encoding");
    require(encoder.noop() == "NOOP\r\n", "NOOP encoding");
    require(encoder.rset() == "RSET\r\n", "RSET encoding");
    require(encoder.quit() == "QUIT\r\n", "QUIT encoding");
    std::cout << "pop3 protocol tests passed\n";
    return 0;
}
