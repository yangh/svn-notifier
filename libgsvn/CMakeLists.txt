LINK_DIRECTORIES(
    ${GLIB2_LIBRARY_DIRS}
    ${SVN_DEP_LIBRARY_DIRS}
    ${SVN_CLIENT_LIBRARY_DIRS}
    )

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${SVN_DEP_INCLUDE_DIRS}
    ${SVN_CLIENT_INCLUDE_DIRS}
    )

SET( GSVN_SRCS 
    anjuta-async-command.c
    anjuta-async-notify.c
    anjuta-command.c
    anjuta-command-queue.c
    svn-add-command.c
    svn-cat-command.c
    svn-checkout-command.c
    svn-command.c
    svn-commit-command.c
    svn-copy-command.c
    svn-diff-command.c
    svn-log-command.c
    svn-log-entry.c
    svn-merge-command.c
    svn-remove-command.c
    svn-resolve-command.c
    svn-revert-command.c
    svn-status.c
    svn-status-command.c
    svn-switch-command.c
    svn-update-command.c
    )

ADD_LIBRARY( gsvn SHARED ${GSVN_SRCS} )

TARGET_LINK_LIBRARIES( gsvn
    ${GLIB2_LIBRARIES}
    ${SVN_DEP_LIBRARIES}
    ${SVN_CLIENT_LIBRARIES}
    )
