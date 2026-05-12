#ifndef GALAY_MAIL_SMTP_PROTOCOL_H
#define GALAY_MAIL_SMTP_PROTOCOL_H

#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace galay::mail::protocol
{
    enum class SmtpParseError
    {
        Incomplete,
        InvalidFormat,
        InvalidCode,
    };

    struct SmtpReply
    {
        int code = 0;
        std::vector<std::string> lines;
        std::string message;

        [[nodiscard]] bool positiveCompletion() const noexcept { return code >= 200 && code < 300; }
        [[nodiscard]] bool positiveIntermediate() const noexcept { return code >= 300 && code < 400; }
        [[nodiscard]] bool negative() const noexcept { return code >= 400; }
    };

    class SmtpParser
    {
    public:
        std::expected<std::pair<size_t, SmtpReply>, SmtpParseError>
        parse(std::string_view data) const;
    };

    class SmtpEncoder
    {
    public:
        std::string ehlo(std::string_view domain) const;
        std::string helo(std::string_view domain) const;
        std::string startTls() const;
        std::string authPlain(std::string_view payload) const;
        std::string authLogin() const;
        std::string authLoginResponse(std::string_view payload) const;
        std::string mailFrom(std::string_view address) const;
        std::string rcptTo(std::string_view address) const;
        std::string dataCommand() const;
        std::string dataBody(std::string_view message) const;
        std::string rset() const;
        std::string noop() const;
        std::string quit() const;

    private:
        static std::string command(std::string_view verb, std::string_view arg = {});
    };
} // namespace galay::mail::protocol

#endif
