#ifndef __ANJUTA_ENUM_TYPES_H__
#define __ANJUTA_ENUM_TYPES_H__

#include <glib-object.h>

G_BEGIN_DECLS

/* enumerations from "anjuta-vcs-status.h" */
GType anjuta_vcs_status_get_type (void) G_GNUC_CONST;
#define ANJUTA_TYPE_VCS_STATUS (anjuta_vcs_status_get_type())

G_END_DECLS

#endif /* __ANJUTA_ENUM_TYPES_H__ */

