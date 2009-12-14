/* * Copyright (C) 2007-2009 Collabora Ltd.
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
 *          Sjoerd Simons <sjoerd.simons@collabora.co.uk>
 *          Cosimo Cecchi <cosimo.cecchi@collabora.co.uk>
 */

#define DISPATCHER_BUS_NAME TP_CLIENT_BUS_NAME_BASE "Empathy"
#define DISPATCHER_OBJECT_PATH TP_CLIENT_OBJECT_PATH_BASE "Empathy"

#include <config.h>

#include <string.h>

#include <glib/gi18n-lib.h>

#include <telepathy-glib/enums.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/proxy-subclass.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/defs.h>
#include <telepathy-glib/svc-client.h>
#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/interfaces.h>

#include <extensions/extensions.h>

#include "empathy-dispatcher.h"
#include "empathy-handler.h"
#include "empathy-utils.h"
#include "empathy-tube-handler.h"
#include "empathy-account-manager.h"
#include "empathy-tp-contact-factory.h"
#include "empathy-chatroom-manager.h"
#include "empathy-utils.h"

#define DEBUG_FLAG EMPATHY_DEBUG_DISPATCHER
#include <libempathy/empathy-debug.h>

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyDispatcher)
typedef struct
{
  gboolean dispose_has_run;

  EmpathyAccountManager *account_manager;
  /* connection to connection data mapping */
  GHashTable *connections;
  GHashTable *outstanding_classes_requests;
  gpointer token;
  GSList *tubes;

  /* channels which the dispatcher is listening "invalidated" */
  GList *channels;
  GPtrArray *array;

  /* main handler */
  EmpathyHandler *handler;

  /* extra handlers */
  GList *handlers;

  GHashTable *request_channel_class_async_ids;
} EmpathyDispatcherPriv;

static GList *
empathy_dispatcher_get_channels (EmpathyHandler *handler,
    gpointer user_data);

static gboolean
empathy_dispatcher_handle_channels (EmpathyHandler *handler,
    const gchar *account_path,
    const gchar *connection_path,
    const GPtrArray *channels,
    const GPtrArray *requests_satisfied,
    guint64 timestamp,
    GHashTable *handler_info,
    gpointer user_data,
    GError **error);

G_DEFINE_TYPE (EmpathyDispatcher, empathy_dispatcher, G_TYPE_OBJECT);

enum
{
  PROP_INTERFACES = 1,
  PROP_HANDLER,
};

enum
{
  OBSERVE,
  APPROVE,
  DISPATCH,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];
static EmpathyDispatcher *dispatcher = NULL;

static void dispatcher_init_connection_if_needed (
    EmpathyDispatcher *dispatcher,
    TpConnection *connection);

static GList * empathy_dispatcher_find_channel_classes
  (EmpathyDispatcher *dispatcher, TpConnection *connection,
   const gchar *channel_type, guint handle_type, GArray *fixed_properties);


typedef struct
{
  EmpathyDispatcher *dispatcher;
  EmpathyDispatchOperation *operation;
  TpConnection *connection;
  gboolean should_ensure;
  gchar *channel_type;
  guint handle_type;
  guint handle;
  EmpathyContact *contact;
  TpProxyPendingCall *pending_call;

  /* Properties to pass to the channel when requesting it */
  GHashTable *request;
  EmpathyDispatcherRequestCb *cb;
  gpointer user_data;
  gpointer *request_data;
} DispatcherRequestData;

typedef struct
{
  TpChannel *channel;
  /* Channel type specific wrapper object */
  GObject *channel_wrapper;
} DispatchData;

typedef struct
{
  /* ObjectPath => DispatchData.. */
  GHashTable *dispatched_channels;
  /* ObjectPath -> EmpathyDispatchOperations */
  GHashTable *dispatching_channels;

  /* ObjectPath -> EmpathyDispatchOperations
   *
   * This holds channels which were announced with NewChannel while we have an
   * outstanding channel request for a channel of this type. On the Requests
   * interface, CreateChannel and EnsureChannel are guaranteed by the spec to
   * return before NewChannels is emitted, but there was no guarantee of the
   * ordering of RequestChannel vs. NewChannel. So if necessary, channels are
   * held in limbo here until we know whether they were requested.
   */
  GHashTable *outstanding_channels;
  /* List of DispatcherRequestData */
  GList *outstanding_requests;
  /* List of requestable channel classes */
  GPtrArray *requestable_channels;
} ConnectionData;

typedef struct
{
  EmpathyDispatcher *dispatcher;
  TpConnection *connection;
  char *channel_type;
  guint handle_type;
  GArray *properties;
  EmpathyDispatcherFindChannelClassCb *callback;
  gpointer user_data;
} FindChannelRequest;

static void
empathy_dispatcher_call_create_or_ensure_channel (
    EmpathyDispatcher *dispatcher,
    DispatcherRequestData *request_data);

static DispatchData *
new_dispatch_data (TpChannel *channel,
                   GObject *channel_wrapper)
{
  DispatchData *d = g_slice_new0 (DispatchData);
  d->channel = g_object_ref (channel);
  if (channel_wrapper != NULL)
    d->channel_wrapper = g_object_ref (channel_wrapper);

  return d;
}

static void
free_dispatch_data (DispatchData *data)
{
  g_object_unref (data->channel);
  if (data->channel_wrapper != NULL)
    g_object_unref (data->channel_wrapper);

  g_slice_free (DispatchData, data);
}

static DispatcherRequestData *
new_dispatcher_request_data (EmpathyDispatcher *self,
                             TpConnection *connection,
                             const gchar *channel_type,
                             guint handle_type,
                             guint handle,
                             GHashTable *request,
                             EmpathyContact *contact,
                             EmpathyDispatcherRequestCb *cb,
                             gpointer user_data)
{
  DispatcherRequestData *result = g_slice_new0 (DispatcherRequestData);

  result->dispatcher = g_object_ref (self);
  result->connection = connection;

  result->should_ensure = FALSE;

  result->channel_type = g_strdup (channel_type);
  result->handle_type = handle_type;
  result->handle = handle;
  result->request = request;

  if (contact != NULL)
    result->contact = g_object_ref (contact);

  result->cb = cb;
  result->user_data = user_data;

  return result;
}

static void
free_dispatcher_request_data (DispatcherRequestData *r)
{
  g_free (r->channel_type);

  if (r->dispatcher != NULL)
    g_object_unref (r->dispatcher);

  if (r->contact != NULL)
    g_object_unref (r->contact);

  if (r->request != NULL)
    g_hash_table_unref (r->request);

  if (r->pending_call != NULL)
    tp_proxy_pending_call_cancel (r->pending_call);

  g_slice_free (DispatcherRequestData, r);
}

static ConnectionData *
new_connection_data (void)
{
  ConnectionData *cd = g_slice_new0 (ConnectionData);

  cd->dispatched_channels = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, (GDestroyNotify) free_dispatch_data);

  cd->dispatching_channels = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, g_object_unref);

  cd->outstanding_channels = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, NULL);

  return cd;
}

