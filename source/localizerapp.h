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
// Filename    : localizerapp.h
// Description : Localizer Application
//
//************************************************************************************************

#ifndef _localizerapp_h
#define _localizerapp_h

#include "ccl/app/application.h"

namespace Localizer {

//************************************************************************************************
// LocalizerApplication
//************************************************************************************************

class LocalizerApplication: public CCL::Application
{
public:
	DECLARE_CLASS (LocalizerApplication, Application)

	LocalizerApplication ();

	// Application
	bool startup () override;
	bool shutdown () override;
	void beforeQuit () override;
	void CCL_API extendMenu (CCL::IMenu& menu, CCL::StringID name) override;
	void CCL_API processCommandLine (CCL::ArgsRef args) override;
	CCL::tbool CCL_API openFile (CCL::UrlRef path) override;

protected:
	CCL::MenuPosition languageMenu;
	CCL::MenuPosition appearanceMenu;
};

} // namespace Localizer

#endif // _localizerapp_h
