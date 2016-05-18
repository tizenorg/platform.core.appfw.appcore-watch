CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(appcore-watch C)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION_MAJOR 1)
SET(VERSION "${VERSION_MAJOR}.1")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -fvisibility=hidden")
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -g -Wall")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

#################################################################
# Build appcore-watch Library
# ------------------------------
SET(APPCORE_WATCH "appcore-watch")
SET(SRCS_watch
		src/appcore-watch.c
		src/appcore-watch-signal.c
		src/appcore-watch-i18n.c
		src/watch_app_main.c
		src/watch_app_error.c
		)
SET(HEADERS_watch appcore-watch.h
			watch_app.h watch_app_efl.h)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkg_watch REQUIRED
		aul
		dlog
		dbus-glib-1
		alarm-service
		capi-appfw-app-control
		capi-appfw-app-common
		vconf
		elementary
		ecore-wayland
		vconf-internal-keys
		libtzplatform-config
		capi-appfw-widget-application
		icu-i18n
		)
FOREACH(flag ${pkg_watch_CFLAGS})
	SET(EXTRA_CFLAGS_watch "${EXTRA_CFLAGS_watch} ${flag}")
ENDFOREACH(flag)

ADD_LIBRARY(${APPCORE_WATCH} SHARED ${SRCS_watch})
SET_TARGET_PROPERTIES(${APPCORE_WATCH} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${APPCORE_WATCH} PROPERTIES VERSION ${VERSION})
SET_TARGET_PROPERTIES(${APPCORE_WATCH} PROPERTIES COMPILE_FLAGS ${EXTRA_CFLAGS_watch})
TARGET_LINK_LIBRARIES(${APPCORE_WATCH} ${pkg_watch_LDFLAGS} "-ldl -Wl,--no-undefined")

CONFIGURE_FILE(${APPCORE_WATCH}.pc.in ${APPCORE_WATCH}.pc @ONLY)

INSTALL(TARGETS ${APPCORE_WATCH} DESTINATION lib COMPONENT RuntimeLibraries)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${APPCORE_WATCH}.pc DESTINATION lib/pkgconfig)
FOREACH(hfile ${HEADERS_watch})
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/${hfile} DESTINATION include/appcore-watch)
ENDFOREACH(hfile)