static void
free_connection_data (ConnectionData *cd)
{
  GList *l;
  guint i;

  g_hash_table_destroy (cd->dispatched_channels);
  g_hash_table_destroy (cd->dispatching_channels);

  for (l = cd->outstanding_requests ; l != NULL; l = g_list_delete_link (l,l))
    {
      free_dispatcher_request_data (l->data);
    }

  if (cd->requestable_channels  != NULL)
    {
      for (i = 0 ; i < cd->requestable_channels->len ; i++)
          g_value_array_free (
            g_ptr_array_index (cd->requestable_channels, i));
      g_ptr_array_free (cd->requestable_channels, TRUE);
    }
}

static void
free_find_channel_request (FindChannelRequest *r)
{
  guint idx;
  char *str;

  g_object_unref (r->dispatcher);
  g_free (r->channel_type);

  if (r->properties != NULL)
    {
      for (idx = 0; idx < r->properties->len ; idx++)
        {
          str = g_array_index (r->properties, char *, idx);
          g_free (str);
        }

      g_array_free (r->properties, TRUE);
    }

  g_slice_free (FindChannelRequest, r);
}

static void
dispatcher_connection_invalidated_cb (TpConnection *connection,
                                      guint domain,
                                      gint code,
                                      gchar *message,
                                      EmpathyDispatcher *self)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);

  DEBUG ("Error: %s", message);
  g_hash_table_remove (priv->connections, connection);
}

static gboolean
dispatcher_operation_can_start (EmpathyDispatcher *self,
                                EmpathyDispatchOperation *operation,
                                ConnectionData *cd)
{
  GList *l;
  const gchar *channel_type =
    empathy_dispatch_operation_get_channel_type (operation);

  for (l = cd->outstanding_requests; l != NULL; l = g_list_next (l))
    {
      DispatcherRequestData *d = (DispatcherRequestData *) l->data;

      if (d->operation == NULL && !tp_strdiff (d->channel_type, channel_type))
        {
          return FALSE;
        }
    }

  return TRUE;
}

static void
dispatch_operation_flush_requests (EmpathyDispatcher *self,
                                   EmpathyDispatchOperation *operation,
                                   GError *error,
                                   ConnectionData *cd)
{
  GList *l;

  l = cd->outstanding_requests;
  while (l != NULL)
    {
      DispatcherRequestData *d = (DispatcherRequestData *) l->data;
      GList *lt = l;

      l = g_list_next (l);

      if (d->operation == operation)
        {
          if (d->cb != NULL)
            {
              if (error != NULL)
                d->cb (NULL, error, d->user_data);
              else
                d->cb (operation, NULL, d->user_data);
            }

          cd->outstanding_requests = g_list_delete_link
            (cd->outstanding_requests, lt);

          free_dispatcher_request_data (d);
        }
    }
}

static void
dispatcher_channel_invalidated_cb (TpProxy *proxy,
                                   guint domain,
                                   gint code,
                                   gchar *message,
                                   EmpathyDispatcher *self)
{
  /* Channel went away... */
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  TpConnection *connection;
  EmpathyDispatchOperation *operation;
  ConnectionData *cd;
  const gchar *object_path;

  connection = tp_channel_borrow_connection (TP_CHANNEL (proxy));

  cd = g_hash_table_lookup (priv->connections, connection);
  /* Connection itself invalidated? */
  if (cd == NULL)
    return;

  object_path = tp_proxy_get_object_path (proxy);

  DEBUG ("Channel %s invalidated", object_path);

  g_hash_table_remove (cd->dispatched_channels, object_path);
  g_hash_table_remove (cd->dispatching_channels, object_path);

  priv->channels = g_list_remove (priv->channels, proxy);

  operation = g_hash_table_lookup (cd->outstanding_channels, object_path);
  if (operation != NULL)
    {
      GError error = { domain, code, message };
      dispatch_operation_flush_requests (self, operation, &error, cd);
      g_hash_table_remove (cd->outstanding_channels, object_path);
      g_object_unref (operation);
    }
}

static void
dispatch_operation_approved_cb (EmpathyDispatchOperation *operation,
                                EmpathyDispatcher *self)
{
  g_assert (empathy_dispatch_operation_is_incoming (operation));
  DEBUG ("Send of for dispatching: %s",
    empathy_dispatch_operation_get_object_path (operation));
  g_signal_emit (self, signals[DISPATCH], 0, operation);
}

static void
dispatch_operation_claimed_cb (EmpathyDispatchOperation *operation,
                               EmpathyDispatcher *self)
{
  /* Our job is done, remove the dispatch operation and mark the channel as
   * dispatched */
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  TpConnection *connection;
  ConnectionData *cd;
  const gchar *object_path;

  connection = empathy_dispatch_operation_get_tp_connection (operation);
  cd = g_hash_table_lookup (priv->connections, connection);
  g_assert (cd != NULL);

  object_path = empathy_dispatch_operation_get_object_path (operation);

  if (g_hash_table_lookup (cd->dispatched_channels, object_path) == NULL)
    {
      DispatchData *d;
      d = new_dispatch_data (
        empathy_dispatch_operation_get_channel (operation),
        empathy_dispatch_operation_get_channel_wrapper (operation));
      g_hash_table_insert (cd->dispatched_channels,
        g_strdup (object_path), d);
    }
  g_hash_table_remove (cd->dispatching_channels, object_path);

  DEBUG ("Channel claimed: %s", object_path);
}

static void
dispatch_operation_ready_cb (EmpathyDispatchOperation *operation,
                             EmpathyDispatcher *self)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  TpConnection *connection;
  ConnectionData *cd;
  EmpathyDispatchOperationState status;

  g_signal_connect (operation, "approved",
    G_CALLBACK (dispatch_operation_approved_cb), self);

  g_signal_connect (operation, "claimed",
    G_CALLBACK (dispatch_operation_claimed_cb), self);

  /* Signal the observers */
  DEBUG ("Send to observers: %s",
    empathy_dispatch_operation_get_object_path (operation));
  g_signal_emit (self, signals[OBSERVE], 0, operation);

  empathy_dispatch_operation_start (operation);

  /* Signal potential requestors */
  connection =  empathy_dispatch_operation_get_tp_connection (operation);
  cd = g_hash_table_lookup (priv->connections, connection);
  g_assert (cd != NULL);

  g_object_ref (operation);
  g_object_ref (self);

  dispatch_operation_flush_requests (self, operation, NULL, cd);
  status = empathy_dispatch_operation_get_status (operation);
  g_object_unref (operation);

  if (status == EMPATHY_DISPATCHER_OPERATION_STATE_CLAIMED)
    return;

  if (status == EMPATHY_DISPATCHER_OPERATION_STATE_APPROVING)
    {
      DEBUG ("Send to approvers: %s",
        empathy_dispatch_operation_get_object_path (operation));
      g_signal_emit (self, signals[APPROVE], 0, operation);
    }
  else
    {
      g_assert (status == EMPATHY_DISPATCHER_OPERATION_STATE_DISPATCHING);
      DEBUG ("Send of for dispatching: %s",
        empathy_dispatch_operation_get_object_path (operation));
      g_signal_emit (self, signals[DISPATCH], 0, operation);
    }

  g_object_unref (self);
}

