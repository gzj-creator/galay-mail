#ifndef GALAY_MAIL_POP3_PROTOCOL_H
#define GALAY_MAIL_POP3_PROTOCOL_H

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace galay::mail::protocol
{
    enum class Pop3Status
    {
        Ok,
        Error,
    };

    enum class Pop3ParseError
    {
        Incomplete,
        InvalidFormat,
    };

    struct Pop3Reply
    {
        Pop3Status status = Pop3Status::Ok;
        std::string message;
        std::vector<std::string> lines;

        [[nodiscard]] bool ok() const noexcept { return status == Pop3Status::Ok; }
    };

    class Pop3Parser
    {
    public:
        std::expected<std::pair<size_t, Pop3Reply>, Pop3ParseError>
        parse(std::string_view data, bool multiline) const;
    };

    class Pop3Encoder
    {
    public:
        std::string capa() const;
        std::string stls() const;
        std::string user(std::string_view username) const;
        std::string pass(std::string_view password) const;
        std::string authPlain(std::string_view payload) const;
        std::string stat() const;
        std::string list(std::optional<int> message = std::nullopt) const;
        std::string retr(int message) const;
        std::string dele(int message) const;
        std::string noop() const;
        std::string rset() const;
        std::string quit() const;

    private:
        static std::string command(std::string_view verb, std::string_view arg = {});
    };
} // namespace galay::mail::protocol

#endif
