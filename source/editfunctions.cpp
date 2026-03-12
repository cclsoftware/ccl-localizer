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
// Filename    : editfunctions.cpp
// Description : Localizer Edit Functions
//
//************************************************************************************************

#include "editfunctions.h"
#include "localizer.model.h"
#include "pseudotable.h"

#include "ccl/app/actions/action.h"

#include "ccl/base/message.h"

#include "ccl/public/text/translation.h"

namespace Localizer {

//************************************************************************************************
// EditTranslationAction
//************************************************************************************************

class EditTranslationAction: public CCL::Action
{
public:
	EditTranslationAction (StringEntry* entry = nullptr, CCL::StringRef newTranslation = 0);

	PROPERTY_STRING (newTranslation, NewTranslation)

protected:
	CCL::SharedPtr<StringEntry> entry;
	bool modifiedState;

	// Action
	bool execute () override;
};

} // namespace Localizer

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (EditTranslation, "Edit Translation")
END_XSTRINGS

//************************************************************************************************
// EditFunctions
//************************************************************************************************

EditFunctions::EditFunctions (Project& project)
: ActionExecuter (&project),
  project (project)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditFunctions::editTranslation (StringEntry& entry, StringRef newTranslation)
{
	return execute (NEW EditTranslationAction (&entry, newTranslation));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditFunctions::pseudoizeTranslation (StringEntry& entry, bool useKeyString)
{
	String string;
	if(useKeyString)
	{
		string = entry.getKeyString ();
		
		// strip scope
		if(string.startsWith ("["))
			string = string.subString (string.index ("]") + 1);
	}
	else
		string = entry.getTranslation ();

	string = PseudoTable::instance ().getPseudoString (string);

	return editTranslation (entry, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditFunctions::removeOrphanedEntries ()
{
	bool changed = false;
	ObjectArray modifiedFiles;

	IterForEach (project.getStrings (), StringEntry, entry)
		if(entry->isOrphaned ())
		{
			if(!modifiedFiles.contains (entry->getFile ()))
				modifiedFiles.add (entry->getFile ());

			entry->getFile ()->removeString (entry);
			project.removeString (entry);
			changed = true;

			__iter->previous (); // (problem with array iterator when removing items)
		}
	EndFor

	if(changed)
	{
		// update stats
		ArrayForEach (modifiedFiles, FileEntry, file)
			file->setModified (true);
			file->updateStats ();
		EndFor
		
		project.signal (Message (kChanged));
	}
	return changed;
}

//************************************************************************************************
// EditTranslationAction
//************************************************************************************************

EditTranslationAction::EditTranslationAction (StringEntry* entry, StringRef newTranslation)
: Action (XSTR (EditTranslation)),
  newTranslation (newTranslation),
  entry (entry),
  modifiedState (true)
{
	ASSERT (entry != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTranslationAction::execute ()
{
	if(entry->getTranslation () == newTranslation)
		return false;

	String oldTranslation (entry->getTranslation ());
	entry->setTranslation (newTranslation);
	newTranslation = oldTranslation;

	bool oldState = entry->isModified ();
	entry->setModified (modifiedState);
	modifiedState = oldState;

	entry->getFile ()->setModified (true);
	entry->getFile ()->updateStats ();
	entry->updateTranslationWarnings ();

	Project* project = entry->getFile ()->getProject ();
	project->signal (Message (Project::kStringChanged, entry->asUnknown ()));
	return true;
}
