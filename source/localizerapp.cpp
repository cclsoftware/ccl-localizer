//************************************************************************************************
//
// Localizer Application
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : localizerapp.cpp
// Description : Localizer Application
//
//************************************************************************************************

#define SERVICES_ENABLED 1//RELEASE // plug-ins + services (for cclspy only)

#include "localizerapp.h"
#include "appversion.h"

#include "localizer.manager.h"
#include "localizer.tools.h"

#include "ccl/app/options/mainoption.h"
#include "ccl/app/components/eulacomponent.h"
#include "ccl/app/documents/recentdocuments.h"

#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/plugins/iservicemanager.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/main/cclargs.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////

void ccl_app_init ()
{
	NEW LocalizerApplication;
}

//************************************************************************************************
// LocalizerApplication
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LocalizerApplication, Application)

//////////////////////////////////////////////////////////////////////////////////////////////////

LocalizerApplication::LocalizerApplication ()
: Application (APP_ID, /*APP_COMPANY*/0, APP_NAME, APP_PACKAGE_ID, APP_VERSION)
{
	setWebsite (APP_WEBSITE);
	setBuildInformation (APP_FULL_NAME);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocalizerApplication::startup ()
{
	if(!SuperClass::startup ())
		return false;
	
	// init color scheme
	MainColorSchemeOption* appSchemeOption = UserOption::init<MainColorSchemeOption> ();
	appSchemeOption->addConfigurationSavers ();
	
	// load theme
	Url skinFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (skinFolder, CCL_APPLICATION_BASEDIR, "skin")
	if(!loadTheme (skinFolder))
		return false;

	// EULA
	Url eulaFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (eulaFolder, VENDOR_IDENTITY_DIR, "eula")
	if(!EULAComponent ().startup (&eulaFolder))
		return false;

	// set debug locations
	#if DEBUG
	Url assetsLocation;
	GET_DEVELOPMENT_FOLDER_LOCATION (assetsLocation, CCL_APPLICATION_BASEDIR, "assets")
	System::GetSystem ().setLocation (System::kAppDeploymentFolder, assetsLocation);

	Url helpFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (helpFolder, CCL_APPLICATION_BASEDIR, "assets/help")
	System::GetHelpManager ().setHelpLocation (helpFolder);
	#endif

	// create components
	addComponent (NEW ProjectManager);
	addComponent (NEW LanguagePackBuilder);
	#if DEBUG
	addComponent (NEW ContextConverter);
	#endif

	// create main window
	createWindow ();

	// plug-ins + start services
	#if SERVICES_ENABLED
	scanPlugIns ();
	System::GetServiceManager ().startup ();
	#endif

	// scan language packs
	Url languageFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (languageFolder, CCL_APPLICATION_BASEDIR, "translations/projects/localizerapp/languages")	
	scanLanguagePacks (languageFolder);

	// register options
	LocaleOption* localeOption = UserOption::init<LocaleOption> ();
	localeOption->addLanguagePacks ();
	localeOption->setLanguageRestartEnabled (true);

	// update menu bar
	if(languageMenu.menu)
	{
		MenuInserter inserter (languageMenu);
		localeOption->makeLanguageMenu (*languageMenu.menu);
	}
	if(appearanceMenu.menu)
	{
		MenuInserter inserter (appearanceMenu);
		appSchemeOption->makeAppearanceMenu (*appearanceMenu.menu, true);
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocalizerApplication::shutdown ()
{
	// stop services
	#if SERVICES_ENABLED
	System::GetServiceManager ().shutdown ();
	#endif

	return SuperClass::shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocalizerApplication::beforeQuit ()
{
	// The menu should not be used anymore because it is going away
	if(auto manager = ProjectManager::getInstance ())
		manager->getRecentPaths ().removeMenus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LocalizerApplication::extendMenu (IMenu& menu, StringID name)
{
	if(name == "Languages")
		languageMenu = MenuPosition (menu);
	else if(name == "Appearance")
		appearanceMenu = MenuPosition (menu);
	else if(name.startsWith ("Recent Files"))
	{
		IMenu* subMenu = menu.createMenu ();
		subMenu->setMenuAttribute (IMenu::kMenuTitle, RecentDocuments::getTranslatedTitle ());
		menu.addMenu (subMenu);

		if(auto manager = ProjectManager::getInstance ())
			manager->getRecentPaths ().addMenu (subMenu);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LocalizerApplication::processCommandLine (ArgsRef args)
{
	if(args.count () >= 2)
	{
		Url path;
		if(path.fromDisplayString (args[1]))
		{
			openFile (path);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LocalizerApplication::openFile (UrlRef path)
{
	if(path.isNativePath ())
		if(auto manager = ProjectManager::getInstance ())
			return manager->openLocation (path);

	return SuperClass::openFile (path);
}
