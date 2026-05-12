#include "mail_error.h"

#include <utility>

namespace galay::mail
{
    MailError::MailError(MailErrorType type)
        : m_type(type)
    {
    }

    MailError::MailError(MailErrorType type, std::string extra_message)
        : m_type(type)
        , m_extra_message(std::move(extra_message))
    {
    }

    MailError::MailError(const galay::kernel::IOError& io_error)
        : m_type(MailErrorType::ConnectionError)
        , m_extra_message(io_error.message())
    {
    }

    MailError::MailError(const galay::ssl::SslError& ssl_error)
        : m_type(MailErrorType::TlsError)
        , m_extra_message(ssl_error.message())
    {
    }

    std::string MailError::message() const
    {
        std::string base;
        switch (m_type) {
        case MailErrorType::Success: base = "success"; break;
        case MailErrorType::InvalidUrl: base = "invalid url"; break;
        case MailErrorType::InvalidScheme: base = "invalid scheme"; break;
        case MailErrorType::InvalidHost: base = "invalid host"; break;
        case MailErrorType::InvalidPort: base = "invalid port"; break;
        case MailErrorType::InvalidCredential: base = "invalid credential"; break;
        case MailErrorType::ConnectionError: base = "connection error"; break;
        case MailErrorType::SendError: base = "send error"; break;
        case MailErrorType::RecvError: base = "receive error"; break;
        case MailErrorType::Timeout: base = "timeout"; break;
        case MailErrorType::ConnectionClosed: base = "connection closed"; break;
        case MailErrorType::TlsError: base = "tls error"; break;
        case MailErrorType::ParseError: base = "parse error"; break;
        case MailErrorType::Incomplete: base = "incomplete input"; break;
        case MailErrorType::ServerError: base = "server error"; break;
        case MailErrorType::UnsupportedAuth: base = "unsupported auth"; break;
        case MailErrorType::InvalidState: base = "invalid state"; break;
        case MailErrorType::Unknown: base = "unknown error"; break;
        }

        if (!m_extra_message.empty()) {
            base += ": ";
            base += m_extra_message;
        }
        return base;
    }
} // namespace galay::mail