static void
dispatcher_start_dispatching (EmpathyDispatcher *self,
                              EmpathyDispatchOperation *operation,
                              ConnectionData *cd)
{
  const gchar *object_path =
    empathy_dispatch_operation_get_object_path (operation);

  DEBUG ("Dispatching process started for %s", object_path);

  if (g_hash_table_lookup (cd->dispatching_channels, object_path) == NULL)
    {
      g_assert (g_hash_table_lookup (cd->outstanding_channels,
        object_path) == NULL);

      g_hash_table_insert (cd->dispatching_channels,
        g_strdup (object_path), operation);

      switch (empathy_dispatch_operation_get_status (operation))
        {
          case EMPATHY_DISPATCHER_OPERATION_STATE_PREPARING:
            g_signal_connect (operation, "ready",
              G_CALLBACK (dispatch_operation_ready_cb), self);
            break;
          case EMPATHY_DISPATCHER_OPERATION_STATE_PENDING:
            dispatch_operation_ready_cb (operation, self);
            break;
          default:
            g_assert_not_reached ();
        }

    }
  else if (empathy_dispatch_operation_get_status (operation) >=
      EMPATHY_DISPATCHER_OPERATION_STATE_PENDING)
    {
      /* Already dispatching and the operation is pending, thus the observers
       * have seen it (if applicable), so we can flush the request right away.
       */
      dispatch_operation_flush_requests (self, operation, NULL, cd);
    }
}

static void
dispatcher_flush_outstanding_operations (EmpathyDispatcher *self,
                                         ConnectionData *cd)
{
  GHashTableIter iter;
  gpointer value;

  g_hash_table_iter_init (&iter, cd->outstanding_channels);
  while (g_hash_table_iter_next (&iter, NULL, &value))
    {
      EmpathyDispatchOperation *operation = EMPATHY_DISPATCH_OPERATION (value);

      if (dispatcher_operation_can_start (self, operation, cd))
        {
          g_hash_table_iter_remove (&iter);
          dispatcher_start_dispatching (self, operation, cd);
        }
    }
}

static void
dispatcher_connection_new_channel (EmpathyDispatcher *self,
                                   TpConnection *connection,
                                   const gchar *object_path,
                                   const gchar *channel_type,
                                   guint handle_type,
                                   guint handle,
                                   GHashTable *properties,
                                   gboolean incoming)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  TpChannel         *channel;
  ConnectionData *cd;
  EmpathyDispatchOperation *operation;
  int i;
  /* Channel types we never want to dispatch because they're either deprecated
   * or can't sensibly be dispatch (e.g. channels that should always be
   * requested) */
  const char *blacklist[] = {
    TP_IFACE_CHANNEL_TYPE_CONTACT_LIST,
    TP_IFACE_CHANNEL_TYPE_TUBES,
    TP_IFACE_CHANNEL_TYPE_ROOM_LIST,
    NULL
  };

  dispatcher_init_connection_if_needed (self, connection);

  cd = g_hash_table_lookup (priv->connections, connection);

  /* Don't bother with channels we have already dispatched or are dispatching
   * currently. This can happen when NewChannel(s) is fired after
   * RequestChannel/CreateChannel/EnsureChannel */
  if (g_hash_table_lookup (cd->dispatched_channels, object_path) != NULL)
    return;

  if (g_hash_table_lookup (cd->dispatching_channels, object_path) != NULL)
    return;

  /* Should never occur, but just in case a CM fires spurious NewChannel(s)
   * signals */
  if (g_hash_table_lookup (cd->outstanding_channels, object_path) != NULL)
    return;

  /* Only pick up non-requested text and file channels. For all other it
   * doesn't make sense to handle it if we didn't request it. The same goes
   * for channels we discovered by the Channels property or ListChannels */
  if (!incoming && tp_strdiff (channel_type, TP_IFACE_CHANNEL_TYPE_TEXT)
        && tp_strdiff (channel_type, TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER))
    {
      DEBUG ("Ignoring incoming channel of type %s on %s",
        channel_type, object_path);
      return;
    }

  for (i = 0 ; blacklist[i] != NULL; i++)
    {
      if (!tp_strdiff (channel_type, blacklist[i]))
        {
          DEBUG ("Ignoring blacklisted channel type %s on %s",
            channel_type, object_path);
          return;
        }
    }

  DEBUG ("New channel of type %s on %s", channel_type, object_path);

  if (properties == NULL)
    channel = tp_channel_new (connection, object_path, channel_type,
      handle_type, handle, NULL);
  else
    channel = tp_channel_new_from_properties (connection, object_path,
      properties, NULL);

  g_signal_connect (channel, "invalidated",
    G_CALLBACK (dispatcher_channel_invalidated_cb),
    self);

  priv->channels = g_list_prepend (priv->channels, channel);

  operation = empathy_dispatch_operation_new (connection, channel, NULL,
    incoming);

  g_object_unref (channel);

  if (incoming)
    {
      /* Request could either be by us or by a remote party. If there are no
       * outstanding requests for this channel type we can assume it's remote.
       * Otherwise we wait untill they are all satisfied */
      if (dispatcher_operation_can_start (self, operation, cd))
        dispatcher_start_dispatching (self, operation, cd);
      else
        g_hash_table_insert (cd->outstanding_channels,
          g_strdup (object_path), operation);
    }
  else
    {
      dispatcher_start_dispatching (self, operation, cd);
    }
}

static void
dispatcher_connection_new_channel_with_properties (
    EmpathyDispatcher *self,
    TpConnection *connection,
    const gchar *object_path,
    GHashTable *properties)
{
  const gchar *channel_type;
  guint handle_type;
  guint handle;
  gboolean requested;
  gboolean valid;


  channel_type = tp_asv_get_string (properties,
    TP_IFACE_CHANNEL ".ChannelType");
  if (channel_type == NULL)
    {
      g_message ("%s had an invalid ChannelType property", object_path);
      return;
    }

  handle_type = tp_asv_get_uint32 (properties,
    TP_IFACE_CHANNEL ".TargetHandleType", &valid);
  if (!valid)
    {
      g_message ("%s had an invalid TargetHandleType property", object_path);
      return;
    }

  handle = tp_asv_get_uint32 (properties,
    TP_IFACE_CHANNEL ".TargetHandle", &valid);
  if (!valid)
    {
      g_message ("%s had an invalid TargetHandle property", object_path);
      return;
    }

  /* We assume there is no channel dispather, so we're the only one dispatching
   * it. Which means that a requested channel it is outgoing one */
  requested = tp_asv_get_boolean (properties,
    TP_IFACE_CHANNEL ".Requested", &valid);
  if (!valid)
    {
      g_message ("%s had an invalid Requested property", object_path);
      requested = FALSE;
    }

  dispatcher_connection_new_channel (self, connection,
    object_path, channel_type, handle_type, handle, properties, !requested);
}

