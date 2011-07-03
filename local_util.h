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
