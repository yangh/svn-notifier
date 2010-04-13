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

#include "svn-status.h"

struct _SvnStatusPriv
{
	gchar *path;
	enum svn_wc_status_kind status;
};

G_DEFINE_TYPE (SvnStatus, svn_status, G_TYPE_OBJECT);

static void
svn_status_init (SvnStatus *self)
{
	self->priv = g_new0 (SvnStatusPriv, 1);
}

static void
svn_status_finalize (GObject *object)
{
	SvnStatus *self;
	
	self = SVN_STATUS (object);
	
	g_free (self->priv->path);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_status_parent_class)->finalize (object);
}

static void
svn_status_class_init (SvnStatusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = svn_status_finalize;
}

SvnStatus *
svn_status_new (gchar *path, enum svn_wc_status_kind status)
{
	SvnStatus *self;
	
	self = g_object_new (SVN_TYPE_STATUS, NULL);
	
	self->priv->path = g_strdup (path);
	self->priv->status = status;
	
	return self;
}

void
svn_status_destroy (SvnStatus *self)
{
	g_object_unref (self);
}

gchar *
svn_status_get_path (SvnStatus *self)
{
	return g_strdup (self->priv->path);
}

AnjutaVcsStatus
svn_status_get_vcs_status (SvnStatus *self)
{
	AnjutaVcsStatus status;
	
	switch (self->priv->status)
	{
		case svn_wc_status_external:
		case svn_wc_status_incomplete:			
			status = ANJUTA_VCS_STATUS_NONE;
			break;
		case svn_wc_status_modified:
		case svn_wc_status_replaced:
		case svn_wc_status_merged:			
			status = ANJUTA_VCS_STATUS_MODIFIED;
			break;
		case svn_wc_status_added:
			status = ANJUTA_VCS_STATUS_ADDED;
			break;
		case svn_wc_status_deleted:
			status = ANJUTA_VCS_STATUS_DELETED;
			break;
		case svn_wc_status_conflicted:
		case svn_wc_status_obstructed:
			status = ANJUTA_VCS_STATUS_CONFLICTED;
			break;
		case svn_wc_status_missing:
			status = ANJUTA_VCS_STATUS_MISSING;
			break;
		case svn_wc_status_unversioned:
			status = ANJUTA_VCS_STATUS_UNVERSIONED;
			break;
		case svn_wc_status_ignored:
			status = ANJUTA_VCS_STATUS_IGNORED;
			break;		
		default:
			status = ANJUTA_VCS_STATUS_UPTODATE;
	}
	
	return status;
}
