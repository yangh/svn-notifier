LINK_DIRECTORIES( ${GLIB2_LIBRARY_DIRS}
    ${SVN_DEP_LIBRARY_DIRS}
    ${SVN_CLIENT_LIBRARY_DIRS}
    )

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${SVN_DEP_INCLUDE_DIRS}
    ${SVN_CLIENT_INCLUDE_DIRS}
    )

SET( SVN_SRCS
    main.c
    svn-notify.c
    )

ADD_EXECUTABLE( svn-notifier ${SVN_SRCS} )
TARGET_LINK_LIBRARIES( svn-notifier
    gsvn
    anjutacmd
    ${GLIB2_LIBRARIES}
    ${SVN_DEP_LIBRARIES}
    ${SVN_CLIENT_LIBRARIES}
    )

