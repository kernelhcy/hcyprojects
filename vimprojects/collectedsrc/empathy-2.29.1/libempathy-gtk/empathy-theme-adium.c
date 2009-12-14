/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008-2009 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 */

#include "config.h"

#include <string.h>
#include <glib/gi18n.h>

#include <webkit/webkit.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/util.h>

#include <gconf/gconf-client.h>
#include <pango/pango.h>
#include <gdk/gdk.h>

#include <libempathy/empathy-time.h>
#include <libempathy/empathy-utils.h>

#include "empathy-theme-adium.h"
#include "empathy-smiley-manager.h"
#include "empathy-conf.h"
#include "empathy-ui-utils.h"
#include "empathy-plist.h"

#define DEBUG_FLAG EMPATHY_DEBUG_CHAT
#include <libempathy/empathy-debug.h>

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyThemeAdium)

/* GConf key containing current value of font */
#define EMPATHY_GCONF_FONT_KEY_NAME       "/desktop/gnome/interface/font_name"
#define BORING_DPI_DEFAULT                96

/* "Join" consecutive messages with timestamps within five minutes */
#define MESSAGE_JOIN_PERIOD 5*60

typedef struct {
	EmpathyAdiumData     *data;
	EmpathySmileyManager *smiley_manager;
	EmpathyContact       *last_contact;
	time_t                last_timestamp;
	gboolean              last_is_backlog;
	gboolean              page_loaded;
	GList                *message_queue;
	guint                 notify_enable_webkit_developer_tools_id;
	GtkWidget            *inspector_window;
} EmpathyThemeAdiumPriv;

struct _EmpathyAdiumData {
	gint  ref_count;
	gchar *path;
	gchar *basedir;
	gchar *default_avatar_filename;
	gchar *default_incoming_avatar_filename;
	gchar *default_outgoing_avatar_filename;
	gchar *template_html;
	gchar *in_content_html;
	gsize  in_content_len;
	gchar *in_context_html;
	gsize  in_context_len;
	gchar *in_nextcontent_html;
	gsize  in_nextcontent_len;
	gchar *in_nextcontext_html;
	gsize  in_nextcontext_len;
	gchar *out_content_html;
	gsize  out_content_len;
	gchar *out_context_html;
	gsize  out_context_len;
	gchar *out_nextcontent_html;
	gsize  out_nextcontent_len;
	gchar *out_nextcontext_html;
	gsize  out_nextcontext_len;
	gchar *status_html;
	gsize  status_len;
	GHashTable *info;
};

static void theme_adium_iface_init (EmpathyChatViewIface *iface);

enum {
	PROP_0,
	PROP_ADIUM_DATA,
};

G_DEFINE_TYPE_WITH_CODE (EmpathyThemeAdium, empathy_theme_adium,
			 WEBKIT_TYPE_WEB_VIEW,
			 G_IMPLEMENT_INTERFACE (EMPATHY_TYPE_CHAT_VIEW,
						theme_adium_iface_init));

static void
theme_adium_update_enable_webkit_developer_tools (EmpathyThemeAdium *theme)
{
	WebKitWebView  *web_view = WEBKIT_WEB_VIEW (theme);
	gboolean        enable_webkit_developer_tools;

	if (!empathy_conf_get_bool (empathy_conf_get (),
				    EMPATHY_PREFS_CHAT_WEBKIT_DEVELOPER_TOOLS,
				    &enable_webkit_developer_tools)) {
		return;
	}

	g_object_set (G_OBJECT (webkit_web_view_get_settings (web_view)),
		      "enable-developer-extras",
		      enable_webkit_developer_tools,
		      NULL);
}

static void
theme_adium_notify_enable_webkit_developer_tools_cb (EmpathyConf *conf,
						     const gchar *key,
						     gpointer     user_data)
{
	EmpathyThemeAdium  *theme = user_data;

	theme_adium_update_enable_webkit_developer_tools (theme);
}

