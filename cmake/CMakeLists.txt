#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2025 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
# 
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : CMakeLists.txt
# Description : Localizer
#
#************************************************************************************************

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project ("CCL Localizer")
set (VENDOR_PRODUCT_NAME "CCL Localizer")
set (PROJECT_VENDOR ccl)

# Find dependencies
include (vendor OPTIONAL)
find_package (ccl ${CCL_VERSION_REQUIREMENT} REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclgui cclextras-tools)
find_package (cclspy)
find_package (skins)
find_package (translations REQUIRED)

list (APPEND localizerapp_license_files
	${CCL_IDENTITIES_DIR}/ccl/legal/CCL\ 3rd\ Party\ Licenses.txt
	${CCL_IDENTITIES_DIR}/ccl/eula/EULA.txt
)

list (APPEND localizerapp_asset_folders
	${CMAKE_CURRENT_LIST_DIR}/../assets/help
)

# Add targets
ccl_add_app (localizerapp GUI
	VENDOR ccl
	VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
	VERSION_PREFIX APP
)
ccl_set_product_name (localizerapp ${localizerapp_NAME})
ccl_set_startup_project (localizerapp)

ccl_use_skin_packages (localizerapp neutralbase neutraldesign fileicons)
ccl_add_skin_package (localizerapp ${CMAKE_CURRENT_LIST_DIR}/../skin)

if (NOT CCL_LOCALIZER_TRANSLATIONS_DIR)
	set (CCL_LOCALIZER_TRANSLATIONS_DIR "${CMAKE_CURRENT_LIST_DIR}/../translations" CACHE PATH "CCL Localizer translations directory" FORCE)
endif ()
ccl_list_append_once (VENDOR_TRANSLATIONS_DIRS "${CCL_LOCALIZER_TRANSLATIONS_DIR}")
ccl_use_translation_packages (localizerapp PROJECT localizerapp de ja zh fr es)

ccl_add_assets (localizerapp DIRECTORY ${localizerapp_asset_folders})
ccl_add_assets (localizerapp FILES ${localizerapp_license_files} PATH "license")

list (APPEND localizerapp_resources
	${CMAKE_CURRENT_LIST_DIR}/../resource/cclgui.config
	${CMAKE_CURRENT_LIST_DIR}/../resource/commands.xml
	${CMAKE_CURRENT_LIST_DIR}/../resource/menubar.xml
	${CMAKE_CURRENT_LIST_DIR}/../resource/pseudo.properties
)

list (APPEND localizerapp_source_files
	${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h
	${CMAKE_CURRENT_LIST_DIR}/../source/editfunctions.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/editfunctions.h
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.builder.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.builder.h
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.editor.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.editor.filelist.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.editor.h
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.editor.occurancelist.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.editor.scopelist.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.editor.searchlist.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.editor.stringlist.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.manager.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.manager.h
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.model.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.model.h
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.po.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.po.h
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.text.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.text.h
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.tools.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizer.tools.h
	${CMAKE_CURRENT_LIST_DIR}/../source/localizerapp.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/localizerapp.h
	${CMAKE_CURRENT_LIST_DIR}/../source/pseudotable.cpp
	${CMAKE_CURRENT_LIST_DIR}/../source/pseudotable.h
)

list (APPEND localizerapp_ccl_sources
	${CCL_DIR}/main/cclmain.cpp
)

source_group ("source" FILES ${localizerapp_source_files})
source_group ("source\\ccl" FILES ${localizerapp_ccl_sources})

list (APPEND localizerapp_sources ${localizerapp_source_files} ${localizerapp_ccl_sources})

ccl_add_resources (localizerapp ${localizerapp_resources})
target_sources (localizerapp PRIVATE ${localizerapp_sources})
target_link_libraries (localizerapp PRIVATE cclapp cclbase ccltext cclsystem cclgui cclextras-tools)

ccl_add_dependencies (localizerapp cclspy)
