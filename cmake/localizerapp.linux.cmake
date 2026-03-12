include_guard (DIRECTORY)

list (APPEND localizerapp_ccl_sources
	${CCL_DIR}/platform/linux/linuxmain.cpp
)

install (TARGETS localizerapp RUNTIME DESTINATION ${VENDOR_APPLICATION_RUNTIME_DIRECTORY})

ccl_install_desktop_file (${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/resource/localizerapp.desktop.in "${localizerapp_PACKAGE_ID}")
ccl_install_icon (localizerapp ${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/resource/localizer.svg)

include (${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/deb/localizerapp.package.cmake)