static gboolean
theme_adium_navigation_policy_decision_requested_cb (WebKitWebView             *view,
						     WebKitWebFrame            *web_frame,
						     WebKitNetworkRequest      *request,
						     WebKitWebNavigationAction *action,
						     WebKitWebPolicyDecision   *decision,
						     gpointer                   data)
{
	const gchar *uri;

	/* Only call url_show on clicks */
	if (webkit_web_navigation_action_get_reason (action) !=
	    WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {
		webkit_web_policy_decision_use (decision);
		return TRUE;
	}

	uri = webkit_network_request_get_uri (request);
	empathy_url_show (GTK_WIDGET (view), uri);

	webkit_web_policy_decision_ignore (decision);
	return TRUE;
}

static void
theme_adium_copy_address_cb (GtkMenuItem *menuitem,
			     gpointer     user_data)
{
	WebKitHitTestResult   *hit_test_result = WEBKIT_HIT_TEST_RESULT (user_data);
	gchar                 *uri;
	GtkClipboard          *clipboard;

	g_object_get (G_OBJECT (hit_test_result), "link-uri", &uri, NULL);

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text (clipboard, uri, -1);

	clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text (clipboard, uri, -1);

	g_free (uri);
}

static void
theme_adium_open_address_cb (GtkMenuItem *menuitem,
			     gpointer     user_data)
{
	WebKitHitTestResult   *hit_test_result = WEBKIT_HIT_TEST_RESULT (user_data);
	gchar                 *uri;

	g_object_get (G_OBJECT (hit_test_result), "link-uri", &uri, NULL);

	empathy_url_show (GTK_WIDGET (menuitem), uri);

	g_free (uri);
}

static gchar *
theme_adium_parse_body (EmpathyThemeAdium *theme,
			const gchar       *text)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (theme);
	gboolean               use_smileys = FALSE;
	GSList                *smileys, *l;
	GString               *string;
	gint                   i;
	GRegex                *uri_regex;
	GMatchInfo            *match_info;
	gboolean               match;
	gchar                 *ret = NULL;
	gint                   prev;

	empathy_conf_get_bool (empathy_conf_get (),
			       EMPATHY_PREFS_CHAT_SHOW_SMILEYS,
			       &use_smileys);

	/* Add <a href></a> arround links */
	uri_regex = empathy_uri_regex_dup_singleton ();
	match = g_regex_match (uri_regex, text, 0, &match_info);
	if (match) {
		gint last = 0;
		gint s = 0, e = 0;

		string = g_string_sized_new (strlen (text));
		do {
			gchar *real_url;

			g_match_info_fetch_pos (match_info, 0, &s, &e);

			if (s > last) {
				/* Append the text between last link (or the
				 * start of the message) and this link */
				gchar *str;
				str = g_markup_escape_text (text + last, s - last);
				g_string_append (string, str);
				g_free (str);
			}

			/* Append the link inside <a href=""></a> tag */
			real_url = empathy_make_absolute_url_len (text + s, e - s);

			g_string_append (string, "<a href=\"");
			g_string_append (string, real_url);
			g_string_append (string, "\">");
			g_string_append_len (string, text + s, e - s);
			g_string_append (string, "</a>");

			g_free (real_url);
			last = e;
		} while (g_match_info_next (match_info, NULL));

		if (e < (gint) strlen (text)) {
			/* Append the text after the last link */
			gchar *str;
			str = g_markup_escape_text (text + e, strlen (text) - e);
			g_string_append (string, str);
			g_free (str);
		}

		g_free (ret);
		text = ret = g_string_free (string, FALSE);
	} else if (use_smileys) {
		/* Replace smileys by a <img/> tag */
		string = g_string_sized_new (strlen (text));
		smileys = empathy_smiley_manager_parse (priv->smiley_manager, text);
		for (l = smileys; l; l = l->next) {
			EmpathySmiley *smiley;

			smiley = l->data;
			if (smiley->path) {
				g_string_append_printf (string,
							"<abbr title='%s'><img src=\"%s\"/ alt=\"%s\"/></abbr>",
							smiley->str, smiley->path, smiley->str);
			} else {
				gchar *str;

				str = g_markup_escape_text (smiley->str, -1);
				g_string_append (string, str);
				g_free (str);
			}
			empathy_smiley_free (smiley);
		}
		g_slist_free (smileys);

		g_free (ret);
		text = ret = g_string_free (string, FALSE);
	} else {
		text = ret = g_markup_escape_text (text, -1);
	}

	g_match_info_free (match_info);
	g_regex_unref (uri_regex);

	/* Replace \n by <br/> */
	string = NULL;
	prev = 0;
	for (i = 0; text[i] != '\0'; i++) {
		if (text[i] == '\n') {
			if (!string ) {
				string = g_string_sized_new (strlen (text));
			}
			g_string_append_len (string, text + prev, i - prev);
			g_string_append (string, "<br/>");
			prev = i + 1;
		}
	}
	if (string) {
		g_string_append (string, text + prev);
		g_free (ret);
		text = ret = g_string_free (string, FALSE);
	}

	return ret;
}

static void
escape_and_append_len (GString *string, const gchar *str, gint len)
{
	while (*str != '\0' && len != 0) {
		switch (*str) {
		case '\\':
			/* \ becomes \\ */
			g_string_append (string, "\\\\");
			break;
		case '\"':
			/* " becomes \" */
			g_string_append (string, "\\\"");
			break;
		case '\n':
			/* Remove end of lines */
			break;
		default:
			g_string_append_c (string, *str);
		}

		str++;
		len--;
	}
}

static gboolean
theme_adium_match (const gchar **str, const gchar *match)
{
	gint len;

	len = strlen (match);
	if (strncmp (*str, match, len) == 0) {
		*str += len - 1;
		return TRUE;
	}

	return FALSE;
}

