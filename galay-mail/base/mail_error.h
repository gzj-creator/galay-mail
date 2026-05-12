#ifndef GALAY_MAIL_ERROR_H
#define GALAY_MAIL_ERROR_H

#include <galay-kernel/common/error.h>
#include <galay-ssl/common/error.h>
#include <string>

namespace galay::mail
{
    enum class MailErrorType
    {
        Success,
        InvalidUrl,
        InvalidScheme,
        InvalidHost,
        InvalidPort,
        InvalidCredential,
        ConnectionError,
        SendError,
        RecvError,
        Timeout,
        ConnectionClosed,
        TlsError,
        ParseError,
        Incomplete,
        ServerError,
        UnsupportedAuth,
        InvalidState,
        Unknown,
    };

    class MailError
    {
    public:
        MailError(MailErrorType type = MailErrorType::Success);
        MailError(MailErrorType type, std::string extra_message);
        explicit MailError(const galay::kernel::IOError& io_error);
        explicit MailError(const galay::ssl::SslError& ssl_error);

        [[nodiscard]] MailErrorType type() const noexcept { return m_type; }
        [[nodiscard]] const std::string& extraMessage() const noexcept { return m_extra_message; }
        [[nodiscard]] std::string message() const;

    private:
        MailErrorType m_type = MailErrorType::Success;
        std::string m_extra_message;
    };
} // namespace galay::mail

#endif
