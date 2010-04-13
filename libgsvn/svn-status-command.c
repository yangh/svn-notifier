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

#include "svn-status-command.h"

struct _SvnStatusCommandPriv
{
	gchar *path;
	gboolean recursive;
	gboolean get_all_items;
	GQueue *status_queue;
};

G_DEFINE_TYPE (SvnStatusCommand, svn_status_command, SVN_TYPE_COMMAND);

static void
svn_status_command_init (SvnStatusCommand *self)
{	
	self->priv = g_new0 (SvnStatusCommandPriv, 1);
	self->priv->status_queue = g_queue_new ();	
}

static void
svn_status_command_finalize (GObject *object)
{
	SvnStatusCommand *self;
	GList *current_status;
	
	self = SVN_STATUS_COMMAND (object);
	current_status = self->priv->status_queue->head;
	
	g_free (self->priv->path);
	
	while (current_status)
	{
		svn_status_destroy (current_status->data);
		current_status = g_list_next (current_status);
	}
	
	g_queue_free (self->priv->status_queue);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_status_command_parent_class)->finalize (object);
}

static void
on_svn_status_notify (void *baton, const char *path, svn_wc_status2_t *status)
{
	SvnStatusCommand *self;
	SvnStatus *status_object;
	
	self = SVN_STATUS_COMMAND (baton);
	
	status_object = svn_status_new ((gchar *) path, 
									status->text_status);
	
	anjuta_async_command_lock (ANJUTA_ASYNC_COMMAND (self));
	g_queue_push_tail (self->priv->status_queue, status_object);
	anjuta_async_command_unlock (ANJUTA_ASYNC_COMMAND (self));
	
	anjuta_command_notify_data_arrived (ANJUTA_COMMAND (self));
	
}

static guint 
svn_status_command_run (AnjutaCommand *command)
{
	SvnStatusCommand *self;
	SvnCommand *svn_command;
	svn_opt_revision_t revision;
	svn_error_t *error;
	
	self = SVN_STATUS_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	revision.kind = svn_opt_revision_working;
	
	error = svn_client_status2 (NULL, 
								self->priv->path, 
								&revision,
								on_svn_status_notify, 
								self, 
								self->priv->recursive, 
								self->priv->get_all_items, 
								FALSE, 
								FALSE,
								TRUE,
								svn_command_get_client_context (svn_command),
								svn_command_get_pool (svn_command));
	
	if (error)
	{
		svn_command_set_error (svn_command, error);
		return 1;
	}
	
	return 0;
}

static void
svn_status_command_class_init (SvnStatusCommandClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	command_class->run = svn_status_command_run;
	object_class->finalize = svn_status_command_finalize;
}

SvnStatusCommand *
svn_status_command_new (const gchar *path, gboolean recursive, gboolean get_all_items)
{
	SvnStatusCommand *self;
	
	self = g_object_new (SVN_TYPE_STATUS_COMMAND, NULL);
	self->priv->path = svn_command_make_canonical_path (SVN_COMMAND (self),
														path);
	self->priv->recursive = recursive;
	self->priv->get_all_items = get_all_items;
	
	return self;
}

void
svn_status_command_destroy (SvnStatusCommand *self)
{
	g_object_unref (self);
}

GQueue *
svn_status_command_get_status_queue (SvnStatusCommand *self)
{
	return self->priv->status_queue;
}