static void
theme_adium_append_html (EmpathyThemeAdium *theme,
			 const gchar       *func,
			 const gchar       *html, gsize len,
		         const gchar       *message,
		         const gchar       *avatar_filename,
		         const gchar       *name,
		         const gchar       *contact_id,
		         const gchar       *service_name,
		         const gchar       *message_classes,
		         time_t             timestamp)
{
	GString     *string;
	const gchar *cur = NULL;
	gchar       *script;

	/* Make some search-and-replace in the html code */
	string = g_string_sized_new (len + strlen (message));
	g_string_append_printf (string, "%s(\"", func);
	for (cur = html; *cur != '\0'; cur++) {
		const gchar *replace = NULL;
		gchar       *dup_replace = NULL;

		if (theme_adium_match (&cur, "%message%")) {
			replace = message;
		} else if (theme_adium_match (&cur, "%messageClasses%")) {
			replace = message_classes;
		} else if (theme_adium_match (&cur, "%userIconPath%")) {
			replace = avatar_filename;
		} else if (theme_adium_match (&cur, "%sender%")) {
			replace = name;
		} else if (theme_adium_match (&cur, "%senderScreenName%")) {
			replace = contact_id;
		} else if (theme_adium_match (&cur, "%senderDisplayName%")) {
			/* %senderDisplayName% -
			 * "The serverside (remotely set) name of the sender,
			 *  such as an MSN display name."
			 *
			 * We don't have access to that yet so we use local
			 * alias instead.*/
			replace = name;
		} else if (theme_adium_match (&cur, "%service%")) {
			replace = service_name;
		} else if (theme_adium_match (&cur, "%shortTime%")) {
			dup_replace = empathy_time_to_string_local (timestamp,
				EMPATHY_TIME_FORMAT_DISPLAY_SHORT);
			replace = dup_replace;
		} else if (theme_adium_match (&cur, "%time")) {
			gchar *format = NULL;
			gchar *end;
			/* Time can be in 2 formats:
			 * %time% or %time{strftime format}%
			 * Extract the time format if provided. */
			if (cur[1] == '{') {
				cur += 2;
				end = strstr (cur, "}%");
				if (!end) {
					/* Invalid string */
					continue;
				}
				format = g_strndup (cur, end - cur);
				cur = end + 1;
			} else {
				cur++;
			}

			dup_replace = empathy_time_to_string_local (timestamp,
				format ? format : EMPATHY_TIME_FORMAT_DISPLAY_SHORT);
			replace = dup_replace;
			g_free (format);
		} else {
			escape_and_append_len (string, cur, 1);
			continue;
		}

		/* Here we have a replacement to make */
		escape_and_append_len (string, replace, -1);
		g_free (dup_replace);
	}
	g_string_append (string, "\")");

	script = g_string_free (string, FALSE);
	webkit_web_view_execute_script (WEBKIT_WEB_VIEW (theme), script);
	g_free (script);
}

static void
theme_adium_append_message (EmpathyChatView *view,
			    EmpathyMessage  *msg)
{
	EmpathyThemeAdium     *theme = EMPATHY_THEME_ADIUM (view);
	EmpathyThemeAdiumPriv *priv = GET_PRIV (theme);
	EmpathyContact        *sender;
	EmpathyAccount        *account;
	gchar                 *dup_body = NULL;
	const gchar           *body;
	const gchar           *name;
	const gchar           *contact_id;
	EmpathyAvatar         *avatar;
	const gchar           *avatar_filename = NULL;
	time_t                 timestamp;
	gchar                 *html = NULL;
	gsize                  len = 0;
	const gchar           *func;
	const gchar           *service_name;
	GString               *message_classes = NULL;
	gboolean               is_backlog;
	gboolean               consecutive;

	if (!priv->page_loaded) {
		priv->message_queue = g_list_prepend (priv->message_queue,
						      g_object_ref (msg));
		return;
	}

	/* Get information */
	sender = empathy_message_get_sender (msg);
	account = empathy_contact_get_account (sender);
	service_name = empathy_protocol_name_to_display_name
		(empathy_account_get_protocol (account));
	if (service_name == NULL)
		service_name = empathy_account_get_protocol (account);
	timestamp = empathy_message_get_timestamp (msg);
	body = empathy_message_get_body (msg);
	dup_body = theme_adium_parse_body (theme, body);
	if (dup_body) {
		body = dup_body;
	}
	name = empathy_contact_get_name (sender);
	contact_id = empathy_contact_get_id (sender);

	/* If this is a /me, append an event */
	if (empathy_message_get_tptype (msg) == TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION) {
		gchar *str;

		str = g_strdup_printf ("%s %s", name, body);
		empathy_chat_view_append_event (view, str);
		g_free (str);
		g_free (dup_body);
		return;
	}

	/* Get the avatar filename, or a fallback */
	avatar = empathy_contact_get_avatar (sender);
	if (avatar) {
		avatar_filename = avatar->filename;
	}
	if (!avatar_filename) {
		if (empathy_contact_is_user (sender)) {
			avatar_filename = priv->data->default_outgoing_avatar_filename;
		} else {
			avatar_filename = priv->data->default_incoming_avatar_filename;
		}
		if (!avatar_filename) {
			if (!priv->data->default_avatar_filename) {
				priv->data->default_avatar_filename =
					empathy_filename_from_icon_name ("stock_person",
									 GTK_ICON_SIZE_DIALOG);
			}
			avatar_filename = priv->data->default_avatar_filename;
		}
	}

	/* We want to join this message with the last one if
	 * - senders are the same contact,
	 * - last message was recieved recently,
	 * - last message and this message both are/aren't backlog, and
	 * - DisableCombineConsecutive is not set in theme's settings */
	is_backlog = empathy_message_is_backlog (msg);
	consecutive = empathy_contact_equal (priv->last_contact, sender) &&
		(timestamp - priv->last_timestamp < MESSAGE_JOIN_PERIOD) &&
		(is_backlog == priv->last_is_backlog) &&
		!tp_asv_get_boolean (priv->data->info,
				     "DisableCombineConsecutive", NULL);

	/* Define message classes */
	message_classes = g_string_new ("message");
	if (is_backlog) {
		g_string_append (message_classes, " history");
	}
	if (consecutive) {
		g_string_append (message_classes, " consecutive");
	}
	if (empathy_contact_is_user (sender)) {
		g_string_append (message_classes, " outgoing");
	} else {
		g_string_append (message_classes, " incoming");
	}

	/* Define javascript function to use */
	if (consecutive) {
		func = "appendNextMessage";
	} else {
		func = "appendMessage";
	}

	/* Outgoing */
	if (empathy_contact_is_user (sender)) {
		if (consecutive) {
			if (is_backlog) {
				html = priv->data->out_nextcontext_html;
				len = priv->data->out_nextcontext_len;
			}

			/* Not backlog, or fallback if NextContext.html
			 * is missing */
			if (html == NULL) {
				html = priv->data->out_nextcontent_html;
				len = priv->data->out_nextcontent_len;
			}
		}

		/* Not consecutive, or fallback if NextContext.html and/or
		 * NextContent.html are missing */
		if (html == NULL) {
			if (is_backlog) {
				html = priv->data->out_context_html;
				len = priv->data->out_context_len;
			}

			if (html == NULL) {
				html = priv->data->out_content_html;
				len = priv->data->out_content_len;
			}
		}
	}

	/* Incoming, or fallback if outgoing files are missing */
	if (html == NULL) {
		if (consecutive) {
			if (is_backlog) {
				html = priv->data->in_nextcontext_html;
				len = priv->data->in_nextcontext_len;
			}

			/* Note backlog, or fallback if NextContext.html
			 * is missing */
			if (html == NULL) {
				html = priv->data->in_nextcontent_html;
				len = priv->data->in_nextcontent_len;
			}
		}

		/* Not consecutive, or fallback if NextContext.html and/or
		 * NextContent.html are missing */
		if (html == NULL) {
			if (is_backlog) {
				html = priv->data->in_context_html;
				len = priv->data->in_context_len;
			}

			if (html == NULL) {
				html = priv->data->in_content_html;
				len = priv->data->in_content_len;
			}
		}
	}

	if (html != NULL) {
		theme_adium_append_html (theme, func, html, len, body,
					 avatar_filename, name, contact_id,
					 service_name, message_classes->str,
					 timestamp);
	} else {
		DEBUG ("Couldn't find HTML file for this message");
	}

	/* Keep the sender of the last displayed message */
	if (priv->last_contact) {
		g_object_unref (priv->last_contact);
	}
	priv->last_contact = g_object_ref (sender);
	priv->last_timestamp = timestamp;
	priv->last_is_backlog = is_backlog;

	g_free (dup_body);
	g_string_free (message_classes, TRUE);
}

