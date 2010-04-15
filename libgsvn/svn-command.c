/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "svn-command.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#define GLADE_FILE "anjuta-subversion.ui"

struct _SvnCommandPriv
{
	svn_client_ctx_t *client_context;
	apr_pool_t *pool;
	GQueue *info_messages;
	GMutex *ui_lock;
	gboolean main_thread_has_ui_lock;
	gboolean cancelled;
};

G_DEFINE_TYPE (SvnCommand, svn_command, ANJUTA_TYPE_ASYNC_COMMAND);

static gboolean
svn_command_acquire_ui_lock (SvnCommand *self)
{
	gboolean got_lock;
	
	if (!self->priv->main_thread_has_ui_lock)
	{
		got_lock = g_mutex_trylock (self->priv->ui_lock);
		
		if (got_lock)
			self->priv->main_thread_has_ui_lock = TRUE;
		
		return !got_lock;
	}
	else
		return FALSE;
}

static gboolean
svn_command_release_ui_lock (GMutex *ui_lock)
{
	g_mutex_unlock (ui_lock);
	g_mutex_free (ui_lock);
	
	return FALSE;
}

/* Auth functions */
/* In order to prevent deadlocking when Subversion prompts for something, we
 * have to make sure that no GTK calls are made from the command threads by way
 * of the authentication baton. To do this, the dialog code will be called from
 * idle sources. */

/* svn_auth_simple_prompt_func_cb argumements */
typedef struct
{
	svn_auth_cred_simple_t **cred;
	void *baton;
	gchar *realm;
	gchar *username;
	svn_boolean_t may_save;
	apr_pool_t *pool;
	svn_error_t *error;
} SimplePromptArgs;

/* svn_auth_ssl_server_trust_prompt_func_cb arguements */
typedef struct
{
	svn_auth_cred_ssl_server_trust_t **cred;
	void *baton; 
	gchar *realm;
	apr_uint32_t failures;
	svn_auth_ssl_server_cert_info_t *cert_info;
	svn_boolean_t may_save;
	apr_pool_t *pool;
	svn_error_t *error;
} SSLServerTrustArgs;

