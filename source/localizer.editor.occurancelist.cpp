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
// Filename    : localizer.editor.occurancelist.cpp
// Description : Occurance List
//
//************************************************************************************************

#include "localizer.editor.h"

#include "ccl/base/message.h"

#include "ccl/public/text/translation.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (Translation, "Translation")
	XSTRING (Occurance, "Occurance")
END_XSTRINGS

//************************************************************************************************
// OccuranceList::OccuranceItem
//************************************************************************************************

bool OccuranceList::OccuranceItem::getDetail (Variant& value, StringID id) const
{
	String string;
	if(id == kTranslationID)
		string = entry->getTranslation ();
	else if(id == kOccuranceID)
		string  << "[" << entry->getScopeName () << "]@" << entry->getFileAndLine ();
	value = Variant (string, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OccuranceList::OccuranceItem::getTooltip (String& tooltip, StringID id)
{
	tooltip = entry->getReferences ().concat (String::getLineEnd ());
	return true;
}

//************************************************************************************************
// OccuranceList
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (OccuranceList, kTranslationID, "translation")
DEFINE_STRINGID_MEMBER_ (OccuranceList, kOccuranceID, "occurance")

//////////////////////////////////////////////////////////////////////////////////////////////////

OccuranceList::OccuranceList (ProjectComponent& component)
: StringEntryList (component)
{
	int flags = IColumnHeaderList::kSizable|IColumnHeaderList::kMoveable;
	getColumns ().addColumn (200, XSTR (Translation), kTranslationID, 50, flags);
	getColumns ().addColumn (150, XSTR (Occurance), kOccuranceID, 50, flags);

	setHiliteColumnID (kTranslationID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OccuranceList::rebuild ()
{
	removeAll ();

	if(StringEntry* entry = component.getInspector ().getCurrentEntry ())
	{
		ObjectArray occurances;
		component.getProject ().getOccurances (occurances, entry->getKeyString ());

		ArrayForEach (occurances, StringEntry, other)
			OccuranceItem* item = NEW OccuranceItem;
			item->setEntry (other);
			item->setTitle (other->getTranslation ());
			item->setEnabled (other != entry);
			addItem (item);
		EndFor
	}

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OccuranceList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	if(OccuranceItem* item = (OccuranceItem*)resolve (index))
	{
		component.focusEntry (item->getEntry ());
		return true;
	}
	return SuperClass::openItem (index, column, info);
}
