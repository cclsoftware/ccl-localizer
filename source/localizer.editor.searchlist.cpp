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
// Filename    : localizer.editor.searchlist.cpp
// Description : Search Result List
//
//************************************************************************************************

#include "localizer.editor.h"

using namespace CCL;
using namespace Localizer;

//************************************************************************************************
// SearchOptions
//************************************************************************************************

SearchOptions::SearchOptions ()
: mode (kAll),
  exactMatch (false),
  caseSensitive (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchOptions::matches (const StringEntry& entry) const
{
	StringList candidates;
	switch(mode)
	{
	case kKey : candidates.add (entry.getKeyString ()); break;
	case kTranslation : candidates.add (entry.getTranslation ()); break;
	case kScope : candidates.add (entry.getScopeName ()); break;

	default : // kAll
		candidates.add (entry.getKeyString ());
		candidates.add (entry.getTranslation ());
		candidates.add (entry.getScopeName ());
		break;
	}

	for(int i = 0, count = candidates.count (); i < count; i++)
	{
		String string (candidates[i]);
		if(exactMatch)
		{
			if(string.compare (searchTerms, caseSensitive) == 0)
				return true;
		}
		else
		{
			if(string.contains (searchTerms, caseSensitive))
				return true;
		}
	}
	return false;
}

//************************************************************************************************
// SearchResultList
//************************************************************************************************

SearchResultList::SearchResultList (ProjectComponent& component)
: TranslationList (component)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultList::rebuild ()
{
	removeAll ();

	SearchOptions options;
	component.getSearchOptions (options);

	if(!options.getSearchTerms ().isEmpty ())
	{
		IterForEach (component.getProject ().getStrings (), StringEntry, entry)
			if(options.matches (*entry))
			{
				StringItem* item = NEW StringItem;
				item->setTitle (entry->getKeyString ());
				item->setEntry (entry);
				addItem (item);
			}
		EndFor
	}

	resort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	if(StringItem* item = (StringItem*)resolve (index))
	{
		component.focusEntry (item->getEntry ());
		return true;
	}
	return SuperClass::openItem (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::appendItemMenu (IContextMenu& menu, ItemIndexRef index, const IItemSelection& selection)
{
	// suppress context menu of base class
	return false;
}