static void
dispatcher_connection_got_all (TpProxy *proxy,
                               GHashTable *properties,
                               const GError *error,
                               gpointer user_data,
                               GObject *object)
{
  EmpathyDispatcher *self = EMPATHY_DISPATCHER (object);
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  GPtrArray *requestable_channels;
  GPtrArray *existing_channels;

  if (error) {
    DEBUG ("Error: %s", error->message);
    return;
  }

  requestable_channels = tp_asv_get_boxed (properties,
    "RequestableChannelClasses", TP_ARRAY_TYPE_REQUESTABLE_CHANNEL_CLASS_LIST);

  if (requestable_channels == NULL)
    DEBUG ("No RequestableChannelClasses property !?! on connection");
  else
    {
      ConnectionData *cd;
      GList *requests, *l;
      FindChannelRequest *request;
      GList *retval;

      cd = g_hash_table_lookup (priv->connections, proxy);
      g_assert (cd != NULL);

      cd->requestable_channels = g_boxed_copy (
        TP_ARRAY_TYPE_REQUESTABLE_CHANNEL_CLASS_LIST, requestable_channels);

      requests = g_hash_table_lookup (priv->outstanding_classes_requests,
          proxy);

      for (l = requests; l != NULL; l = l->next)
        {
          request = l->data;

          retval = empathy_dispatcher_find_channel_classes (self,
              TP_CONNECTION (proxy), request->channel_type,
              request->handle_type, request->properties);
          request->callback (retval, request->user_data);

          free_find_channel_request (request);
          g_list_free (retval);
        }

      g_list_free (requests);

      g_hash_table_remove (priv->outstanding_classes_requests, proxy);
    }

  existing_channels = tp_asv_get_boxed (properties,
      "Channels", TP_ARRAY_TYPE_CHANNEL_DETAILS_LIST);

  if (existing_channels != NULL)
    {
      guint idx;

      for (idx = 0; idx < existing_channels->len; idx++)
        {
          GValueArray *values = g_ptr_array_index (existing_channels, idx);
          const gchar *object_path;
          GHashTable *props;

          object_path = g_value_get_boxed (g_value_array_get_nth (values, 0));
          props = g_value_get_boxed (g_value_array_get_nth (values, 1));

          if (tp_strdiff (tp_asv_get_string (props,
                      TP_IFACE_CHANNEL ".ChannelType"),
                  TP_IFACE_CHANNEL_TYPE_TEXT))
            continue;

          dispatcher_connection_new_channel_with_properties (self,
              TP_CONNECTION (proxy), object_path, props);
        }
    }
}

static void
dispatcher_connection_advertise_capabilities_cb (TpConnection    *connection,
                                                 const GPtrArray *capabilities,
                                                 const GError    *error,
                                                 gpointer         user_data,
                                                 GObject         *self)
{
  if (error)
    DEBUG ("Error: %s", error->message);
}

static void
dispatcher_init_connection_if_needed (EmpathyDispatcher *self,
    TpConnection *connection)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  GPtrArray   *capabilities;
  GType        cap_type;
  GValue       cap = {0, };
  const gchar *remove_ = NULL;

  if (g_hash_table_lookup (priv->connections, connection) != NULL)
    return;

  g_hash_table_insert (priv->connections, g_object_ref (connection),
    new_connection_data ());

  g_signal_connect (connection, "invalidated",
    G_CALLBACK (dispatcher_connection_invalidated_cb), self);

  if (tp_proxy_has_interface_by_id (TP_PROXY (connection),
      TP_IFACE_QUARK_CONNECTION_INTERFACE_REQUESTS))
    {
      tp_cli_dbus_properties_call_get_all (connection, -1,
        TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
        dispatcher_connection_got_all,
        NULL, NULL, G_OBJECT (self));
    }

  /* Advertise VoIP capabilities */
  capabilities = g_ptr_array_sized_new (1);
  cap_type = dbus_g_type_get_struct ("GValueArray", G_TYPE_STRING,
    G_TYPE_UINT, G_TYPE_INVALID);
  g_value_init (&cap, cap_type);
  g_value_take_boxed (&cap, dbus_g_type_specialized_construct (cap_type));
  dbus_g_type_struct_set (&cap,
        0, TP_IFACE_CHANNEL_TYPE_STREAMED_MEDIA,
        1, TP_CHANNEL_MEDIA_CAPABILITY_AUDIO |
           TP_CHANNEL_MEDIA_CAPABILITY_VIDEO |
           TP_CHANNEL_MEDIA_CAPABILITY_NAT_TRAVERSAL_STUN  |
           TP_CHANNEL_MEDIA_CAPABILITY_NAT_TRAVERSAL_GTALK_P2P |
           TP_CHANNEL_MEDIA_CAPABILITY_NAT_TRAVERSAL_ICE_UDP,
           G_MAXUINT);
  g_ptr_array_add (capabilities, g_value_get_boxed (&cap));

  tp_cli_connection_interface_capabilities_call_advertise_capabilities (
    connection, -1, capabilities, &remove_,
    dispatcher_connection_advertise_capabilities_cb,
    NULL, NULL, G_OBJECT (self));

  g_value_unset (&cap);
  g_ptr_array_free (capabilities, TRUE);
}

static void
dispatcher_new_connection_cb (EmpathyAccountManager *manager,
                              TpConnection *connection,
                              EmpathyDispatcher *self)
{
  dispatcher_init_connection_if_needed (self, connection);
}

static void
remove_idle_handlers (gpointer key,
                      gpointer value,
                      gpointer user_data)
{
  guint source_id;

  source_id = GPOINTER_TO_UINT (value);
  g_source_remove (source_id);
}

static GObject *
dispatcher_constructor (GType type,
                        guint n_construct_params,
                        GObjectConstructParam *construct_params)
{
  GObject *retval;
  EmpathyDispatcherPriv *priv;

  if (dispatcher != NULL)
    return g_object_ref (dispatcher);

  retval = G_OBJECT_CLASS (empathy_dispatcher_parent_class)->constructor
    (type, n_construct_params, construct_params);

  dispatcher = EMPATHY_DISPATCHER (retval);
  g_object_add_weak_pointer (retval, (gpointer) &dispatcher);

  priv = GET_PRIV (dispatcher);

  if (priv->handler == NULL)
    priv->handler = empathy_handler_new (NULL, NULL, NULL);

  empathy_handler_set_handle_channels_func (priv->handler,
    empathy_dispatcher_handle_channels,
    dispatcher);

  empathy_handler_set_channels_func (priv->handler,
    empathy_dispatcher_get_channels,
    dispatcher);

  return retval;
}

static void
dispatcher_dispose (GObject *object)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (object);
  GHashTableIter iter;
  gpointer connection;
  GList *l;

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  for (l = priv->handlers ; l != NULL; l = g_list_next (l))
    g_object_unref (G_OBJECT (l->data));

  g_list_free (priv->handlers);
  priv->handlers = NULL;

  if (priv->handler != NULL)
    g_object_unref (priv->handler);
  priv->handler = NULL;

  g_hash_table_iter_init (&iter, priv->connections);
  while (g_hash_table_iter_next (&iter, &connection, NULL))
    {
      g_signal_handlers_disconnect_by_func (connection,
          dispatcher_connection_invalidated_cb, object);
    }

  g_hash_table_destroy (priv->connections);
  priv->connections = NULL;

  G_OBJECT_CLASS (empathy_dispatcher_parent_class)->dispose (object);
}

static void
dispatcher_finalize (GObject *object)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (object);
  GList *l;
  GHashTableIter iter;
  gpointer connection;
  GList *list;

  if (priv->request_channel_class_async_ids != NULL)
    {
      g_hash_table_foreach (priv->request_channel_class_async_ids,
        remove_idle_handlers, NULL);
      g_hash_table_destroy (priv->request_channel_class_async_ids);
    }

  g_signal_handlers_disconnect_by_func (priv->account_manager,
      dispatcher_new_connection_cb, object);

  for (l = priv->channels; l; l = l->next)
    {
      g_signal_handlers_disconnect_by_func (l->data,
          dispatcher_channel_invalidated_cb, object);
    }

  g_list_free (priv->channels);

  g_hash_table_iter_init (&iter, priv->outstanding_classes_requests);
  while (g_hash_table_iter_next (&iter, &connection, (gpointer *) &list))
    {
      g_list_foreach (list, (GFunc) free_find_channel_request, NULL);
      g_list_free (list);
    }

  g_object_unref (priv->account_manager);

  g_hash_table_destroy (priv->outstanding_classes_requests);
}

