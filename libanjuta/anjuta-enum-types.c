
#include "anjuta-enum-types.h"
#include "anjuta-vcs-status.h"

/* enumerations from "anjuta-vcs-status.h" */
GType
anjuta_vcs_status_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { ANJUTA_VCS_STATUS_MODIFIED, "ANJUTA_VCS_STATUS_MODIFIED", "modified" },
      { ANJUTA_VCS_STATUS_ADDED, "ANJUTA_VCS_STATUS_ADDED", "added" },
      { ANJUTA_VCS_STATUS_DELETED, "ANJUTA_VCS_STATUS_DELETED", "deleted" },
      { ANJUTA_VCS_STATUS_CONFLICTED, "ANJUTA_VCS_STATUS_CONFLICTED", "conflicted" },
      { ANJUTA_VCS_STATUS_UPTODATE, "ANJUTA_VCS_STATUS_UPTODATE", "uptodate" },
      { ANJUTA_VCS_STATUS_LOCKED, "ANJUTA_VCS_STATUS_LOCKED", "locked" },
      { ANJUTA_VCS_STATUS_MISSING, "ANJUTA_VCS_STATUS_MISSING", "missing" },
      { ANJUTA_VCS_STATUS_UNVERSIONED, "ANJUTA_VCS_STATUS_UNVERSIONED", "unversioned" },
      { ANJUTA_VCS_STATUS_IGNORED, "ANJUTA_VCS_STATUS_IGNORED", "ignored" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static (g_intern_static_string ("AnjutaVcsStatus"), values);
  }
  return etype;
}

#define __ANJUTA_ENUM_TYPES_C__

