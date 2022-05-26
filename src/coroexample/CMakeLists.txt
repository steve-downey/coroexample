add_library(coroexample STATIC "")

target_sources(
  coroexample
  PRIVATE
  coroexample.cpp
  coroexample_generalhelper.cpp
  coroexample_helper.cpp
  coroexample_task.cpp
  coroexample_asyncscope.cpp
  coroexample_syncwait.cpp
  coroexample_manualeventloop.cpp
  coroexample_lazytask.cpp
  )

include(GNUInstallDirs)

target_include_directories(coroexample PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_LOWER_PROJECT_NAME}>  # <prefix>/include/coroexample
  )

install(
  TARGETS coroexample
  EXPORT ${TARGETS_EXPORT_NAME}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

string(TOLOWER ${CMAKE_PROJECT_NAME} CMAKE_LOWER_PROJECT_NAME)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_LOWER_PROJECT_NAME}
  FILES_MATCHING PATTERN "*.h"
  )
