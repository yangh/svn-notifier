#include <stdlib.h>
#include <gtk/gtk.h>

#include "libgsvn/svn-log-command.h"
#include "libgsvn/svn-log-entry.h"
#include "libanjuta/anjuta-command.h"
#include "libanjuta/anjuta-async-command.h"

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
static gboolean query_running = FALSE;

static const char *path = NULL;
static gulong last_rev = 1;
static gint delay = 5;

static void log_arrived (SvnLogCommand *cmd);
static void log_finished (SvnLogCommand *cmd, guint return_code);
static gboolean query_log (gpointer data);

static void
log_arrived (SvnLogCommand *cmd)
{
    GQueue *entries = NULL;
    SvnLogEntry *entry = NULL;

    entries = svn_log_command_get_entry_queue(cmd);

    while(! g_queue_is_empty (entries)) {
        entry = g_queue_pop_tail(entries);
        g_print (" r%ld | %8s | %s | %s\n",
                svn_log_entry_get_revision(entry),
                svn_log_entry_get_author(entry),
                svn_log_entry_get_date(entry),
                svn_log_entry_get_short_log(entry));
        last_rev = svn_log_entry_get_revision(entry);
        svn_log_entry_destroy (entry);
    }
}

static void
log_finished (SvnLogCommand *cmd, guint return_code)
{
    //g_message ("Log finished. ret = %d", return_code);
    svn_log_command_destroy (cmd);

    g_static_mutex_lock (&mutex);
    query_running = FALSE;
    g_static_mutex_unlock (&mutex);
}

static gboolean
query_log (gpointer data)
{
    SvnLogCommand *log = NULL;
    
    g_static_mutex_lock (&mutex);
    if (query_running) {
        g_static_mutex_unlock (&mutex);
        return TRUE;
    }

    log = svn_log_command_new (path);
    svn_log_command_set_start_rev (log, last_rev + 1);
    g_signal_connect (log, "data-arrived", (GCallback) log_arrived, NULL);
    g_signal_connect (log, "command-finished", (GCallback) log_finished, NULL);

    query_running = TRUE;
    g_static_mutex_unlock (&mutex);

    //g_message ("Log start.");
    anjuta_command_start(ANJUTA_COMMAND(log));

    return TRUE;
}

int
main(int argc, char *argv[])
{
    if (argc < 4) {
        g_print ("Usage: %s <path> <start_rev> <query-delay>\n", argv[0]);
        exit(1);
    }

    if (!g_thread_supported ()) g_thread_init (NULL);

    apr_initialize ();

    gtk_init (&argc, &argv);

    path = argv[1];
    last_rev = atoi(argv[2]);
    delay = atoi(argv[3]);

    query_log(NULL);

    g_timeout_add (1000 * delay, (GSourceFunc) query_log, NULL);

    gtk_main ();

    apr_terminate ();

    return 0;
}

