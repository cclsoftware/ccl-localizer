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
// Filename    : localizer.editor.scopelist.cpp
// Description : Scope List
//
//************************************************************************************************

#include "localizer.editor.h"

#include "ccl/public/text/translation.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (ScopeName, "Context")
END_XSTRINGS

//************************************************************************************************
// ScopeList
//************************************************************************************************

ScopeList::ScopeList (ProjectComponent& component)
: BasicList (component)
{
	getColumns ().addColumn (24, 0, kCheckBoxID);
	getColumns ().addColumn (200, XSTR (ScopeName), kTitleID, 50, IColumnHeaderList::kSizable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScopeList::rebuild ()
{
	removeAll ();

	IterForEach (component.getProject ().getScopes (), ScopeEntry, entry)
		if(!component.matchesScope (*entry)) // apply filter
			continue;

		ScopeItem* item = NEW ScopeItem;
		item->setEntry (entry);
		item->setTitle (*entry);
		item->setChecked (true);
		addItem (item);
	EndFor

	resort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScopeList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	ScopeItem* item = (ScopeItem*)resolve (index);
	if(!item)
		return false;

	checkAlone (index.getIndex (), true/*!item->isChecked ()*/);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScopeList::appendItemMenu (IContextMenu& menu, ItemIndexRef index, const IItemSelection& selection)
{
	menu.addCommandItem (TranslationList::ResetFilter (), "View", "Reset Filter", &component);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScopeList::ScopeItem* ScopeList::findScopeItem (StringRef scopeName) const
{
	ArrayForEach (items, ScopeItem, item)
		if(*item->getEntry () == scopeName)
			return item;
	EndFor
	return nullptr;
}
