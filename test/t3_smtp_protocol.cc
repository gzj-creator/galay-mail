#include "galay-mail/protocol/smtp_protocol.h"
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
    SmtpParser parser;
    {
        auto result = parser.parse("220 mail.example.com ready\r\n");
        require(result.has_value(), "single-line reply parses");
        require(result->second.code == 220, "single-line code");
        require(result->second.message == "mail.example.com ready", "single-line message");
        require(result->first == 28, "single-line consumed");
    }
    {
        auto result = parser.parse("250-mail.example.com\r\n250-STARTTLS\r\n250 AUTH PLAIN LOGIN\r\n");
        require(result.has_value(), "multiline reply parses");
        require(result->second.code == 250, "multiline code");
        require(result->second.lines.size() == 3, "multiline lines");
        require(result->second.lines[1] == "STARTTLS", "multiline middle line");
        require(result->second.message == "mail.example.com\nSTARTTLS\nAUTH PLAIN LOGIN", "multiline message");
    }
    {
        auto result = parser.parse("250-mail.example.com\r\n");
        require(!result.has_value() && result.error() == SmtpParseError::Incomplete, "incomplete multiline");
    }
    {
        auto result = parser.parse("xx bad\r\n");
        require(!result.has_value() && result.error() == SmtpParseError::InvalidFormat, "invalid reply");
    }

    SmtpEncoder encoder;
    require(encoder.ehlo("client.example.com") == "EHLO client.example.com\r\n", "EHLO encoding");
    require(encoder.startTls() == "STARTTLS\r\n", "STARTTLS encoding");
    require(encoder.authPlain("payload") == "AUTH PLAIN payload\r\n", "AUTH PLAIN encoding");
    require(encoder.mailFrom("sender@example.com") == "MAIL FROM:<sender@example.com>\r\n", "MAIL FROM encoding");
    require(encoder.rcptTo("rcpt@example.com") == "RCPT TO:<rcpt@example.com>\r\n", "RCPT TO encoding");
    require(encoder.dataCommand() == "DATA\r\n", "DATA command encoding");
    require(encoder.quit() == "QUIT\r\n", "QUIT encoding");
    require(encoder.dataBody("hello\r\n.line\r\n") == "hello\r\n..line\r\n.\r\n", "DATA dot stuffing");
    std::cout << "smtp protocol tests passed\n";
    return 0;
}
