#include "pop3_protocol.h"

#include <string>

namespace galay::mail::protocol
{
    std::expected<std::pair<size_t, Pop3Reply>, Pop3ParseError>
    Pop3Parser::parse(std::string_view data, bool multiline) const
    {
        const size_t first_crlf = data.find("\r\n");
        if (first_crlf == std::string_view::npos) {
            return std::unexpected(Pop3ParseError::Incomplete);
        }

        const std::string_view first = data.substr(0, first_crlf);
        Pop3Reply reply;
        if (first.starts_with("+OK")) {
            reply.status = Pop3Status::Ok;
            reply.message = first.size() > 3 && first[3] == ' ' ? std::string(first.substr(4)) : std::string{};
        } else if (first.starts_with("-ERR")) {
            reply.status = Pop3Status::Error;
            reply.message = first.size() > 4 && first[4] == ' ' ? std::string(first.substr(5)) : std::string{};
        } else {
            return std::unexpected(Pop3ParseError::InvalidFormat);
        }

        size_t offset = first_crlf + 2;
        if (!multiline || reply.status == Pop3Status::Error) {
            return std::pair{offset, std::move(reply)};
        }

        while (offset < data.size()) {
            const size_t crlf = data.find("\r\n", offset);
            if (crlf == std::string_view::npos) {
                return std::unexpected(Pop3ParseError::Incomplete);
            }

            std::string line(data.substr(offset, crlf - offset));
            offset = crlf + 2;
            if (line == ".") {
                return std::pair{offset, std::move(reply)};
            }
            if (line.starts_with("..")) {
                line.erase(line.begin());
            }
            reply.lines.emplace_back(std::move(line));
        }

        return std::unexpected(Pop3ParseError::Incomplete);
    }

    std::string Pop3Encoder::command(std::string_view verb, std::string_view arg)
    {
        std::string out(verb);
        if (!arg.empty()) {
            out.push_back(' ');
            out.append(arg.data(), arg.size());
        }
        out += "\r\n";
        return out;
    }

    std::string Pop3Encoder::capa() const { return command("CAPA"); }
    std::string Pop3Encoder::stls() const { return command("STLS"); }
    std::string Pop3Encoder::user(std::string_view username) const { return command("USER", username); }
    std::string Pop3Encoder::pass(std::string_view password) const { return command("PASS", password); }
    std::string Pop3Encoder::authPlain(std::string_view payload) const { return command("AUTH PLAIN", payload); }
    std::string Pop3Encoder::stat() const { return command("STAT"); }

    std::string Pop3Encoder::list(std::optional<int> message) const
    {
        return message ? command("LIST", std::to_string(*message)) : command("LIST");
    }

    std::string Pop3Encoder::retr(int message) const { return command("RETR", std::to_string(message)); }
    std::string Pop3Encoder::dele(int message) const { return command("DELE", std::to_string(message)); }
    std::string Pop3Encoder::noop() const { return command("NOOP"); }
    std::string Pop3Encoder::rset() const { return command("RSET"); }
    std::string Pop3Encoder::quit() const { return command("QUIT"); }
} // namespace galay::mail::protocol
