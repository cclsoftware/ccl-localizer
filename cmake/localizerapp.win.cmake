include_guard (DIRECTORY)

list (APPEND localizerapp_ccl_sources
	${CCL_DIR}/platform/win/winmain.cpp
)

# Numbers used here need to match the ones used in localizerapp.nsi!
list (APPEND localizerapp_icons
	1	${CMAKE_CURRENT_LIST_DIR}/../packaging/win/resource/localizer.ico
	2	${CMAKE_CURRENT_LIST_DIR}/../packaging/win/resource/localizer-doc.ico
)

ccl_embed_manifest (localizerapp "${CCL_DIR}/packaging/win/application.manifest")

ccl_nsis_package (localizerapp "${CMAKE_CURRENT_LIST_DIR}/../packaging/win/nsis/localizerapp.nsi" "/DCCL_APPLICATIONS_DIR=${CMAKE_CURRENT_LIST_DIR}/../..")