static void
dispatcher_set_property (GObject *object,
  guint property_id,
  const GValue *value,
  GParamSpec *pspec)
{
  EmpathyDispatcher *self = EMPATHY_DISPATCHER (object);
  EmpathyDispatcherPriv *priv = GET_PRIV (self);

  switch (property_id)
    {
      case PROP_HANDLER:
        priv->handler = g_value_dup_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
dispatcher_get_property (GObject *object,
  guint property_id,
  GValue *value,
  GParamSpec *pspec)
{
  EmpathyDispatcher *self = EMPATHY_DISPATCHER (object);
  EmpathyDispatcherPriv *priv = GET_PRIV (self);

  switch (property_id)
    {
      case PROP_HANDLER:
        g_value_set_object (value, priv->handler);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
empathy_dispatcher_class_init (EmpathyDispatcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  object_class->dispose = dispatcher_dispose;
  object_class->finalize = dispatcher_finalize;
  object_class->constructor = dispatcher_constructor;

  object_class->get_property = dispatcher_get_property;
  object_class->set_property = dispatcher_set_property;

  param_spec = g_param_spec_object ("handler", "handler",
    "The main Telepathy Client Hander object",
    EMPATHY_TYPE_HANDLER,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class,
    PROP_HANDLER, param_spec);

  signals[OBSERVE] =
    g_signal_new ("observe",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE,
      1, EMPATHY_TYPE_DISPATCH_OPERATION);

  signals[APPROVE] =
    g_signal_new ("approve",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE,
      1, EMPATHY_TYPE_DISPATCH_OPERATION);

  signals[DISPATCH] =
    g_signal_new ("dispatch",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE,
      1, EMPATHY_TYPE_DISPATCH_OPERATION);


  g_type_class_add_private (object_class, sizeof (EmpathyDispatcherPriv));
}

static void
empathy_dispatcher_init (EmpathyDispatcher *self)
{
  GList *connections, *l;
  EmpathyDispatcherPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
    EMPATHY_TYPE_DISPATCHER, EmpathyDispatcherPriv);

  self->priv = priv;
  priv->account_manager = empathy_account_manager_dup_singleton ();

  g_signal_connect (priv->account_manager, "new-connection",
    G_CALLBACK (dispatcher_new_connection_cb),
    self);

  priv->connections = g_hash_table_new_full (g_direct_hash, g_direct_equal,
    g_object_unref, (GDestroyNotify) free_connection_data);

  priv->outstanding_classes_requests = g_hash_table_new_full (g_direct_hash,
    g_direct_equal, g_object_unref, NULL);

  priv->channels = NULL;

  connections = empathy_account_manager_dup_connections (
      priv->account_manager);
  for (l = connections; l; l = l->next)
    {
      dispatcher_new_connection_cb (priv->account_manager, l->data,
          self);
      g_object_unref (l->data);
    }
  g_list_free (connections);

  priv->request_channel_class_async_ids = g_hash_table_new (g_direct_hash,
    g_direct_equal);
}

EmpathyDispatcher *
empathy_dispatcher_new (const gchar *name,
  GPtrArray *filters,
  GStrv capabilities)
{
  EmpathyHandler *handler;
  EmpathyDispatcher *ret;

  g_assert (dispatcher == NULL);
  handler = empathy_handler_new (name, filters, capabilities);

  ret = EMPATHY_DISPATCHER (
    g_object_new (EMPATHY_TYPE_DISPATCHER,
      "handler", handler,
      NULL));
  g_object_unref (handler);

  return ret;
}

EmpathyDispatcher *
empathy_dispatcher_dup_singleton (void)
{
  return EMPATHY_DISPATCHER (g_object_new (EMPATHY_TYPE_DISPATCHER, NULL));
}

static void
dispatcher_request_failed (EmpathyDispatcher *self,
                           DispatcherRequestData *request_data,
                           const GError *error)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  ConnectionData *conn_data;

  conn_data = g_hash_table_lookup (priv->connections,
      request_data->connection);
  if (request_data->cb != NULL)
    request_data->cb (NULL, error, request_data->user_data);

  if (conn_data != NULL)
    {
      conn_data->outstanding_requests =
          g_list_remove (conn_data->outstanding_requests, request_data);
    }
  /* else Connection has been invalidated */

  free_dispatcher_request_data (request_data);
}

static void
dispatcher_connection_new_requested_channel (EmpathyDispatcher *self,
  DispatcherRequestData *request_data,
  const gchar *object_path,
  GHashTable *properties,
  const GError *error)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  EmpathyDispatchOperation *operation = NULL;
  ConnectionData *conn_data;

  conn_data = g_hash_table_lookup (priv->connections,
    request_data->connection);

  if (error)
    {
      DEBUG ("Channel request failed: %s", error->message);

      dispatcher_request_failed (self, request_data, error);

      goto out;
    }

  operation = g_hash_table_lookup (conn_data->outstanding_channels,
    object_path);

  if (operation != NULL)
    g_hash_table_remove (conn_data->outstanding_channels, object_path);
  else
    operation = g_hash_table_lookup (conn_data->dispatching_channels,
        object_path);

  if (operation == NULL)
    {
      DispatchData *data = g_hash_table_lookup (conn_data->dispatched_channels,
        object_path);

      if (data != NULL)
        {
          operation = empathy_dispatch_operation_new_with_wrapper (
            request_data->connection,
            data->channel, request_data->contact, FALSE,
            data->channel_wrapper);
        }
      else
        {
          TpChannel *channel;

          if (properties != NULL)
            channel = tp_channel_new_from_properties (request_data->connection,
              object_path, properties, NULL);
          else
            channel = tp_channel_new (request_data->connection, object_path,
              request_data->channel_type, request_data->handle_type,
              request_data->handle, NULL);

          g_signal_connect (channel, "invalidated",
            G_CALLBACK (dispatcher_channel_invalidated_cb),
            request_data->dispatcher);

          priv->channels = g_list_prepend (priv->channels, channel);

          operation = empathy_dispatch_operation_new (request_data->connection,
             channel, request_data->contact, FALSE);
          g_object_unref (channel);
        }
    }
  else
    {
      /* Already existed set potential extra information */
      g_object_set (G_OBJECT (operation),
        "contact", request_data->contact,
        NULL);
    }

  request_data->operation = operation;

  /* (pre)-approve this right away as we requested it
   * This might cause the channel to be claimed, in which case the operation
   * will disappear. So ref it, and check the status before starting the
   * dispatching */

  g_object_ref (operation);
  empathy_dispatch_operation_approve (operation);

   if (empathy_dispatch_operation_get_status (operation) <
     EMPATHY_DISPATCHER_OPERATION_STATE_APPROVING)
      dispatcher_start_dispatching (request_data->dispatcher, operation,
          conn_data);

  g_object_unref (operation);

out:
  dispatcher_flush_outstanding_operations (request_data->dispatcher,
    conn_data);
}

static void
dispatcher_request_channel_cb (TpConnection *connection,
                               const gchar  *object_path,
                               const GError *error,
                               gpointer user_data,
                               GObject *weak_object)
{
  DispatcherRequestData *request_data = (DispatcherRequestData *) user_data;
  EmpathyDispatcher *self =
      EMPATHY_DISPATCHER (request_data->dispatcher);

  request_data->pending_call = NULL;

  dispatcher_connection_new_requested_channel (self,
    request_data, object_path, NULL, error);
}

static void
dispatcher_request_channel (DispatcherRequestData *request_data)
{
  if (tp_proxy_has_interface_by_id (TP_PROXY (request_data->connection),
      TP_IFACE_QUARK_CONNECTION_INTERFACE_REQUESTS))
    {
      /* Extend the request_data to be a valid request */
      g_assert (request_data->request == NULL);
      request_data->request = tp_asv_new (
        TP_IFACE_CHANNEL ".ChannelType",
          G_TYPE_STRING, request_data->channel_type,
        TP_IFACE_CHANNEL ".TargetHandleType",
          G_TYPE_UINT, request_data->handle_type,
        NULL);

      if (request_data->handle_type != TP_HANDLE_TYPE_NONE)
        tp_asv_set_uint32 (request_data->request,
          TP_IFACE_CHANNEL ".TargetHandle", request_data->handle);

      empathy_dispatcher_call_create_or_ensure_channel (
        request_data->dispatcher, request_data);
    }
  else
    {
      request_data->pending_call = tp_cli_connection_call_request_channel (
        request_data->connection, -1,
        request_data->channel_type,
        request_data->handle_type,
        request_data->handle,
        TRUE, dispatcher_request_channel_cb,
        request_data, NULL, NULL);
    }
}

void
empathy_dispatcher_chat_with_contact (EmpathyContact *contact,
                                      EmpathyDispatcherRequestCb *callback,
                                      gpointer user_data)
{
  EmpathyDispatcher *self;
  EmpathyDispatcherPriv *priv;
  TpConnection *connection;
  ConnectionData *connection_data;
  DispatcherRequestData *request_data;

  g_return_if_fail (EMPATHY_IS_CONTACT (contact));

  self = empathy_dispatcher_dup_singleton ();
  priv = GET_PRIV (self);

  connection = empathy_contact_get_connection (contact);
  connection_data = g_hash_table_lookup (priv->connections, connection);

  /* The contact handle might not be known yet */
  request_data = new_dispatcher_request_data (self, connection,
    TP_IFACE_CHANNEL_TYPE_TEXT, TP_HANDLE_TYPE_CONTACT,
    empathy_contact_get_handle (contact), NULL, contact, callback, user_data);
  request_data->should_ensure = TRUE;

  connection_data->outstanding_requests = g_list_prepend
    (connection_data->outstanding_requests, request_data);

  dispatcher_request_channel (request_data);

  g_object_unref (self);
}

typedef struct
{
  EmpathyDispatcher *dispatcher;
  EmpathyDispatcherRequestCb *callback;
  gpointer user_data;
} ChatWithContactIdData;

static void
dispatcher_chat_with_contact_id_cb (EmpathyTpContactFactory *factory,
                                    EmpathyContact          *contact,
                                    const GError            *error,
                                    gpointer                 user_data,
                                    GObject                 *weak_object)
{
  ChatWithContactIdData *data = user_data;

  if (error)
    {
      /* FIXME: Should call data->callback with the error */
      DEBUG ("Error: %s", error->message);
    }
  else
    {
      empathy_dispatcher_chat_with_contact (contact, data->callback,
          data->user_data);
    }

  g_object_unref (data->dispatcher);
  g_slice_free (ChatWithContactIdData, data);
}

void
empathy_dispatcher_chat_with_contact_id (TpConnection *connection,
                                         const gchar *contact_id,
                                         EmpathyDispatcherRequestCb *callback,
                                         gpointer user_data)
{
  EmpathyDispatcher *self;
  EmpathyTpContactFactory *factory;
  ChatWithContactIdData *data;

  g_return_if_fail (TP_IS_CONNECTION (connection));
  g_return_if_fail (!EMP_STR_EMPTY (contact_id));

  self = empathy_dispatcher_dup_singleton ();
  factory = empathy_tp_contact_factory_dup_singleton (connection);
  data = g_slice_new0 (ChatWithContactIdData);
  data->dispatcher = self;
  data->callback = callback;
  data->user_data = user_data;
  empathy_tp_contact_factory_get_from_id (factory, contact_id,
      dispatcher_chat_with_contact_id_cb, data, NULL, NULL);

  g_object_unref (factory);
}

static void
dispatcher_request_handles_cb (TpConnection *connection,
                               const GArray *handles,
                               const GError *error,
                               gpointer user_data,
                               GObject *object)
{
  DispatcherRequestData *request_data = (DispatcherRequestData *) user_data;

  request_data->pending_call = NULL;

  if (error != NULL)
    {
      EmpathyDispatcher *self = request_data->dispatcher;
      EmpathyDispatcherPriv *priv = GET_PRIV (self);
      ConnectionData *cd;

      cd = g_hash_table_lookup (priv->connections, request_data->connection);

      if (request_data->cb)
        request_data->cb (NULL, error, request_data->user_data);

      cd->outstanding_requests = g_list_remove (cd->outstanding_requests,
        request_data);

      free_dispatcher_request_data (request_data);

      dispatcher_flush_outstanding_operations (self, cd);
      return;
    }

  request_data->handle = g_array_index (handles, guint, 0);
  dispatcher_request_channel (request_data);
}

void
empathy_dispatcher_join_muc (TpConnection *connection,
                             const gchar *roomname,
                             EmpathyDispatcherRequestCb *callback,
                             gpointer user_data)
{
  EmpathyDispatcher *self;
  EmpathyDispatcherPriv *priv;
  DispatcherRequestData *request_data;
  ConnectionData *connection_data;
  const gchar *names[] = { roomname, NULL };

  g_return_if_fail (TP_IS_CONNECTION (connection));
  g_return_if_fail (!EMP_STR_EMPTY (roomname));

  self = empathy_dispatcher_dup_singleton ();
  priv = GET_PRIV (self);

  connection_data = g_hash_table_lookup (priv->connections, connection);

  /* Don't know the room handle yet */
  request_data  = new_dispatcher_request_data (self, connection,
    TP_IFACE_CHANNEL_TYPE_TEXT, TP_HANDLE_TYPE_ROOM, 0, NULL,
    NULL, callback, user_data);

  connection_data->outstanding_requests = g_list_prepend
    (connection_data->outstanding_requests, request_data);

  request_data->pending_call = tp_cli_connection_call_request_handles (
    connection, -1,
    TP_HANDLE_TYPE_ROOM, names,
    dispatcher_request_handles_cb, request_data, NULL, NULL);

  g_object_unref (self);
}

static void
dispatcher_create_channel_cb (TpConnection *connect,
                              const gchar *object_path,
                              GHashTable *properties,
                              const GError *error,
                              gpointer user_data,
                              GObject *weak_object)
{
  DispatcherRequestData *request_data = (DispatcherRequestData *) user_data;
  EmpathyDispatcher *self =
      EMPATHY_DISPATCHER (request_data->dispatcher);

  request_data->pending_call = NULL;

  dispatcher_connection_new_requested_channel (self,
    request_data, object_path, properties, error);
}

static void
dispatcher_ensure_channel_cb (TpConnection *connect,
                              gboolean is_ours,
                              const gchar *object_path,
                              GHashTable *properties,
                              const GError *error,
                              gpointer user_data,
                              GObject *weak_object)
{
  DispatcherRequestData *request_data = (DispatcherRequestData *) user_data;
  EmpathyDispatcher *self =
      EMPATHY_DISPATCHER (request_data->dispatcher);

  request_data->pending_call = NULL;

  dispatcher_connection_new_requested_channel (self,
    request_data, object_path, properties, error);
}

static void
empathy_dispatcher_call_create_or_ensure_channel (
    EmpathyDispatcher *self,
    DispatcherRequestData *request_data)
{
  if (request_data->should_ensure)
    {
      request_data->pending_call =
          tp_cli_connection_interface_requests_call_ensure_channel (
          request_data->connection, -1,
          request_data->request, dispatcher_ensure_channel_cb,
          request_data, NULL, NULL);
    }
  else
    {
      request_data->pending_call =
          tp_cli_connection_interface_requests_call_create_channel (
          request_data->connection, -1,
          request_data->request, dispatcher_create_channel_cb,
          request_data, NULL, NULL);
    }
}

void
empathy_dispatcher_create_channel (EmpathyDispatcher *self,
                                   TpConnection *connection,
                                   GHashTable *request,
                                   EmpathyDispatcherRequestCb *callback,
                                   gpointer user_data)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  ConnectionData *connection_data;
  DispatcherRequestData *request_data;
  const gchar *channel_type;
  guint handle_type;
  guint handle;
  gboolean valid;

  g_return_if_fail (EMPATHY_IS_DISPATCHER (self));
  g_return_if_fail (TP_IS_CONNECTION (connection));
  g_return_if_fail (request != NULL);

  connection_data = g_hash_table_lookup (priv->connections, connection);
  g_assert (connection_data != NULL);

  channel_type = tp_asv_get_string (request, TP_IFACE_CHANNEL ".ChannelType");

  handle_type = tp_asv_get_uint32 (request,
    TP_IFACE_CHANNEL ".TargetHandleType", &valid);
  if (!valid)
    handle_type = TP_UNKNOWN_HANDLE_TYPE;

  handle = tp_asv_get_uint32 (request, TP_IFACE_CHANNEL ".TargetHandle", NULL);

  request_data  = new_dispatcher_request_data (self, connection,
    channel_type, handle_type, handle, request,
    NULL, callback, user_data);

  connection_data->outstanding_requests = g_list_prepend
    (connection_data->outstanding_requests, request_data);

  empathy_dispatcher_call_create_or_ensure_channel (self, request_data);
}

