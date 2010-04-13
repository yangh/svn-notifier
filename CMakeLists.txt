PROJECT( sync C )

cmake_minimum_required( VERSION 2.6 )

FIND_PACKAGE( PkgConfig REQUIRED )

IF( PKG_CONFIG_FOUND )
    PKG_CHECK_MODULES ( GLIB2   REQUIRED glib-2.0 gobject-2.0 gthread-2.0 )
    PKG_CHECK_MODULES ( SVN_DEP REQUIRED apr-1 apr-util-1 )
ENDIF( PKG_CONFIG_FOUND )

# Add svn include/lib
FIND_PATH( SVN_HEADER_DIR svn_client.h
    HINTS
    $ENV{SVN_DIR}
    PATH_SUFFIXES include/subversion include/subversion-1
    PATHS
    /usr
    /usr/local
    /opt
    /opt/local
)

SET(SVN_CLIENT_INCLUDE_DIRS ${SVN_HEADER_DIR})

SET(SVN_CLIENT_LIBS svn_client-1 svn_subr-1)

FIND_LIBRARY(SVN_CLIENT_LIBRARY
    NAMES ${SVN_CLIENT_LIBS}
    $ENV{SVN_DIR}
    PATH_SUFFIXES lib lib64
    PATHS
    /usr
    /usr/local
    /opt
    /opt/local
)

SET(SVN_CLIENT_LIBRARIES ${SVN_CLIENT_LIBRARY})

ADD_SUBDIRECTORY( libgsvn src )

