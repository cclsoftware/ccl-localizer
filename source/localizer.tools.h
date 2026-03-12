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
// Filename    : localizer.tools.h
// Description : Language Pack Builder
//
//************************************************************************************************

#ifndef _localizer_tools_h
#define _localizer_tools_h

#include "ccl/app/component.h"

#include "ccl/public/gui/commanddispatch.h"

namespace Localizer {

//************************************************************************************************
// ToolsComponent
//************************************************************************************************

class ToolsComponent: public CCL::Component
{
public:
	DECLARE_CLASS (ToolsComponent, Component)

	bool onCmd (CCL::CmdArgs);

protected:
	CCL::AutoPtr<CCL::IUrl> path; // File or folder to process.

	ToolsComponent (CCL::StringRef name = 0);

	virtual bool selectPath ();
	virtual void setup ();
	virtual void run (); // Work with active console window.
	virtual void cleanup ();
};

//************************************************************************************************
// LanguagePackBuilder
//************************************************************************************************

class LanguagePackBuilder: public ToolsComponent,
						   public CCL::CommandDispatcher<LanguagePackBuilder>
{
public:
	DECLARE_CLASS (LanguagePackBuilder, ToolsComponent)

	LanguagePackBuilder ();

	// Commands
	DECLARE_COMMANDS (LanguagePackBuilder)
	DECLARE_COMMAND_CATEGORY ("Tools", ToolsComponent)

protected:
	CCL::AutoPtr<CCL::IUrl> outputPath;

	// ToolsComponent
	bool selectPath () override;
	void setup () override;
	void run () override;
	void cleanup () override;
};

//************************************************************************************************
// ContextConverter
/** Batch convert po files from legacy to po msgctxt format. */
//************************************************************************************************

class ContextConverter: public ToolsComponent,
						public CCL::CommandDispatcher<ContextConverter>
{
public:
	DECLARE_CLASS (ContextConverter, ToolsComponent)

	ContextConverter ();

	// Commands
	DECLARE_COMMANDS (ContextConverter)
	DECLARE_COMMAND_CATEGORY ("Tools", ToolsComponent)

protected:
	int succeeded;
	int failed;

	void processFolder (CCL::UrlRef path);
	void processFile (CCL::UrlRef path);

	// ToolsComponent
	bool selectPath () override;
	void setup () override;
	void run () override;
};

} // namespace Localizer

#endif // _localizer_tools_h
