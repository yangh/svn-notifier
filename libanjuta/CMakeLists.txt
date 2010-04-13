LINK_DIRECTORIES(
    ${GLIB2_LIBRARY_DIRS}
    )

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${GLIB2_INCLUDE_DIRS}
    )

SET( ANJUTA_SRCS 
    anjuta-async-command.c
    anjuta-async-notify.c
    anjuta-command.c
    anjuta-command-queue.c
    anjuta-enum-types.c
    anjuta-vcs-status-tree-view.c
    )

ADD_LIBRARY( anjutacmd SHARED ${ANJUTA_SRCS} )

TARGET_LINK_LIBRARIES( anjutacmd
    ${GLIB2_LIBRARIES}
    )
