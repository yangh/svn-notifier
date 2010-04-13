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

#ifndef _SVN_SWITCH_COMMAND_H_
#define _SVN_SWITCH_COMMAND_H_

#include <glib-object.h>
#include "svn-command.h"

G_BEGIN_DECLS

#define SVN_TYPE_SWITCH_COMMAND             (svn_switch_command_get_type ())
#define SVN_SWITCH_COMMAND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVN_TYPE_SWITCH_COMMAND, SvnSwitchCommand))
#define SVN_SWITCH_COMMAND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SVN_TYPE_SWITCH_COMMAND, SvnSwitchCommandClass))
#define SVN_IS_SWITCH_COMMAND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVN_TYPE_SWITCH_COMMAND))
#define SVN_IS_SWITCH_COMMAND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SVN_TYPE_SWITCH_COMMAND))
#define SVN_SWITCH_COMMAND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SVN_TYPE_SWITCH_COMMAND, SvnSwitchCommandClass))

typedef struct _SvnSwitchCommandClass SvnSwitchCommandClass;
typedef struct _SvnSwitchCommand SvnSwitchCommand;
typedef struct _SvnSwitchCommandPriv SvnSwitchCommandPriv;

struct _SvnSwitchCommandClass
{
	SvnCommandClass parent_class;
};

struct _SvnSwitchCommand
{
	SvnCommand parent_instance;
	
	SvnSwitchCommandPriv *priv;
};

enum
{
	SVN_SWITCH_REVISION_HEAD = -1
};

GType svn_switch_command_get_type (void) G_GNUC_CONST;
SvnSwitchCommand * svn_switch_command_new (const gchar *working_copy_path, 
										   const gchar *branch_url, glong revision, 
										   gboolean recursive);
void svn_switch_command_destroy (SvnSwitchCommand *self);

G_END_DECLS

#endif /* _SVN_SWITCH_COMMAND_H_ */
