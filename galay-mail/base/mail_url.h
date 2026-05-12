#ifndef GALAY_MAIL_URL_H
#define GALAY_MAIL_URL_H

#include "mail_config.h"
#include "mail_error.h"
#include <expected>
#include <string>
#include <string_view>

namespace galay::mail
{
    std::expected<MailEndpoint, MailError> parseMailUrl(std::string_view url);
    std::string percentDecode(std::string_view value);
} // namespace galay::mail

#endif