static void
theme_adium_append_event (EmpathyChatView *view,
			  const gchar     *str)
{
	EmpathyThemeAdium     *theme = EMPATHY_THEME_ADIUM (view);
	EmpathyThemeAdiumPriv *priv = GET_PRIV (theme);

	if (priv->data->status_html) {
		gchar *str_escaped;

		str_escaped = g_markup_escape_text (str, -1);
		theme_adium_append_html (theme, "appendMessage",
					 priv->data->status_html,
					 priv->data->status_len,
					 str_escaped, NULL, NULL, NULL, NULL,
					 "event", empathy_time_get_current ());
		g_free (str_escaped);
	}

	/* There is no last contact */
	if (priv->last_contact) {
		g_object_unref (priv->last_contact);
		priv->last_contact = NULL;
	}
}

static void
theme_adium_scroll (EmpathyChatView *view,
		    gboolean         allow_scrolling)
{
	/* FIXME: Is it possible? I guess we need a js function, but I don't
	 * see any... */
}

static void
theme_adium_scroll_down (EmpathyChatView *view)
{
	webkit_web_view_execute_script (WEBKIT_WEB_VIEW (view), "scrollToBottom()");
}

static gboolean
theme_adium_get_has_selection (EmpathyChatView *view)
{
	return webkit_web_view_has_selection (WEBKIT_WEB_VIEW (view));
}

static void
theme_adium_clear (EmpathyChatView *view)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (view);
	gchar *basedir_uri;

	priv->page_loaded = FALSE;
	basedir_uri = g_strconcat ("file://", priv->data->basedir, NULL);
	webkit_web_view_load_html_string (WEBKIT_WEB_VIEW (view),
					  priv->data->template_html,
					  basedir_uri);
	g_free (basedir_uri);

	/* Clear last contact to avoid trying to add a 'joined'
	 * message when we don't have an insertion point. */
	if (priv->last_contact) {
		g_object_unref (priv->last_contact);
		priv->last_contact = NULL;
	}
}

static gboolean
theme_adium_find_previous (EmpathyChatView *view,
			   const gchar     *search_criteria,
			   gboolean         new_search)
{
	return webkit_web_view_search_text (WEBKIT_WEB_VIEW (view),
					    search_criteria, FALSE,
					    FALSE, TRUE);
}

static gboolean
theme_adium_find_next (EmpathyChatView *view,
		       const gchar     *search_criteria,
		       gboolean         new_search)
{
	return webkit_web_view_search_text (WEBKIT_WEB_VIEW (view),
					    search_criteria, FALSE,
					    TRUE, TRUE);
}

static void
theme_adium_find_abilities (EmpathyChatView *view,
			    const gchar    *search_criteria,
			    gboolean       *can_do_previous,
			    gboolean       *can_do_next)
{
	/* FIXME: Does webkit provide an API for that? We have wrap=true in
	 * find_next and find_previous to work around this problem. */
	if (can_do_previous)
		*can_do_previous = TRUE;
	if (can_do_next)
		*can_do_next = TRUE;
}

static void
theme_adium_highlight (EmpathyChatView *view,
		       const gchar     *text)
{
	webkit_web_view_unmark_text_matches (WEBKIT_WEB_VIEW (view));
	webkit_web_view_mark_text_matches (WEBKIT_WEB_VIEW (view),
					   text, FALSE, 0);
	webkit_web_view_set_highlight_text_matches (WEBKIT_WEB_VIEW (view),
						    TRUE);
}