static gboolean
channel_class_matches (GValueArray *class,
                       const char *channel_type,
                       guint handle_type,
                       GArray *fixed_properties)
{
  GHashTable *fprops;
  GValue *v;
  const char *c_type;
  guint h_type;
  gboolean valid;

  v = g_value_array_get_nth (class, 0);

  /* if the class doesn't match channel type discard it. */
  fprops = g_value_get_boxed (v);
  c_type = tp_asv_get_string (fprops, TP_IFACE_CHANNEL ".ChannelType");

  if (tp_strdiff (channel_type, c_type))
    return FALSE;

  /* we have the right channel type, see if the handle type matches */
  h_type = tp_asv_get_uint32 (fprops,
                              TP_IFACE_CHANNEL ".TargetHandleType", &valid);

  if (!valid || handle_type != h_type)
    return FALSE;

  if (fixed_properties != NULL)
    {
      gpointer h_key, h_val;
      guint idx;
      GHashTableIter iter;
      gboolean found;

      g_hash_table_iter_init (&iter, fprops);

      while (g_hash_table_iter_next (&iter, &h_key, &h_val))
        {
          /* discard ChannelType and TargetHandleType, as we already
           * checked them.
           */
          if (!tp_strdiff ((char *) h_key, TP_IFACE_CHANNEL ".ChannelType") ||
              !tp_strdiff
                ((char *) h_key, TP_IFACE_CHANNEL ".TargetHandleType"))
            continue;

          found = FALSE;

          for (idx = 0; idx < fixed_properties->len; idx++)
            {
              /* if |key| doesn't exist in |fixed_properties|, discard
               * the class.
               */
              if (!tp_strdiff
                    ((char *) h_key,
                     g_array_index (fixed_properties, char *, idx)))
                {
                  found = TRUE;
                  /* exit the for() loop */
                  break;
                }
            }

          if (!found)
            return FALSE;
        }
    }
  else
    {
      /* if no fixed_properties are specified, discard the classes
       * with some fixed properties other than the two we already
       * checked.
       */
      if (g_hash_table_size (fprops) > 2)
        return FALSE;
    }

  return TRUE;
}

