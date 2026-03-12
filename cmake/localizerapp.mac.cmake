include_guard (DIRECTORY)

list (APPEND localizerapp_ccl_sources
	${CCL_DIR}/platform/cocoa/cocoamain.mm
)

set (localizerapp_file_icons_dir ${CMAKE_CURRENT_LIST_DIR}/../packaging/mac/resource)
list (APPEND localizerapp_file_icons
	${localizerapp_file_icons_dir}/localizer-doc.iconset
)
set_source_files_properties (${localizerapp_file_icons} PROPERTIES 
	XCODE_EXPLICIT_FILE_TYPE "folder.iconset"
)

file (GLOB gettext_binary_files ${CCL_TOOLS_BINDIR}/mac/gettext/*)
target_sources (localizerapp PRIVATE ${gettext_binary_files})
set_source_files_properties (${gettext_binary_files} PROPERTIES MACOSX_PACKAGE_LOCATION "Frameworks")

list (APPEND localizerapp_bundle_resources
	${CMAKE_CURRENT_LIST_DIR}/../packaging/mac/resource/localizer.xcassets
	${CCL_DIR}/packaging/mac/resource/MainMenu.xib
	${localizerapp_file_icons}
)

list (APPEND localizerapp_resources
	${localizerapp_bundle_resources}
)

list (APPEND localizerapp_sources
	${localizerapp_resources}
)
source_group ("Resources" FILES ${localizerapp_resources})

set (localizerapp_FRAMEWORKS
	ccltext
	cclsystem
	cclgui
)

set_target_properties (localizerapp PROPERTIES
	RESOURCE "${localizerapp_bundle_resources}"
	MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/../packaging/mac/Info.plist"
	XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"

	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
)

if(${VENDOR_USE_PUBLISHER_CERTIFICATE})
	set_target_properties (localizerapp PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${SIGNING_CERTIFICATE_MAC}"
	)
endif()