static void
theme_adium_copy_clipboard (EmpathyChatView *view)
{
	webkit_web_view_copy_clipboard (WEBKIT_WEB_VIEW (view));
}

static void
theme_adium_context_menu_selection_done_cb (GtkMenuShell *menu, gpointer user_data)
{
	WebKitHitTestResult *hit_test_result = WEBKIT_HIT_TEST_RESULT (user_data);

	g_object_unref (hit_test_result);
}

static void
theme_adium_context_menu_for_event (EmpathyThemeAdium *theme, GdkEventButton *event)
{
	WebKitWebView              *view = WEBKIT_WEB_VIEW (theme);
	WebKitHitTestResult        *hit_test_result;
	WebKitHitTestResultContext  context;
	GtkWidget                  *menu;
	GtkWidget                  *item;

	hit_test_result = webkit_web_view_get_hit_test_result (view, event);
	g_object_get (G_OBJECT (hit_test_result), "context", &context, NULL);

	/* The menu */
	menu = gtk_menu_new ();

	/* Select all item */
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SELECT_ALL, NULL);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

	g_signal_connect_swapped (item, "activate",
				  G_CALLBACK (webkit_web_view_select_all),
				  view);

	/* Copy menu item */
	if (webkit_web_view_can_copy_clipboard (view)) {
		item = gtk_image_menu_item_new_from_stock (GTK_STOCK_COPY, NULL);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

		g_signal_connect_swapped (item, "activate",
					  G_CALLBACK (webkit_web_view_copy_clipboard),
					  view);
	}

	/* Clear menu item */
	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLEAR, NULL);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

	g_signal_connect_swapped (item, "activate",
				  G_CALLBACK (empathy_chat_view_clear),
				  view);

	/* We will only add the following menu items if we are
	 * right-clicking a link */
	if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK) {
		/* Separator */
		item = gtk_separator_menu_item_new ();
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

		/* Copy Link Address menu item */
		item = gtk_menu_item_new_with_mnemonic (_("_Copy Link Address"));
		g_signal_connect (item, "activate",
				  G_CALLBACK (theme_adium_copy_address_cb),
				  hit_test_result);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

		/* Open Link menu item */
		item = gtk_menu_item_new_with_mnemonic (_("_Open Link"));
		g_signal_connect (item, "activate",
				  G_CALLBACK (theme_adium_open_address_cb),
				  hit_test_result);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	}

	g_signal_connect (GTK_MENU_SHELL (menu), "selection-done",
			  G_CALLBACK (theme_adium_context_menu_selection_done_cb),
			  hit_test_result);

	/* Display the menu */
	gtk_widget_show_all (menu);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			event->button, event->time);
}

static gboolean
theme_adium_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	if (event->button == 3) {
		gboolean developer_tools_enabled;

		g_object_get (G_OBJECT (webkit_web_view_get_settings (WEBKIT_WEB_VIEW (widget))),
			      "enable-developer-extras", &developer_tools_enabled, NULL);

		/* We currently have no way to add an inspector menu
		 * item ourselves, so we disable our customized menu
		 * if the developer extras are enabled. */
		if (!developer_tools_enabled) {
			theme_adium_context_menu_for_event (EMPATHY_THEME_ADIUM (widget), event);
			return TRUE;
		}
	}

	return GTK_WIDGET_CLASS (empathy_theme_adium_parent_class)->button_press_event (widget, event);
}

static void
theme_adium_iface_init (EmpathyChatViewIface *iface)
{
	iface->append_message = theme_adium_append_message;
	iface->append_event = theme_adium_append_event;
	iface->scroll = theme_adium_scroll;
	iface->scroll_down = theme_adium_scroll_down;
	iface->get_has_selection = theme_adium_get_has_selection;
	iface->clear = theme_adium_clear;
	iface->find_previous = theme_adium_find_previous;
	iface->find_next = theme_adium_find_next;
	iface->find_abilities = theme_adium_find_abilities;
	iface->highlight = theme_adium_highlight;
	iface->copy_clipboard = theme_adium_copy_clipboard;
}

static void
theme_adium_load_finished_cb (WebKitWebView  *view,
			      WebKitWebFrame *frame,
			      gpointer        user_data)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (view);
	EmpathyChatView       *chat_view = EMPATHY_CHAT_VIEW (view);

	DEBUG ("Page loaded");
	priv->page_loaded = TRUE;

	/* Display queued messages */
	priv->message_queue = g_list_reverse (priv->message_queue);
	while (priv->message_queue) {
		EmpathyMessage *message = priv->message_queue->data;

		theme_adium_append_message (chat_view, message);
		priv->message_queue = g_list_remove (priv->message_queue, message);
		g_object_unref (message);
	}
}

static void
theme_adium_finalize (GObject *object)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (object);

	empathy_adium_data_unref (priv->data);

	empathy_conf_notify_remove (empathy_conf_get (),
				    priv->notify_enable_webkit_developer_tools_id);

	G_OBJECT_CLASS (empathy_theme_adium_parent_class)->finalize (object);
}

