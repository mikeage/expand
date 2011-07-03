/* Instead of internal.h */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_NLS
#  include <locale.h>
#  include <libintl.h>
#  ifndef _
#    define _(String) ((const char *)dgettext(PACKAGE, String))
#  endif
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  include <locale.h>
#  define N_(String) (String)
#  ifndef _
#    define _(String) ((const char *)String)
#  endif
#  define ngettext(Singular, Plural, Number) ((Number == 1) ? ((const char *)Singular) : ((const char *)Plural))
#  define dngettext(Domain, Singular, Plural, Number) ((Number == 1) ? ((const char *)Singular) : ((const char *)Plural))
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include "win32dep.h"
#endif

#include "util.h"

PurpleUtilFetchUrlData *
local_purple_util_fetch_url_request(const char *url, gboolean full,
		const char *user_agent, gboolean http11,
		const char *request, gboolean include_headers,
		PurpleUtilFetchUrlCallback callback, void *user_data);

PurpleUtilFetchUrlData *
local_purple_util_fetch_url_request_len(const char *url, gboolean full,
		const char *user_agent, gboolean http11,
		const char *request, gboolean include_headers, gssize max_len,
		PurpleUtilFetchUrlCallback callback, void *user_data);

PurpleUtilFetchUrlData *
local_purple_util_fetch_url_request_len_with_account(PurpleAccount *account,
		const char *url, gboolean full,	const char *user_agent, gboolean http11,
		const char *request, gboolean include_headers, gssize max_len,
		PurpleUtilFetchUrlCallback callback, void *user_data);