static gboolean
simple_prompt (SimplePromptArgs *args)
{
	GtkBuilder* bxml = gtk_builder_new ();
	GtkWidget* svn_user_auth;
	GtkWidget* auth_realm;
	GtkWidget* username_entry;
	GtkWidget* password_entry;
	GtkWidget* remember_pwd;
	svn_error_t *err = NULL;
	SvnCommand *svn_command;
	GError* error = NULL;

	if (!gtk_builder_add_from_file (bxml, GLADE_FILE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	svn_user_auth = GTK_WIDGET (gtk_builder_get_object (bxml, "svn_user_auth"));
	auth_realm = GTK_WIDGET (gtk_builder_get_object (bxml, "auth_realm"));
	username_entry = GTK_WIDGET (gtk_builder_get_object (bxml, "username_entry"));
	password_entry = GTK_WIDGET (gtk_builder_get_object (bxml, "password_entry"));
	remember_pwd = GTK_WIDGET (gtk_builder_get_object (bxml, "remember_pwd"));

	gtk_dialog_set_default_response (GTK_DIALOG (svn_user_auth), GTK_RESPONSE_OK);
	
	if (args->realm)
		gtk_label_set_text (GTK_LABEL (auth_realm), args->realm);
	if (args->username)
	{
		gtk_entry_set_text (GTK_ENTRY (username_entry), args->username);
		gtk_widget_grab_focus (password_entry);
	}
	if (!args->may_save)
		gtk_widget_set_sensitive(remember_pwd, FALSE);
		
	/* Then the dialog is prompted to user and when user clicks ok, the
	 * values entered, i.e username, password and remember password (true
	 * by default) should be used to initialized the memebers below. If the
	 * user cancels the dialog, I think we return an error struct
	 * appropriately initialized. -- naba
	 */
	 
 	switch (gtk_dialog_run(GTK_DIALOG(svn_user_auth)))
	{
		case GTK_RESPONSE_OK:
		{
			*args->cred = apr_pcalloc (args->pool, sizeof(*(args->cred)));
			(*(args->cred))->may_save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
															       (remember_pwd));
			(*(args->cred))->username = apr_pstrdup (args->pool,
								 	  gtk_entry_get_text(GTK_ENTRY(username_entry)));
			(*(args->cred))->password = apr_pstrdup (args->pool,
								 	  gtk_entry_get_text(GTK_ENTRY(password_entry)));
															 			
			err = SVN_NO_ERROR;
			break;
		}
		default:
			err = svn_error_create (SVN_ERR_AUTHN_CREDS_UNAVAILABLE, NULL,
									_("Authentication canceled"));
									 
			break;
	}
	gtk_widget_destroy (svn_user_auth);
	args->error = err;
	
	/* Release because main thread should already have the lock */
	svn_command = SVN_COMMAND (args->baton);
	svn_command_unlock_ui (svn_command);
	
	return FALSE;
}

static gboolean
ssl_server_trust_prompt (SSLServerTrustArgs *args)
{
	GtkBuilder* bxml = gtk_builder_new ();
	GtkWidget* svn_server_trust;
	GtkWidget* auth_realm;
	GtkWidget* server_info;
	GtkWidget* remember_check;
	svn_error_t *err = NULL;
	gchar* info;
	SvnCommand *svn_command;
	GError* error = NULL;

	if (!gtk_builder_add_from_file (bxml, GLADE_FILE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	svn_server_trust = GTK_WIDGET (gtk_builder_get_object (bxml, "svn_server_trust"));
	auth_realm = GTK_WIDGET (gtk_builder_get_object (bxml, "realm_label"));
	server_info = GTK_WIDGET (gtk_builder_get_object (bxml, "server_info_label"));
	remember_check = GTK_WIDGET (gtk_builder_get_object (bxml, "remember_check"));

	if (args->realm)
		gtk_label_set_text (GTK_LABEL (auth_realm), args->realm);
	
	info = g_strconcat(_("Hostname:"), args->cert_info->hostname, "\n",
					   _("Fingerprint:"), args->cert_info->fingerprint, "\n",
					   _("Valid from:"), args->cert_info->valid_from, "\n",
					   _("Valid until:"), args->cert_info->valid_until, "\n",
					   _("Issuer DN:"), args->cert_info->issuer_dname, "\n",
					   _("DER certificate:"), args->cert_info->ascii_cert, "\n",
					   NULL);
	gtk_label_set_text (GTK_LABEL (server_info), info);
	
	if (!args->may_save)
		gtk_widget_set_sensitive(remember_check, FALSE);
	
	gtk_dialog_set_default_response (GTK_DIALOG (svn_server_trust), GTK_RESPONSE_YES);

	
	switch (gtk_dialog_run(GTK_DIALOG(svn_server_trust)))
	{
		case GTK_RESPONSE_YES:
			*args->cred = apr_pcalloc (args->pool, 
									   sizeof(*(args->cred)));
			(*(args->cred))->may_save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
															 	   (remember_check));
			err = SVN_NO_ERROR;
		/* TODO: Set bitmask for accepted_failures */
			break;
		default:
			err = svn_error_create (SVN_ERR_AUTHN_CREDS_UNAVAILABLE, NULL,
									_("Authentication canceled"));									 
			break;
	}
	gtk_widget_destroy (svn_server_trust);
	args->error = err;
	
	/* Release because main thread should already have the lock */
	svn_command = SVN_COMMAND (args->baton);
	svn_command_unlock_ui (svn_command);
	
	return FALSE;
}

/* User authentication prompts handlers */
static svn_error_t*
svn_auth_simple_prompt_func_cb (svn_auth_cred_simple_t **cred, void *baton,
								const char *realm, const char *username,
								svn_boolean_t may_save, apr_pool_t *pool)
{
	SimplePromptArgs *args;
	SvnCommand *svn_command;
	svn_error_t *error;
	
	args = g_new0 (SimplePromptArgs, 1);
	args->cred = cred;
	args->baton = baton;
	args->realm = g_strdup (realm);
	args->username = g_strdup (username);
	args->may_save = may_save;
	args->pool = pool;
	
	svn_command = SVN_COMMAND (baton);
	
	g_idle_add ((GSourceFunc) simple_prompt, args);
	
	svn_command_lock_ui (svn_command);
	svn_command_unlock_ui (svn_command);
	
	error = args->error;
	g_free (args->realm);
	g_free (args->username);
	g_free (args);
	
	return error;

}

static svn_error_t*
svn_auth_ssl_server_trust_prompt_func_cb (svn_auth_cred_ssl_server_trust_t **cred,
										  void *baton, const char *realm,
										  apr_uint32_t failures,
										  const svn_auth_ssl_server_cert_info_t *cert_info,
										  svn_boolean_t may_save,
										  apr_pool_t *pool)
{
	SSLServerTrustArgs *args;
	SvnCommand *svn_command;
	svn_error_t *error;
	
	args = g_new0 (SSLServerTrustArgs, 1);
	args->cred = cred;
	args->baton = baton;
	args->realm = g_strdup (realm);
	args->failures = failures;
	args->cert_info = g_memdup (cert_info, 
								sizeof (svn_auth_ssl_server_cert_info_t));
	args->may_save = may_save;
	args->pool = pool;
	
	svn_command = SVN_COMMAND (baton);
	
	g_idle_add_full (G_PRIORITY_HIGH_IDLE,
					 (GSourceFunc) ssl_server_trust_prompt, args, NULL);
	
	svn_command_lock_ui (svn_command);
	svn_command_unlock_ui (svn_command);
	
	error = args->error;
	g_free (args->realm);
	g_free (args->cert_info);
	g_free (args);
	
	return error;
}

static svn_error_t*
svn_auth_ssl_client_cert_prompt_func_cb (svn_auth_cred_ssl_client_cert_t **cred,
										 void *baton, const char *realm,
										 svn_boolean_t may_save,
										 apr_pool_t *pool)
{
	
	/* Ask for the file where client certificate of authenticity is.
	 * I think it is some sort of private key. */
	return SVN_NO_ERROR;
}

static svn_error_t*
svn_auth_ssl_client_cert_pw_prompt_func_cb (svn_auth_cred_ssl_client_cert_pw_t **cred,
											void *baton, const char *realm,
											svn_boolean_t may_save,
											apr_pool_t *pool)
{
	
	/* Prompt for password only. I think it is pass-phrase of the above key. */
	return SVN_NO_ERROR;;
}

/* Notification callback to handle notifications from Subversion itself */
static void
on_svn_notify (gpointer baton,
			   const svn_wc_notify_t *notify,
			   apr_pool_t *pool)
{
	SvnCommand *self;
	gchar *action_message;
	gchar *state_message;
	
	self = SVN_COMMAND (baton);
	action_message = NULL;
	state_message = NULL;
	
	switch (notify->action)
	{
		case svn_wc_notify_delete:
			action_message = g_strdup_printf (_("Deleted: %s"), notify->path);
			break;
		case svn_wc_notify_add:
			action_message = g_strdup_printf (_("Added: %s"), notify->path);
			break;
		case svn_wc_notify_revert:
			action_message = g_strdup_printf ("Reverted: %s", notify->path);
			break;
		case svn_wc_notify_failed_revert:
			action_message = g_strdup_printf ("Revert failed: %s", 
											  notify->path);
			break;
		case svn_wc_notify_resolved:
			action_message = g_strdup_printf (_("Resolved: %s"), notify->path);
			break;
		case svn_wc_notify_update_delete:
			action_message = g_strdup_printf (_("Deleted: %s"), notify->path);
			break;
		case svn_wc_notify_update_update:
			action_message = g_strdup_printf (_("Updated: %s"), notify->path);
			break;
		case svn_wc_notify_update_add:
			action_message = g_strdup_printf (_("Added: %s"), notify->path);
			break;
		case svn_wc_notify_update_external:
			action_message = g_strdup_printf (_("Externally Updated: %s"), 
									   notify->path);
			break;
		case svn_wc_notify_commit_modified:
			action_message = g_strdup_printf ("Commit Modified: %s", 
											  notify->path);
			break;
		case svn_wc_notify_commit_added:
			action_message = g_strdup_printf ("Commit Added: %s", notify->path);
			break;
		case svn_wc_notify_commit_deleted:
			action_message = g_strdup_printf ("Commit Deleted: %s", 
											  notify->path);
			break;
		case svn_wc_notify_commit_replaced:
			action_message = g_strdup_printf ("Commit Replaced: %s", 
											  notify->path);
			break;
		case svn_wc_notify_copy:
			action_message = g_strdup_printf ("Created File: %s", notify->path);
			break;
		default:
			break;
	}
	
	if (action_message)
	{
		svn_command_push_info (self, action_message);
		g_free (action_message);
	}
	
	switch (notify->content_state)
	{
		case svn_wc_notify_state_changed:
			state_message = g_strdup_printf (_("Modified: %s"), notify->path);
			break;
		case svn_wc_notify_state_merged:
			state_message = g_strdup_printf (_("Merged: %s"), notify->path);
			break;
		case svn_wc_notify_state_conflicted:
			state_message = g_strdup_printf (_("Conflicted: %s"), 
											 notify->path);
			break;
		case svn_wc_notify_state_missing:
			state_message = g_strdup_printf (_("Missing: %s"), notify->path);
			break;
		case svn_wc_notify_state_obstructed:
			state_message = g_strdup_printf (_("Obstructed: %s"), notify->path);
			break;
		default:
			break;
	}
	
	if (state_message)
	{
		svn_command_push_info (self, state_message);
		g_free (state_message);
	}
}

/* Operation cancelling callback */
static svn_error_t *
on_svn_cancel (gpointer cancel_baton)
{
	SvnCommand *self;
	
	self = SVN_COMMAND (cancel_baton);
	
	if (self->priv->cancelled)
		return svn_error_create (SVN_ERR_CANCELLED, NULL, NULL);
	else
		return SVN_NO_ERROR;
}

static void
svn_command_init (SvnCommand *self)
{
	svn_auth_baton_t *auth_baton;
	apr_array_header_t *providers;
	svn_auth_provider_object_t *provider;
	
	self->priv = g_new0 (SvnCommandPriv, 1);
	
	self->priv->pool = svn_pool_create (NULL);
	svn_client_create_context (&self->priv->client_context, self->priv->pool);
	self->priv->client_context->notify_func2 = on_svn_notify;
	self->priv->client_context->notify_baton2 = self;
	self->priv->client_context->cancel_func = on_svn_cancel;
	self->priv->client_context->cancel_baton = self;
	
	svn_config_get_config (&(self->priv->client_context)->config,
						   NULL, /* default dir */
						   self->priv->pool);
	
	self->priv->info_messages = g_queue_new ();
	self->priv->ui_lock = g_mutex_new ();
	
	/* Make sure that the main thread holds the lock */
	g_idle_add ((GSourceFunc) svn_command_acquire_ui_lock, self);
	
	/* Fill in the auth baton callbacks */
	providers = apr_array_make (self->priv->pool, 1, 
								sizeof (svn_auth_provider_object_t *));
	
	/* Provider that authenticates username/password from ~/.subversion */
	provider = apr_pcalloc (self->priv->pool, sizeof(*provider));
	svn_client_get_simple_provider (&provider, self->priv->pool);
	*(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that authenticates server trust from ~/.subversion */
	provider = apr_pcalloc (self->priv->pool, sizeof(*provider));
	svn_client_get_ssl_server_trust_file_provider (&provider, self->priv->pool);
	*(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that authenticates client cert from ~/.subversion */
	provider = apr_pcalloc (self->priv->pool, sizeof(*provider));
	svn_client_get_ssl_client_cert_file_provider (&provider, self->priv->pool);
	*(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that authenticates client cert password from ~/.subversion */
	provider = apr_pcalloc (self->priv->pool, sizeof(*provider));
	svn_client_get_ssl_client_cert_pw_file_provider (&provider, 
													 self->priv->pool);
	*(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for username/password */
	provider = apr_pcalloc (self->priv->pool, sizeof(*provider));
	svn_client_get_simple_prompt_provider(&provider,
										  svn_auth_simple_prompt_func_cb,
										  self, 3, self->priv->pool);
	*(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for server trust */
	provider = apr_pcalloc (self->priv->pool, sizeof(*provider));
	svn_client_get_ssl_server_trust_prompt_provider (&provider,
													 svn_auth_ssl_server_trust_prompt_func_cb,
													 self, 
													 self->priv->pool);
	*(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for client certificate file */
	provider = apr_pcalloc (self->priv->pool, sizeof(*provider));
	svn_client_get_ssl_client_cert_prompt_provider (&provider,
													svn_auth_ssl_client_cert_prompt_func_cb,
													NULL, 3, self->priv->pool);
	*(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for client certificate file password */
	provider = apr_pcalloc (self->priv->pool, sizeof(*provider));
	svn_client_get_ssl_client_cert_pw_prompt_provider (&provider,
													   svn_auth_ssl_client_cert_pw_prompt_func_cb,
													   NULL, 3, 
													   self->priv->pool);
	*(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	svn_auth_open (&auth_baton, providers, self->priv->pool);
	self->priv->client_context->auth_baton = auth_baton;
	
}

static void
svn_command_finalize (GObject *object)
{
	SvnCommand *self;
	GList *current_message_line;
	
	self = SVN_COMMAND (object);
	
	svn_pool_clear (self->priv->pool);
	svn_pool_destroy (self->priv->pool);
	
	current_message_line = self->priv->info_messages->head;
	
	while (current_message_line)
	{
		g_free (current_message_line->data);
		current_message_line = g_list_next (current_message_line);
	}
	
	g_idle_add ((GSourceFunc) svn_command_release_ui_lock, self->priv->ui_lock);
	
	g_queue_free (self->priv->info_messages);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_command_parent_class)->finalize (object);
}

static void
svn_command_cancel (AnjutaCommand *command)
{
	SvnCommand *self;
	
	self = SVN_COMMAND (command);
	
	self->priv->cancelled = TRUE;
}

static void
svn_command_class_init (SvnCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_command_finalize;
	command_class->cancel = svn_command_cancel;
}


void
svn_command_push_info (SvnCommand *self, const gchar *message)
{
	anjuta_async_command_lock (ANJUTA_ASYNC_COMMAND (self));
	g_queue_push_tail (self->priv->info_messages, g_strdup (message));
	anjuta_async_command_unlock (ANJUTA_ASYNC_COMMAND (self));
	
	anjuta_command_notify_data_arrived (ANJUTA_COMMAND (self));
}

GQueue *
svn_command_get_info_queue (SvnCommand *self)
{
	return self->priv->info_messages;
}

void
svn_command_set_error (SvnCommand *self, svn_error_t *error)
{
	GString *error_string;
	svn_error_t *current_error;
	gchar *error_c_string;
	
	error_string = g_string_new ("");
	current_error = error;
	
	while (current_error)
	{
		g_string_append (error_string, current_error->message);
		
		if (current_error->child)
			g_string_append_c (error_string, '\n');
		
		current_error = current_error->child;
	}
	
	error_c_string = g_string_free (error_string, FALSE);
	anjuta_async_command_set_error_message (ANJUTA_COMMAND (self), 
											error_c_string);
	
	g_free (error_c_string);
}

svn_client_ctx_t *
svn_command_get_client_context (SvnCommand *self)
{
	return self->priv->client_context;
}

apr_pool_t *
svn_command_get_pool (SvnCommand *self)
{
	return self->priv->pool;
}

void
svn_command_lock_ui (SvnCommand *self)
{
	g_mutex_lock (self->priv->ui_lock);
	
	/* Have the main thread acquire the lock as soon as the other thread is done
	 * with it. The main thread should *not* acqure the lock, only release 
	 * it. */
	self->priv->main_thread_has_ui_lock = FALSE;
	g_idle_add ((GSourceFunc) svn_command_acquire_ui_lock, self);
}

void
svn_command_unlock_ui (SvnCommand *self)
{
	g_mutex_unlock (self->priv->ui_lock);
}

gchar *
svn_command_make_canonical_path (SvnCommand *self, const gchar *path)
{
	const gchar *canonical_path;
	
	canonical_path = NULL;
	
	if (path)
		canonical_path = svn_path_canonicalize (path, self->priv->pool);
	
	return g_strdup (canonical_path);
}

svn_opt_revision_t *
svn_command_get_revision (const gchar *revision)
{
	svn_opt_revision_t* svn_revision; 
	
	svn_revision = g_new0 (svn_opt_revision_t, 1);
	
	/* FIXME: Parse the revision string */
	svn_revision->kind = svn_opt_revision_head;
	
	return svn_revision;
}

GList *
svn_command_copy_path_list (GList *list)
{
	GList *current_path;
	GList *new_list;
	
	new_list = NULL;
	current_path = list;
	
	while (current_path)
	{
		new_list = g_list_append (new_list, g_strdup (current_path->data));
		current_path = g_list_next (current_path);
	}
	
	return new_list;
}

void
svn_command_free_path_list (GList *list)
{
	GList *current_path;
	
	current_path = list;
	
	while (current_path)
	{
		g_free (current_path->data);
		current_path = g_list_next (current_path);
	}
	
	g_list_free (list);
}
