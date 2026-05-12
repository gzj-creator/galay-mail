#include "smtp_protocol.h"

#include <charconv>
#include <cctype>

namespace galay::mail::protocol
{
    namespace
    {
        bool digit3(std::string_view line)
        {
            return line.size() >= 4 &&
                   std::isdigit(static_cast<unsigned char>(line[0])) &&
                   std::isdigit(static_cast<unsigned char>(line[1])) &&
                   std::isdigit(static_cast<unsigned char>(line[2]));
        }

        int parseCode(std::string_view line)
        {
            int code = 0;
            std::from_chars(line.data(), line.data() + 3, code);
            return code;
        }
    } // namespace

    std::expected<std::pair<size_t, SmtpReply>, SmtpParseError>
    SmtpParser::parse(std::string_view data) const
    {
        SmtpReply reply;
        size_t offset = 0;
        int expected_code = -1;

        while (offset < data.size()) {
            const size_t crlf = data.find("\r\n", offset);
            if (crlf == std::string_view::npos) {
                return std::unexpected(SmtpParseError::Incomplete);
            }

            const std::string_view line = data.substr(offset, crlf - offset);
            if (!digit3(line) || (line[3] != ' ' && line[3] != '-')) {
                return std::unexpected(SmtpParseError::InvalidFormat);
            }

            const int code = parseCode(line);
            if (expected_code < 0) {
                expected_code = code;
                reply.code = code;
            } else if (code != expected_code) {
                return std::unexpected(SmtpParseError::InvalidCode);
            }

            reply.lines.emplace_back(line.substr(4));
            offset = crlf + 2;
            if (line[3] == ' ') {
                for (size_t i = 0; i < reply.lines.size(); ++i) {
                    if (i != 0) {
                        reply.message.push_back('\n');
                    }
                    reply.message += reply.lines[i];
                }
                return std::pair{offset, std::move(reply)};
            }
        }

        return std::unexpected(SmtpParseError::Incomplete);
    }

    std::string SmtpEncoder::command(std::string_view verb, std::string_view arg)
    {
        std::string out(verb);
        if (!arg.empty()) {
            out.push_back(' ');
            out.append(arg.data(), arg.size());
        }
        out += "\r\n";
        return out;
    }

    std::string SmtpEncoder::ehlo(std::string_view domain) const { return command("EHLO", domain); }
    std::string SmtpEncoder::helo(std::string_view domain) const { return command("HELO", domain); }
    std::string SmtpEncoder::startTls() const { return command("STARTTLS"); }
    std::string SmtpEncoder::authPlain(std::string_view payload) const { return command("AUTH PLAIN", payload); }
    std::string SmtpEncoder::authLogin() const { return command("AUTH LOGIN"); }
    std::string SmtpEncoder::authLoginResponse(std::string_view payload) const { return command(payload); }

    std::string SmtpEncoder::mailFrom(std::string_view address) const
    {
        std::string arg = "FROM:<";
        arg.append(address.data(), address.size());
        arg.push_back('>');
        return command("MAIL", arg);
    }

    std::string SmtpEncoder::rcptTo(std::string_view address) const
    {
        std::string arg = "TO:<";
        arg.append(address.data(), address.size());
        arg.push_back('>');
        return command("RCPT", arg);
    }

    std::string SmtpEncoder::dataCommand() const { return command("DATA"); }
    std::string SmtpEncoder::rset() const { return command("RSET"); }
    std::string SmtpEncoder::noop() const { return command("NOOP"); }
    std::string SmtpEncoder::quit() const { return command("QUIT"); }

    std::string SmtpEncoder::dataBody(std::string_view message) const
    {
        std::string out;
        out.reserve(message.size() + 8);

        bool at_line_start = true;
        for (size_t i = 0; i < message.size(); ++i) {
            const char ch = message[i];
            if (at_line_start && ch == '.') {
                out.push_back('.');
            }
            out.push_back(ch);
            at_line_start = ch == '\n';
        }

        if (out.size() < 2 || out.substr(out.size() - 2) != "\r\n") {
            out += "\r\n";
        }
        out += ".\r\n";
        return out;
    }
} // namespace galay::mail::protocol