static GList *
empathy_dispatcher_find_channel_classes (EmpathyDispatcher *self,
                                         TpConnection *connection,
                                         const gchar *channel_type,
                                         guint handle_type,
                                         GArray *fixed_properties)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  GValueArray *class;
  GPtrArray *classes;
  GList *matching_classes;
  guint i;
  ConnectionData *cd;

  g_return_val_if_fail (channel_type != NULL, NULL);
  g_return_val_if_fail (handle_type != 0, NULL);

  cd = g_hash_table_lookup (priv->connections, connection);

  if (cd == NULL)
    return NULL;

  classes = cd->requestable_channels;
  if (classes == NULL)
    return NULL;

  matching_classes = NULL;

  for (i = 0; i < classes->len; i++)
    {
      class = g_ptr_array_index (classes, i);

      if (!channel_class_matches
          (class, channel_type, handle_type, fixed_properties))
        continue;

      matching_classes = g_list_prepend (matching_classes, class);
    }

  return matching_classes;
}

static gboolean
find_channel_class_idle_cb (gpointer user_data)
{
  GList *retval;
  GList *requests;
  FindChannelRequest *request = user_data;
  ConnectionData *cd;
  gboolean is_ready = TRUE;
  EmpathyDispatcherPriv *priv = GET_PRIV (request->dispatcher);

  g_hash_table_remove (priv->request_channel_class_async_ids, request);

  cd = g_hash_table_lookup (priv->connections, request->connection);

  if (cd == NULL)
    is_ready = FALSE;
  else if (cd->requestable_channels == NULL)
    is_ready = FALSE;

  if (is_ready)
    {
      retval = empathy_dispatcher_find_channel_classes (request->dispatcher,
          request->connection, request->channel_type, request->handle_type,
          request->properties);

      request->callback (retval, request->user_data);
      free_find_channel_request (request);
      g_list_free (retval);

      return FALSE;
    }

  requests = g_hash_table_lookup (priv->outstanding_classes_requests,
      request->connection);
  requests = g_list_prepend (requests, request);

  g_hash_table_insert (priv->outstanding_classes_requests,
      request->connection, requests);

  return FALSE;
}

static GArray *
setup_varargs (va_list var_args,
               const char *channel_namespace,
               const char *first_property_name)
{
  const char *name;
  char *name_full;
  GArray *properties;

  if (first_property_name == NULL)
    return NULL;

  name = first_property_name;
  properties = g_array_new (TRUE, TRUE, sizeof (char *));

  while (name != NULL)
    {
      name_full = g_strdup (name);
      properties = g_array_append_val (properties, name_full);
      name = va_arg (var_args, char *);
    }

  return properties;
}

