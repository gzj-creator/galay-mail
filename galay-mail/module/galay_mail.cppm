module;

#include "galay-mail/module/module_prelude.hpp"

export module galay.mail;

export {
#include "galay-mail/base/mail_config.h"
#include "galay-mail/base/mail_error.h"
#include "galay-mail/base/mail_url.h"
#include "galay-mail/protocol/auth.h"
#include "galay-mail/protocol/smtp_protocol.h"
#include "galay-mail/protocol/pop3_protocol.h"
#include "galay-mail/async/mail_transport.h"
#include "galay-mail/async/smtp_client.h"
#include "galay-mail/async/pop3_client.h"
}
