#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                       /* HAVE_CONFIG_H */

#include "conversation.h"
#include "core.h"
#include "debug.h"
#include "util.h"
#include "version.h"

#include "gtkplugin.h"
#include "gtkconv.h"
#include "gtkimhtml.h"
#include <ctype.h>

#include "expand.h"

#ifdef WIN32
#include "win32dep.h"
#endif

#ifdef ENABLE_NLS
#include <glib/gi18n-lib.h>
#else
#define _(String) ((/* const */ char *) (String))
#define N_(String) ((/* const */ char *) (String))
#endif                       // ENABLE NLS

#include "local_util.h"

struct AccountConv {
    PurpleAccount  *account;
    PurpleConversation *conv;
};

typedef void    (*SHORTENER_CB) (const char *, gpointer);
struct Shortener {
    char            name[100];
    SHORTENER_CB    cb;
};

static PurplePlugin *expand_plugin = NULL;
// char           *shorteners[] = { "tinyurl.com", "bit.ly", "t.co", "is.gd", "j.mp", "goo.gl", "ow.ly" };

struct ExpandData {
    gchar          *original_url;
    gpointer        userdata;
};

static void     expand_twitlonger_cb(PurpleUtilFetchUrlData * url_data, gpointer userdata, const gchar * url_text, gsize len, const gchar * error_message);
static void     expand_shortlink_cb(PurpleUtilFetchUrlData * url_data, gpointer userdata, const gchar * url_text, gsize len, const gchar * error_message);
static void     replace(PurpleAccount * account, PurpleConversation * conv, const gchar * original, const gchar * new, gboolean link);
static void     expand_twitlonger(const char *url, gpointer userdata);
static void     expand_shortlink(const char *url, gpointer userdata);
static GList   *get_links(const char *text);
static gboolean displaying_msg(PurpleAccount * account, const char *message, PurpleConversation * conv);
static gboolean displaying_im_msg_cb(PurpleAccount * account, const char *who, char **message, PurpleConversation * conv, PurpleMessageFlags flags);
static gboolean displaying_chat_msg_cb(PurpleAccount * account, const char *who, char **message, PurpleConversation * conv, PurpleMessageFlags flags);
static gboolean plugin_load(PurplePlugin * plugin);
static gboolean plugin_unload(PurplePlugin * plugin);
static PurplePluginPrefFrame *get_plugin_pref_frame(PurplePlugin * plugin);
static void     plugin_destroy(PurplePlugin * plugin);
static void     plugin_init(PurplePlugin * plugin);
static gchar   *xmlnode_get_child_data(const xmlnode * node, const char *name);

struct Shortener shorteners[] = {
    {"tinyurl.com", expand_shortlink},
    {"bit.ly", expand_shortlink},
    {"t.co", expand_shortlink},
    {"is.gd", expand_shortlink},
    {"j.mp", expand_shortlink},
    {"goo.gl", expand_shortlink},
    {"ow.ly", expand_shortlink},
    {"tl.gd", expand_shortlink},
    {"twitlonger.com", expand_twitlonger},
    {"www.twitlonger.com", expand_twitlonger}
};

static gchar   *xmlnode_get_child_data(const xmlnode * node, const char *name)
{
    xmlnode        *child = xmlnode_get_child(node, name);
    if (!child)
        return NULL;
    return xmlnode_get_data_unescaped(child);
}

static void expand_twitlonger_cb(PurpleUtilFetchUrlData * url_data, gpointer userdata, const gchar * url_text, gsize len, const gchar * error_message)
{
    struct ExpandData *store = userdata;
    struct AccountConv *convmsg = NULL;
    xmlnode        *response_node = NULL;
    xmlnode        *post = NULL;
    gchar          *fulltext;

    if (store) {
        convmsg = store->userdata;
    }

    purple_debug_info(PLUGIN_ID, "MHM: Got |%s|\n", url_text);

    if (url_text && len) {
        response_node = xmlnode_from_str(url_text, strlen(url_text));
    } else {
        purple_debug_error(PLUGIN_ID, "Couldn't retreive! Error: %s\n", error_message);
    }

    if (response_node) {
        post = xmlnode_get_child(response_node, "post");
    }

    if (post) {
        gchar          *content = xmlnode_get_child_data(post, "content");

        fulltext = g_strdup_printf("... (%s)", content);

        g_free(content);

        replace(convmsg->account, convmsg->conv, store->original_url, fulltext, FALSE);
    } else {
        purple_debug_error(PLUGIN_ID, "Couldn't expand %s\n", store->original_url);
        return;
    }

    if (response_node)
        xmlnode_free(response_node);
    if (post)
        xmlnode_free(post);
    g_free(convmsg);
    g_free(fulltext);
    g_free(store->original_url);
    g_free(store);
}