static void
theme_adium_dispose (GObject *object)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (object);

	if (priv->smiley_manager) {
		g_object_unref (priv->smiley_manager);
		priv->smiley_manager = NULL;
	}

	if (priv->last_contact) {
		g_object_unref (priv->last_contact);
		priv->last_contact = NULL;
	}

	if (priv->inspector_window) {
		gtk_widget_destroy (priv->inspector_window);
		priv->inspector_window = NULL;
	}

	G_OBJECT_CLASS (empathy_theme_adium_parent_class)->dispose (object);
}

static gboolean
theme_adium_inspector_show_window_cb (WebKitWebInspector *inspector,
				      EmpathyThemeAdium  *theme)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (theme);

	if (priv->inspector_window) {
		gtk_widget_show_all (priv->inspector_window);
	}

	return TRUE;
}

static gboolean
theme_adium_inspector_close_window_cb (WebKitWebInspector *inspector,
				       EmpathyThemeAdium  *theme)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (theme);

	if (priv->inspector_window) {
		gtk_widget_hide (priv->inspector_window);
	}

	return TRUE;
}

static WebKitWebView *
theme_adium_inspect_web_view_cb (WebKitWebInspector *inspector,
				 WebKitWebView      *web_view,
				 EmpathyThemeAdium  *theme)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (theme);
	GtkWidget             *scrolled_window;
	GtkWidget             *inspector_web_view;

	if (!priv->inspector_window) {
		/* Create main window */
		priv->inspector_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_default_size (GTK_WINDOW (priv->inspector_window),
					     800, 600);
		g_signal_connect (priv->inspector_window, "delete-event",
				  G_CALLBACK (gtk_widget_hide_on_delete), NULL);

		/* Pack a scrolled window */
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
						GTK_POLICY_AUTOMATIC,
						GTK_POLICY_AUTOMATIC);
		gtk_container_add (GTK_CONTAINER (priv->inspector_window),
				   scrolled_window);
		gtk_widget_show  (scrolled_window);

		/* Pack a webview in the scrolled window. That webview will be
		 * used to render the inspector tool.  */
		inspector_web_view = webkit_web_view_new ();
		gtk_container_add (GTK_CONTAINER (scrolled_window),
				   inspector_web_view);
		gtk_widget_show (scrolled_window);

		return WEBKIT_WEB_VIEW (inspector_web_view);
	}

	return NULL;
}

static PangoFontDescription *
theme_adium_get_default_font (void)
{
	GConfClient *gconf_client;
	PangoFontDescription *pango_fd;
	gchar *gconf_font_family;

	gconf_client = gconf_client_get_default ();
	if (gconf_client == NULL) {
		return NULL;
	}
	gconf_font_family = gconf_client_get_string (gconf_client,
		     EMPATHY_GCONF_FONT_KEY_NAME,
		     NULL);
	if (gconf_font_family == NULL) {
		g_object_unref (gconf_client);
		return NULL;
	}
	pango_fd = pango_font_description_from_string (gconf_font_family);
	g_free (gconf_font_family);
	g_object_unref (gconf_client);
	return pango_fd;
}

static void
theme_adium_set_webkit_font (WebKitWebSettings *w_settings,
			     const gchar *name,
			     gint size)
{
	g_object_set (w_settings, "default-font-family", name, NULL);
	g_object_set (w_settings, "default-font-size", size, NULL);
}

static void
theme_adium_set_default_font (WebKitWebSettings *w_settings)
{
	PangoFontDescription *default_font_desc;
	GdkScreen *current_screen;
	gdouble dpi = 0;
	gint pango_font_size = 0;

	default_font_desc = theme_adium_get_default_font ();
	if (default_font_desc == NULL)
		return ;
	pango_font_size = pango_font_description_get_size (default_font_desc)
		/ PANGO_SCALE ;
	if (pango_font_description_get_size_is_absolute (default_font_desc)) {
		current_screen = gdk_screen_get_default ();
		if (current_screen != NULL) {
			dpi = gdk_screen_get_resolution (current_screen);
		} else {
			dpi = BORING_DPI_DEFAULT;
		}
		pango_font_size = (gint) (pango_font_size / (dpi / 72));
	}
	theme_adium_set_webkit_font (w_settings,
		pango_font_description_get_family (default_font_desc),
		pango_font_size);
	pango_font_description_free (default_font_desc);
}

static void
theme_adium_constructed (GObject *object)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (object);
	gchar                 *basedir_uri;
	const gchar           *font_family = NULL;
	gint                   font_size = 0;
	WebKitWebView         *webkit_view = WEBKIT_WEB_VIEW (object);
	WebKitWebSettings     *webkit_settings;
	WebKitWebInspector    *webkit_inspector;

	/* Set default settings */
	font_family = tp_asv_get_string (priv->data->info, "DefaultFontFamily");
	font_size = tp_asv_get_int32 (priv->data->info, "DefaultFontSize", NULL);
	webkit_settings = webkit_web_view_get_settings (webkit_view);

	if (font_family && font_size) {
		theme_adium_set_webkit_font (webkit_settings, font_family, font_size);
	} else {
		theme_adium_set_default_font (webkit_settings);
	}

	/* Setup webkit inspector */
	webkit_inspector = webkit_web_view_get_inspector (webkit_view);
	g_signal_connect (webkit_inspector, "inspect-web-view",
			  G_CALLBACK (theme_adium_inspect_web_view_cb),
			  object);
	g_signal_connect (webkit_inspector, "show-window",
			  G_CALLBACK (theme_adium_inspector_show_window_cb),
			  object);
	g_signal_connect (webkit_inspector, "close-window",
			  G_CALLBACK (theme_adium_inspector_close_window_cb),
			  object);

	/* Load template */
	basedir_uri = g_strconcat ("file://", priv->data->basedir, NULL);
	webkit_web_view_load_html_string (WEBKIT_WEB_VIEW (object),
					  priv->data->template_html,
					  basedir_uri);
	g_free (basedir_uri);
}

