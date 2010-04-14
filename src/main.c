#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "libgsvn/svn-log-command.h"
#include "libgsvn/svn-log-entry.h"
#include "libanjuta/anjuta-command.h"
#include "libanjuta/anjuta-async-command.h"

void log_arrived (SvnLogCommand *cmd)
{
    GQueue *entries = NULL;
    SvnLogEntry *entry = NULL;

    //anjuta_async_command_lock(cmd);
    entries = svn_log_command_get_entry_queue(cmd);
    while(! g_queue_is_empty (entries)) {
        entry = g_queue_pop_tail(entries);
        g_print (" r%ld | %8s | %s | %s\n",
                svn_log_entry_get_revision(entry),
                svn_log_entry_get_author(entry),
                svn_log_entry_get_date(entry),
                svn_log_entry_get_short_log(entry));
        svn_log_entry_destroy (entry);
    }
    //anjuta_async_command_unlock(cmd);
}

gboolean query_log (SvnLogCommand *log)
{
    anjuta_command_start(log);
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf ("Usage: %s <path> <start_rev> <query-delay>\n", argv[0]);
        exit(1);
    }

    if (!g_thread_supported ()) g_thread_init (NULL);

    apr_initialize ();

    gtk_init (&argc, &argv);

    const char *path = argv[1];
    gulong rev = atoi(argv[2]);
    gint delay = atoi(argv[3]);

    SvnLogCommand *log = svn_log_command_new (path);

    svn_log_command_set_start_rev (log, rev);

    g_signal_connect (log, "data-arrived", (GCallback) log_arrived, NULL);

    anjuta_command_start(log);

    g_timeout_add (1000 * delay, (GSourceFunc) query_log, log);

    gtk_main ();

    apr_terminate ();

    return 0;
}