static void expand_shortlink_cb(PurpleUtilFetchUrlData * url_data, gpointer userdata, const gchar * url_text, gsize len, const gchar * error_message)
{
    struct ExpandData *store = userdata;
    gchar          *location = NULL;
    gchar          *new_location = NULL;
    struct AccountConv *convmsg = NULL;

    if (store) {
        convmsg = store->userdata;
    }

    if (url_text && len) {
        location = g_strstr_len(url_text, len, "\nLocation: ");
    } else {
        purple_debug_error(PLUGIN_ID, "Couldn't retreive! Error: %s\n", error_message);
    }

    if (location) {
        location += 11;
        new_location = g_strndup(location, strchr(location, '\r') - location);
    }

    if (new_location) {
        /* Check for nesting */
        displaying_msg(convmsg->account, new_location, convmsg->conv);

        purple_debug_info(PLUGIN_ID, "Expanded |%s| into |%s|\n", store->original_url, new_location);

        replace(convmsg->account, convmsg->conv, store->original_url, new_location, TRUE);
    } else {
        purple_debug_error(PLUGIN_ID, "Couldn't expand %s\n", store->original_url);
        return;
    }

    g_free(convmsg);
    g_free(new_location);
    g_free(location);
    g_free(store->original_url);
    g_free(store);
}

static void expand_twitlonger(const char *url, gpointer userdata)
{
    struct ExpandData *store;
    gchar          *request_url;

    store = g_new0(struct ExpandData, 1);
    request_url = g_strdup_printf("%s/fulltext", url);
    store->original_url = g_strdup(url);
    store->userdata = userdata;

    purple_debug_misc(PLUGIN_ID, "Getting |%s| using |%s|...\n", url, request_url);

    purple_util_fetch_url_request(request_url, TRUE, "Mozilla/4.0 (compatible; MSIE 5.5)", FALSE, NULL, FALSE, expand_twitlonger_cb, store);

    g_free(request_url);
}

static void expand_shortlink(const char *url, gpointer userdata)
{
    struct ExpandData *store;
    gchar          *request;

    store = g_new0(struct ExpandData, 1);
    store->original_url = g_strdup(url);
    store->userdata = userdata;

    /* Host? */
    request = g_strdup_printf("HEAD %s HTTP/1.0\r\n" "User-Agent: Mozilla/4.0 (compatible; MSIE 5.5)\r\nContent-Length: 0\r\n\r\n", url);

    purple_debug_misc(PLUGIN_ID, "Getting |%s| using |%s|...\n", url, request);

    /* Custom version that doesn't follow redirects */
    local_purple_util_fetch_url_request(url, TRUE, "Mozilla/4.0 (compatible; MSIE 5.5)", FALSE, request, TRUE, expand_shortlink_cb, store);
    g_free(request);
}