static void
theme_adium_get_property (GObject    *object,
			  guint       param_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_ADIUM_DATA:
		g_value_set_boxed (value, priv->data);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
theme_adium_set_property (GObject      *object,
			  guint         param_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	EmpathyThemeAdiumPriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_ADIUM_DATA:
		g_assert (priv->data == NULL);
		priv->data = g_value_dup_boxed (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
empathy_theme_adium_class_init (EmpathyThemeAdiumClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = theme_adium_finalize;
	object_class->dispose = theme_adium_dispose;
	object_class->constructed = theme_adium_constructed;
	object_class->get_property = theme_adium_get_property;
	object_class->set_property = theme_adium_set_property;

	widget_class->button_press_event = theme_adium_button_press_event;

	g_object_class_install_property (object_class,
					 PROP_ADIUM_DATA,
					 g_param_spec_boxed ("adium-data",
							     "The theme data",
							     "Data for the adium theme",
							      EMPATHY_TYPE_ADIUM_DATA,
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_READWRITE |
							      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (EmpathyThemeAdiumPriv));
}

static void
empathy_theme_adium_init (EmpathyThemeAdium *theme)
{
	EmpathyThemeAdiumPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (theme,
		EMPATHY_TYPE_THEME_ADIUM, EmpathyThemeAdiumPriv);

	theme->priv = priv;

	priv->smiley_manager = empathy_smiley_manager_dup_singleton ();

	g_signal_connect (theme, "load-finished",
			  G_CALLBACK (theme_adium_load_finished_cb),
			  NULL);
	g_signal_connect (theme, "navigation-policy-decision-requested",
			  G_CALLBACK (theme_adium_navigation_policy_decision_requested_cb),
			  NULL);

	priv->notify_enable_webkit_developer_tools_id =
		empathy_conf_notify_add (empathy_conf_get (),
					 EMPATHY_PREFS_CHAT_WEBKIT_DEVELOPER_TOOLS,
					 theme_adium_notify_enable_webkit_developer_tools_cb,
					 theme);

	theme_adium_update_enable_webkit_developer_tools (theme);
}

EmpathyThemeAdium *
empathy_theme_adium_new (EmpathyAdiumData *data)
{
	g_return_val_if_fail (data != NULL, NULL);

	return g_object_new (EMPATHY_TYPE_THEME_ADIUM,
			     "adium-data", data,
			     NULL);
}

gboolean
empathy_adium_path_is_valid (const gchar *path)
{
	gboolean ret;
	gchar   *file;

	/* The theme is not valid if there is no Info.plist */
	file = g_build_filename (path, "Contents", "Info.plist",
				 NULL);
	ret = g_file_test (file, G_FILE_TEST_EXISTS);
	g_free (file);

	if (ret == FALSE)
		return ret;

	/* We ship a default Template.html as fallback if there is any problem
	 * with the one inside the theme. The only other required file is
	 * Content.html for incoming messages (outgoing fallback to use
	 * incoming). */
	file = g_build_filename (path, "Contents", "Resources", "Incoming",
				 "Content.html", NULL);
	ret = g_file_test (file, G_FILE_TEST_EXISTS);
	g_free (file);

	return ret;
}

GHashTable *
empathy_adium_info_new (const gchar *path)
{
	gchar *file;
	GValue *value;
	GHashTable *info = NULL;

	g_return_val_if_fail (empathy_adium_path_is_valid (path), NULL);

	file = g_build_filename (path, "Contents", "Info.plist", NULL);
	value = empathy_plist_parse_from_file (file);
	g_free (file);

	if (value == NULL)
		return NULL;

	info = g_value_dup_boxed (value);
	tp_g_value_slice_free (value);

	/* Insert the theme's path into the hash table,
	 * keys have to be dupped */
	tp_asv_set_string (info, g_strdup ("path"), path);

	return info;
}

GType
empathy_adium_data_get_type (void)
{
  static GType type_id = 0;

  if (!type_id)
    {
      type_id = g_boxed_type_register_static ("EmpathyAdiumData",
          (GBoxedCopyFunc) empathy_adium_data_ref,
          (GBoxedFreeFunc) empathy_adium_data_unref);
    }

  return type_id;
}

EmpathyAdiumData  *
empathy_adium_data_new_with_info (const gchar *path, GHashTable *info)
{
	EmpathyAdiumData *data;
	gchar            *file;
	gchar            *template_html = NULL;
	gsize             template_len;
	gchar            *footer_html = NULL;
	gsize             footer_len;
	GString          *string;
	gchar           **strv = NULL;
	gchar            *css_path;
	guint             len = 0;
	guint             i = 0;

	g_return_val_if_fail (empathy_adium_path_is_valid (path), NULL);

	data = g_slice_new0 (EmpathyAdiumData);
	data->ref_count = 1;
	data->path = g_strdup (path);
	data->basedir = g_strconcat (path, G_DIR_SEPARATOR_S "Contents"
		G_DIR_SEPARATOR_S "Resources" G_DIR_SEPARATOR_S, NULL);
	data->info = g_hash_table_ref (info);

	/* Load html files */
	file = g_build_filename (data->basedir, "Incoming", "Content.html", NULL);
	g_file_get_contents (file, &data->in_content_html, &data->in_content_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Incoming", "NextContent.html", NULL);
	g_file_get_contents (file, &data->in_nextcontent_html, &data->in_nextcontent_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Incoming", "Context.html", NULL);
	g_file_get_contents (file, &data->in_context_html, &data->in_context_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Incoming", "NextContext.html", NULL);
	g_file_get_contents (file, &data->in_nextcontext_html, &data->in_nextcontext_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Outgoing", "Content.html", NULL);
	g_file_get_contents (file, &data->out_content_html, &data->out_content_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Outgoing", "NextContent.html", NULL);
	g_file_get_contents (file, &data->out_nextcontent_html, &data->out_nextcontent_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Outgoing", "Context.html", NULL);
	g_file_get_contents (file, &data->out_context_html, &data->out_context_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Outgoing", "NextContext.html", NULL);
	g_file_get_contents (file, &data->out_nextcontext_html, &data->out_nextcontext_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Status.html", NULL);
	g_file_get_contents (file, &data->status_html, &data->status_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Footer.html", NULL);
	g_file_get_contents (file, &footer_html, &footer_len, NULL);
	g_free (file);

	file = g_build_filename (data->basedir, "Incoming", "buddy_icon.png", NULL);
	if (g_file_test (file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
		data->default_incoming_avatar_filename = file;
	} else {
		g_free (file);
	}

	file = g_build_filename (data->basedir, "Outgoing", "buddy_icon.png", NULL);
	if (g_file_test (file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
		data->default_outgoing_avatar_filename = file;
	} else {
		g_free (file);
	}

	css_path = g_build_filename (data->basedir, "main.css", NULL);

	/* There is 2 formats for Template.html: The old one has 4 parameters,
	 * the new one has 5 parameters. */
	file = g_build_filename (data->basedir, "Template.html", NULL);
	if (g_file_get_contents (file, &template_html, &template_len, NULL)) {
		strv = g_strsplit (template_html, "%@", -1);
		len = g_strv_length (strv);
	}
	g_free (file);

	if (len != 5 && len != 6) {
		/* Either the theme has no template or it don't have the good
		 * number of parameters. Fallback to use our own template. */
		g_free (template_html);
		g_strfreev (strv);

		file = empathy_file_lookup ("Template.html", "data");
		g_file_get_contents (file, &template_html, &template_len, NULL);
		g_free (file);
		strv = g_strsplit (template_html, "%@", -1);
		len = g_strv_length (strv);
	}

	/* Replace %@ with the needed information in the template html. */
	string = g_string_sized_new (template_len);
	g_string_append (string, strv[i++]);
	g_string_append (string, data->basedir);
	g_string_append (string, strv[i++]);
	if (len == 6) {
		const gchar *variant;

		/* We include main.css by default */
		g_string_append_printf (string, "@import url(\"%s\");", css_path);
		g_string_append (string, strv[i++]);
		variant = tp_asv_get_string (data->info, "DefaultVariant");
		if (variant) {
			g_string_append (string, "Variants/");
			g_string_append (string, variant);
			g_string_append (string, ".css");
		}
	} else {
		/* FIXME: We should set main.css OR the variant css */
		g_string_append (string, css_path);
	}
	g_string_append (string, strv[i++]);
	g_string_append (string, ""); /* We don't want header */
	g_string_append (string, strv[i++]);
	/* FIXME: We should replace adium %macros% in footer */
	if (footer_html) {
		g_string_append (string, footer_html);
	}
	g_string_append (string, strv[i++]);
	data->template_html = g_string_free (string, FALSE);

	g_free (footer_html);
	g_free (template_html);
	g_free (css_path);
	g_strfreev (strv);

	return data;
}

EmpathyAdiumData  *
empathy_adium_data_new (const gchar *path)
{
	EmpathyAdiumData *data;
	GHashTable *info;

	info = empathy_adium_info_new (path);
	data = empathy_adium_data_new_with_info (path, info);
	g_hash_table_unref (info);

	return data;
}

EmpathyAdiumData  *
empathy_adium_data_ref (EmpathyAdiumData *data)
{
	g_return_val_if_fail (data != NULL, NULL);

	g_atomic_int_inc (&data->ref_count);

	return data;
}

void
empathy_adium_data_unref (EmpathyAdiumData *data)
{
	g_return_if_fail (data != NULL);

	if (g_atomic_int_dec_and_test (&data->ref_count)) {
		g_free (data->path);
		g_free (data->basedir);
		g_free (data->template_html);
		g_free (data->in_content_html);
		g_free (data->in_nextcontent_html);
		g_free (data->in_context_html);
		g_free (data->in_nextcontext_html);
		g_free (data->out_content_html);
		g_free (data->out_nextcontent_html);
		g_free (data->out_context_html);
		g_free (data->out_nextcontext_html);
		g_free (data->default_avatar_filename);
		g_free (data->default_incoming_avatar_filename);
		g_free (data->default_outgoing_avatar_filename);
		g_free (data->status_html);
		g_hash_table_unref (data->info);
		g_slice_free (EmpathyAdiumData, data);
	}
}

GHashTable *
empathy_adium_data_get_info (EmpathyAdiumData *data)
{
	g_return_val_if_fail (data != NULL, NULL);

	return data->info;
}

const gchar *
empathy_adium_data_get_path (EmpathyAdiumData *data)
{
	g_return_val_if_fail (data != NULL, NULL);

	return data->path;
}

