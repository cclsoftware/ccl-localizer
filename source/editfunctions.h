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
// Filename    : editfunctions.h
// Description : Localizer Edit Functions
//
//************************************************************************************************

#ifndef _localizer_editfunctions_h
#define _localizer_editfunctions_h

#include "ccl/app/actions/actionexecuter.h"

namespace Localizer {

class Project;
class StringEntry;

//************************************************************************************************
// EditFunctions
//************************************************************************************************

class EditFunctions: public CCL::ActionExecuter
{
public:
	EditFunctions (Project& project);

	bool editTranslation (StringEntry& entry, CCL::StringRef newTranslation);
	
	bool pseudoizeTranslation (StringEntry& entry, bool useKeyString);

	bool removeOrphanedEntries ();

protected:
	Project& project;
};

} // namespace Localizer

#endif // _localizer_editfunctions_h