/**
 * empathy_dispatcher_find_requestable_channel_classes:
 * @dispatcher: an #EmpathyDispatcher
 * @connection: a #TpConnection
 * @channel_type: a string identifying the type of the channel to lookup
 * @handle_type: the handle type for the channel
 * @first_property_name: %NULL, or the name of the first fixed property,
 * followed optionally by more names, followed by %NULL.
 *
 * Returns all the channel classes that a client can request for the connection
 * @connection, of the type identified by @channel_type, @handle_type and the
 * fixed properties list.
 * The classes which are compatible with a fixed properties list (i.e. those
 * that will be returned by this function) are intended as those that do not
 * contain any fixed property other than those in the list; note that this
 * doesn't guarantee that all the classes compatible with the list will contain
 * all the requested fixed properties, so the clients will have to filter
 * the returned list themselves.
 * If @first_property_name is %NULL, only the classes with no other fixed
 * properties than ChannelType and TargetHandleType will be returned.
 * Note that this function may return %NULL without performing any lookup if
 * @connection is not ready. To ensure that @connection is always ready,
 * use the empathy_dispatcher_find_requestable_channel_classes_async() variant.
 *
 * Return value: a #GList of #GValueArray objects, where the first element in
 * the array is a #GHashTable of the fixed properties, and the second is
 * a #GStrv of the allowed properties for the class. The list should be free'd
 * with g_list_free() when done, but the objects inside the list are owned
 * by the #EmpathyDispatcher and must not be modified.
 */
GList *
empathy_dispatcher_find_requestable_channel_classes
                                 (EmpathyDispatcher *self,
                                  TpConnection *connection,
                                  const gchar *channel_type,
                                  guint handle_type,
                                  const char *first_property_name,
                                  ...)
{
  va_list var_args;
  GArray *properties;
  EmpathyDispatcherPriv *priv;
  GList *retval;
  guint idx;
  char *str;

  g_return_val_if_fail (EMPATHY_IS_DISPATCHER (self), NULL);
  g_return_val_if_fail (TP_IS_CONNECTION (connection), NULL);
  g_return_val_if_fail (channel_type != NULL, NULL);
  g_return_val_if_fail (handle_type != 0, NULL);

  priv = GET_PRIV (self);

  va_start (var_args, first_property_name);

  properties = setup_varargs (var_args, channel_type, first_property_name);

  va_end (var_args);

  retval = empathy_dispatcher_find_channel_classes (self, connection,
    channel_type, handle_type, properties);

  if (properties != NULL)
    {
      /* free the properties array */
      for (idx = 0; idx < properties->len ; idx++)
        {
          str = g_array_index (properties, char *, idx);
          g_free (str);
        }

      g_array_free (properties, TRUE);
    }

  return retval;
}

/**
 * empathy_dispatcher_find_requestable_channel_classes_async:
 * @dispatcher: an #EmpathyDispatcher
 * @connection: a #TpConnection
 * @channel_type: a string identifying the type of the channel to lookup
 * @handle_type: the handle type for the channel
 * @callback: the callback to call when @connection is ready
 * @user_data: the user data to pass to @callback
 * @first_property_name: %NULL, or the name of the first fixed property,
 * followed optionally by more names, followed by %NULL.
 *
 * Please see the documentation of
 * empathy_dispatcher_find_requestable_channel_classes() for a detailed
 * description of this function.
 */
void
empathy_dispatcher_find_requestable_channel_classes_async
                                 (EmpathyDispatcher *self,
                                  TpConnection *connection,
                                  const gchar *channel_type,
                                  guint handle_type,
                                  EmpathyDispatcherFindChannelClassCb callback,
                                  gpointer user_data,
                                  const char *first_property_name,
                                  ...)
{
  va_list var_args;
  GArray *properties;
  FindChannelRequest *request;
  EmpathyDispatcherPriv *priv;
  guint source_id;

  g_return_if_fail (EMPATHY_IS_DISPATCHER (self));
  g_return_if_fail (TP_IS_CONNECTION (connection));
  g_return_if_fail (channel_type != NULL);
  g_return_if_fail (handle_type != 0);

  priv = GET_PRIV (self);

  va_start (var_args, first_property_name);

  properties = setup_varargs (var_args, channel_type, first_property_name);

  va_end (var_args);

  /* append another request for this connection */
  request = g_slice_new0 (FindChannelRequest);
  request->dispatcher = g_object_ref (self);
  request->channel_type = g_strdup (channel_type);
  request->handle_type = handle_type;
  request->connection = connection;
  request->callback = callback;
  request->user_data = user_data;
  request->properties = properties;

  source_id = g_idle_add (find_channel_class_idle_cb, request);

  g_hash_table_insert (priv->request_channel_class_async_ids,
    request, GUINT_TO_POINTER (source_id));
}

static GList *
empathy_dispatcher_get_channels (EmpathyHandler *handler,
  gpointer user_data)
{
  EmpathyDispatcher *self = EMPATHY_DISPATCHER (user_data);
  EmpathyDispatcherPriv *priv = GET_PRIV (self);

  return priv->channels;
}

static gboolean
empathy_dispatcher_handle_channels (EmpathyHandler *handler,
    const gchar *account_path,
    const gchar *connection_path,
    const GPtrArray *channels,
    const GPtrArray *requests_satisfied,
    guint64 timestamp,
    GHashTable *handler_info,
    gpointer user_data,
    GError **error)
{
  EmpathyDispatcher *self = EMPATHY_DISPATCHER (user_data);
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  guint i;
  EmpathyAccount *account;
  TpConnection *connection;

  account = empathy_account_manager_ensure_account (priv->account_manager,
    account_path);
  g_assert (account != NULL);

  connection = empathy_account_get_connection_for_path (account,
      connection_path);
  if (connection == NULL)
    {
      g_set_error_literal (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
        "Invalid connection argument");
      return FALSE;
    }

  for (i = 0; i < channels->len ; i++)
    {
      GValueArray *arr = g_ptr_array_index (channels, i);
      const gchar *object_path;
      GHashTable *properties;

      object_path = g_value_get_boxed (g_value_array_get_nth (arr, 0));
      properties = g_value_get_boxed (g_value_array_get_nth (arr, 1));

      dispatcher_connection_new_channel_with_properties (self,
        connection, object_path, properties);
    }

  return TRUE;
}


EmpathyHandler *
empathy_dispatcher_add_handler (EmpathyDispatcher *self,
    const gchar *name,
    GPtrArray *filters,
    GStrv capabilities)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  EmpathyHandler *handler;

  handler = empathy_handler_new (name, filters, capabilities);
  priv->handlers = g_list_prepend (priv->handlers, handler);

  /* Only set the handle_channels function, the Channel property on the main
   * handler will always report all dispatched channels even if they came from
   * a different Handler */
  empathy_handler_set_handle_channels_func (handler,
    empathy_dispatcher_handle_channels, self);

  return handler;
}

void
empathy_dispatcher_remove_handler (EmpathyDispatcher *self,
  EmpathyHandler *handler)
{
  EmpathyDispatcherPriv *priv = GET_PRIV (self);
  GList *h;

  h = g_list_find (priv->handlers, handler);
  g_return_if_fail (h != NULL);

  priv->handlers = g_list_delete_link (priv->handlers, h);

  g_object_unref (handler);
}