static GList   *get_links(const char *text)
{
    GList          *urls = NULL;
    const char     *p;
    const char     *url_start;
    const char     *url_end;

    p = text;

    while (NULL != (url_start = strstr(p, "http"))) {
        gchar          *url = NULL;
        if (!memcmp(url_start + 4, "://", 3)) {
            p = url_start + 7;
        } else if (!memcmp(url_start + 4, "s://", 4)) {
            p = url_start + 8;
        } else {
            p = url_start + 4;
            continue;
        }
        /* TODO: replace this line with a better one. */
        /* TODO: replace this whole function with a better one! */
        while (*p && (*p == '.' || *p == '/' || *p == '-' || *p == '_' || (*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')))
            p++;
        url_end = p;
        url = g_strndup(url_start, url_end - url_start);

        urls = g_list_prepend(urls, url);
        /* URL will be freed later */
    }
    return urls;
}

static void replace(PurpleAccount * account, PurpleConversation * conv, const gchar * original, const gchar * new, gboolean link)
{
    GtkIMHtml      *imhtml = NULL;
    GtkTextBuffer  *text_buffer = NULL;

    GtkTextIter     buf_end;
    GtkTextIter     text_start;
    GtkTextIter     text_end;

    imhtml = GTK_IMHTML(PIDGIN_CONVERSATION(conv)->imhtml);
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(imhtml));

    /* Work backwards to be faster on large buffers */
    gtk_text_buffer_get_end_iter(text_buffer, &buf_end);

    if (!gtk_text_iter_backward_search(&buf_end, original, 0, &text_start, &text_end, NULL)) {
        purple_debug_warning(PLUGIN_ID, "Couldn't find the text (%s) in the buffer!\n", original);
        return;
    }
    gtk_text_buffer_delete(text_buffer, &text_start, &text_end);
    if (link) {
        GtkTextMark    *mark;
        mark = gtk_text_buffer_create_mark(text_buffer, "new_url", &text_start, TRUE);
        gtk_imhtml_insert_link(imhtml, mark, new, new);
        gtk_text_buffer_delete_mark(text_buffer, mark);
    } else {
        gtk_text_buffer_insert(text_buffer, &text_start, new, -1);
    }

    return;
}

static gboolean displaying_msg(PurpleAccount * account, const char *message, PurpleConversation * conv)
{
    gchar          *url;
    GList          *urls;
    GList          *cur;

    if (!purple_prefs_get_bool(EXPAND_PREF_EXPAND_ALL_LINKS)) {
        return FALSE;
    }

    purple_debug_info(PLUGIN_ID, "Received message |%s|\n", message);

    cur = urls = get_links(message);
    if (!urls) {
        purple_debug_misc(PLUGIN_ID, "No URLs found\n");
        return FALSE;
    }

    do {
        char           *host;
        int             port;
        char           *path;
        char           *user;
        char           *passwd;

        url = cur->data;
        purple_debug_misc(PLUGIN_ID, "Analyzing %s\n", url);

        if (purple_url_parse(url, &host, &port, &path, &user, &passwd)) {
            int             i;
            int             found = 0;
            for (i = 0; i < sizeof (shorteners) / sizeof (shorteners[0]) && !found; i++) {
                if (!strcmp(host, shorteners[i].name)) {
                    struct AccountConv *convmsg;

                    convmsg = g_new0(struct AccountConv, 1);
                    convmsg->account = account;
                    convmsg->conv = conv;

                    purple_debug_misc(PLUGIN_ID, "Known significant type!\n");

                    (*shorteners[i].cb) (url, convmsg);
                    found = 1;
                }
            }

            if (host)
                g_free(host);
            if (path)
                g_free(path);
            if (user)
                g_free(user);
            if (passwd)
                g_free(passwd);
        } else {
            purple_debug_error(PLUGIN_ID, "Couldn't parse URL! (%s)\n", url);
        }
        g_free(url);
    } while (NULL != (cur = g_list_next(cur)));
    g_list_free(urls);

    return FALSE;
}

static gboolean displaying_im_msg_cb(PurpleAccount * account, const char *who, char **message, PurpleConversation * conv, PurpleMessageFlags flags)
{
    if (flags & PURPLE_MESSAGE_RECV) {
        return displaying_msg(account, *message, conv);
    }
    return FALSE;
}

static gboolean displaying_chat_msg_cb(PurpleAccount * account, const char *who, char **message, PurpleConversation * conv, PurpleMessageFlags flags)
{
    if (flags & PURPLE_MESSAGE_RECV) {
        return displaying_msg(account, *message, conv);
    }
    return FALSE;
}

static gboolean plugin_load(PurplePlugin * plugin)
{
    expand_plugin = plugin;
    purple_signal_connect(pidgin_conversations_get_handle(), "displaying-im-msg", plugin, PURPLE_CALLBACK(displaying_im_msg_cb), NULL);
    purple_signal_connect(pidgin_conversations_get_handle(), "displaying-chat-msg", plugin, PURPLE_CALLBACK(displaying_chat_msg_cb), NULL);
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin * plugin)
{
    purple_prefs_disconnect_by_handle(plugin);
    purple_signals_disconnect_by_handle(plugin);
    return TRUE;
}

static PurplePluginPrefFrame *get_plugin_pref_frame(PurplePlugin * plugin)
{
    PurplePluginPrefFrame *frame;
    PurplePluginPref *ppref;

    frame = purple_plugin_pref_frame_new();

    ppref = purple_plugin_pref_new_with_name_and_label(EXPAND_PREF_EXPAND_ALL_LINKS, _("Automatically expand shortened links"));
    purple_plugin_pref_frame_add(frame, ppref);

    return frame;
}

static void plugin_destroy(PurplePlugin * plugin)
{
}

static PurplePluginUiInfo prefs_info = {
    get_plugin_pref_frame,
    0,                                           /* page_num (Reserved) */
    NULL,                                        /* frame (Reserved) */
    /* Padding */
    NULL,
    NULL,
    NULL,
    NULL
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,                           /**< major version */
    PURPLE_MINOR_VERSION,                           /**< minor version */
    PURPLE_PLUGIN_STANDARD,                         /**< type */
    PIDGIN_PLUGIN_TYPE,                             /**< ui_requirement */
    0,                                              /**< flags */
    NULL,                                           /**< dependencies */
    PURPLE_PRIORITY_DEFAULT,                        /**< priority */
    PLUGIN_ID,                                   /**< id */
    "Expand shortened URLs",                                      /**< name */
    PACKAGE_VERSION,                               /**< version */
    "Expand inline short URLs",                        /**< summary */
    "Expand inline short URLs",               /**< description */
    "Mike Miller <mikeage@gmail.com>",           /* author */
    "https://code.google.com/p/expand/",         /* homepage */
    plugin_load,                                    /**< load */
    plugin_unload,                                  /**< unload */
    plugin_destroy,                                 /**< destroy */
    NULL,                                           /**< ui_info */
    NULL,                                           /**< extra_info */
    &prefs_info,                                    /**< prefs_info */
    NULL,                                           /**< actions */
    NULL,                                        /* padding... */
    NULL,
    NULL,
    NULL,
};

static void plugin_init(PurplePlugin * plugin)
{
#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif                       /* ENABLE_NLS */
    info.summary = _("Expand inline short URLs");
    info.description = _("Expand inline short URLs");

    expand_plugin = plugin;

    purple_prefs_add_none(PREF_PREFIX);
    purple_prefs_add_bool(EXPAND_PREF_EXPAND_ALL_LINKS, TRUE);
}

PURPLE_INIT_PLUGIN(expand_plugin, plugin_init, info)
