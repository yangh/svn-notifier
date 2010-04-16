#include "svn-notify.h"
#include "libnotify/notify.h"
#include "libnotify/notification.h"

static void closed_callback (NotifyNotification *notification,
                             gpointer            user_data);  

void
svn_notify_entry (const SvnLogEntry *entry,
                  const char *path,
                  int delay)
{
    g_return_if_fail (entry != NULL);

    if (! notify_is_initted()) notify_init("svn-notify");

    gchar *sum, *body;
    NotifyNotification* note;


    sum = g_strdup_printf ("%s * %ld",
                svn_log_entry_get_author(entry),
                svn_log_entry_get_revision(entry)
                );
    body = g_strdup_printf ("  %s",
                svn_log_entry_get_full_log(entry));

    note = notify_notification_new (sum, body, "gtk-about", NULL);

    g_signal_connect (note, "closed", G_CALLBACK(closed_callback), NULL);
    notify_notification_set_timeout (note, delay);
    notify_notification_show (note, NULL);

    g_free (sum);
    g_free (body);
}

static void
closed_callback (NotifyNotification *notification,
                 gpointer            user_data)
{
    g_object_unref (notification);
}


