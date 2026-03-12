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
// Filename    : localizer.editor.stringlist.cpp
// Description : Translation List
//
//************************************************************************************************

#include "localizer.editor.h"
#include "editfunctions.h"

#include "ccl/app/params.h"

#include "ccl/public/text/translation.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/idragndrop.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (ScopeName, "Context")
	XSTRING (KeyString, "Original")
	XSTRING (Translation, "Translation")
	XSTRING (File, "File")
	XSTRING (Copy, "Copy")
	XSTRING (Paste, "Paste")
	XSTRING (Delete, "Delete")
	XSTRING (SetToOriginal, "Set To Original")
	XSTRING (FilterByContext, "Filter by Context")
	XSTRING (FilterByFile, "Filter by File")
	XSTRING (ResetFilter, "Reset Filter")
END_XSTRINGS

StringRef TranslationList::KeyString ()		{ return XSTR (KeyString); }
StringRef TranslationList::Translation ()	{ return XSTR (Translation); }
StringRef TranslationList::ScopeName ()		{ return XSTR (ScopeName); }

StringRef TranslationList::ResetFilter ()	{ return XSTR (ResetFilter); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Sort Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

#define COMPARE_STRINGITEM_ENTRY(FunctionName, e1, e2) \
DEFINE_ARRAY_COMPARE (FunctionName, TranslationList::StringItem, item1, item2) \
 StringEntry* e1 = item1->getEntry (); \
 StringEntry* e2 = item2->getEntry ();

COMPARE_STRINGITEM_ENTRY (SortByScopeName, e1, e2)
	return e1->getScopeName ().compare (e2->getScopeName ()); 
}

COMPARE_STRINGITEM_ENTRY (SortByKeyString, e1, e2)
	return e1->getKeyString ().compare (e2->getKeyString ()); 
}

/*COMPARE_STRINGITEM_ENTRY (SortByTranslation, e1, e2)
	return e1->getTranslation ().compare (e2->getTranslation ()); 
}*/

COMPARE_STRINGITEM_ENTRY (SortByFileName, e1, e2)
	if(e1->getFile ()->getFileName () == e2->getFile ()->getFileName ())
	{
		if(e1->getLineNumber () > 0 && e2->getLineNumber () > 0)
			return e1->getLineNumber () - e2->getLineNumber ();
	}
	return e1->getFile ()->getFileName ().compare (e2->getFile ()->getFileName ()); 
}

//************************************************************************************************
// StringEntryItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringEntryItem, ListViewItem)

//************************************************************************************************
// StringEntryList
//************************************************************************************************

StringEntryList::StringEntryList (ProjectComponent& component)
: BasicList (component),
  hiliteColumnID (kTitleID)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StringEntryList::getIndexForEntry (const StringEntry* entry) const
{
	int index = 0;
	ArrayForEach (items, StringEntryItem, item)
		if(item->getEntry () == entry)
			return index;
		index++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringEntryList::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	StringEntryItem* item = (StringEntryItem*)resolve (index);
	if(!item)
		return false;

	CString columnID;
	ColumnType columnType = getColumnType (columnID, column);
	if(columnID == hiliteColumnID)
	{
		if(item->getEntry ()->isOrphaned () || item->getEntry ()->isModified ())
		{
			SolidBrush textBrush (item->getEntry ()->isOrphaned () ? Colors::kRed : Colors::kBlue);
			String title;

			if(columnID == kTitleID) 
				title = item->getTitle ();
			else
			{
				Variant var;
				item->getDetail (var, columnID);
				title = var.asString ();
			}

			Font::collapseString (title, info.rect.getWidth (), info.style.font);
			info.graphics.drawString (info.rect, title, info.style.font, textBrush, Alignment::kLeft|Alignment::kVCenter);
			return true;
		}
	}
	else if(columnID == errorColumnID && !item->getEntry ()->getErrorMessage ().isEmpty ())
	{
		// fill background
		SolidBrush brush (Color (Colors::kRed).setAlphaF (.33f));
		info.graphics.fillRect (info.rect, brush);
	}

	return BasicList::drawCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringEntryList::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	StringEntryItem* item = unknown_cast<StringEntryItem> (data.getFirst ());
	if(!item)
		return false;

	if(session)
	{
		UnknownPtr<IItemView> itemView (targetView);
		if(itemView)
		{
			int flags = IItemView::kCanDragOnItem|IItemView::kDropInsertsData;
			AutoPtr<IDragHandler> handler = itemView->createDragHandler (flags);
			session->setDragHandler (handler);
			session->setResult (IDragSession::kDropCopyReal);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringEntryList::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	StringEntryItem* item = unknown_cast<StringEntryItem> (data.getFirst ());
	if(!item)
		return false;

	StringEntryItem* target = (StringEntryItem*)resolve (index);
	if(!target)
		return false;

	if(item == target)
		return false;

	EditFunctions (component.getProject ()).editTranslation (*target->getEntry (), item->getEntry ()->getTranslation ());
	return true;
}

//************************************************************************************************
// TranslationList::StringItem
//************************************************************************************************

bool TranslationList::StringItem::getDetail (Variant& value, StringID id) const
{
	String string;
	if(id == kScopeID)
		string = entry->getScopeName ();
	else if(id == kTranslationID)
		string = entry->getTranslation ();
	else if(id == kFileID)
		string = entry->getFileAndLine ();
	value = Variant (string, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TranslationList::StringItem::getTooltip (String& tooltip, StringID id)
{
	tooltip.empty ();
	if(!entry->getErrorMessage ().isEmpty ())
		tooltip << entry->getErrorMessage () << String::getLineEnd ();
	if(entry->hasTranslationWarnings ())
		tooltip << entry->getTranslationWarnings () << String::getLineEnd ();
	tooltip << entry->getReferences ().concat (String::getLineEnd ());
	return true;
}

//************************************************************************************************
// TranslationList
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (TranslationList, kScopeID, "scope")
DEFINE_STRINGID_MEMBER_ (TranslationList, kTranslationID, "string")
DEFINE_STRINGID_MEMBER_ (TranslationList, kFileID, "file")

//////////////////////////////////////////////////////////////////////////////////////////////////

TranslationList::TranslationList (ProjectComponent& component)
: StringEntryList (component)
{
	ListViewSorter* defaultSorter = NEW ListViewSorter (kTitleID, XSTR (KeyString), &SortByKeyString);
	sorters.add (defaultSorter);
	sorters.add (NEW ListViewSorter (kScopeID, XSTR (ScopeName), &SortByScopeName));
	//sorters.add (NEW ListViewSorter (kTranslationID, XSTR (Translation), &SortByTranslation));
	sorters.add (NEW ListViewSorter (kFileID, XSTR (File), &SortByFileName));

	int flags = IColumnHeaderList::kSizable|IColumnHeaderList::kMoveable;
	int hidable = IColumnHeaderList::kHideable;
	int sortable = IColumnHeaderList::kSortable;
	int canFit = IColumnHeaderList::kCanFit;

	getColumns ().addColumn (100, XSTR (ScopeName), kScopeID, 50, flags|hidable|sortable|canFit);
	getColumns ().addColumn (250, XSTR (KeyString), kTitleID, 50, flags|sortable|canFit);
	getColumns ().addColumn (250, XSTR (Translation), kTranslationID, 50, flags|canFit);
	getColumns ().addColumn (100, XSTR (File), kFileID, 50, flags|hidable|sortable|canFit);

	setErrorColumnID (kTranslationID);

	sortBy (defaultSorter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TranslationList::rebuild ()
{
	removeAll ();

	IterForEach (component.getProject ().getStrings (), StringEntry, entry)
		if(!component.matchesString (*entry)) // apply filter
			continue;

		StringItem* item = NEW StringItem;
		item->setTitle (entry->getKeyString ());
		item->setEntry (entry);
		addItem (item);
	EndFor

	resort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TranslationList::onSortColumnChanged (StringID columnID, bool upwards)
{
	if(ListViewSorter* sorter = getSorterWithID (columnID))
		if(sorter != activeSorter)
			component.sortBy (sorter->getID ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TranslationList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	int editColumn = getColumnIndex (kTranslationID);
	Rect editRect;
	UnknownPtr<IItemView> (info.view)->getItemRect (editRect, index, editColumn); 

	const EditInfo info2 = {info.view, editRect, info.style, info.editEvent};
	return editCell (index, editColumn, info2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TranslationList::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	StringItem* item = (StringItem*)resolve (index);
	if(!item)
		return false;

	if(getColumns ().getColumnID (column) == kTranslationID)
	{
		AutoPtr<IParameter> param = NEW StringParam;
		param->fromString (item->getEntry ()->getTranslation ());
		param->connect (this, kEditTag + index.getIndex ());

		ControlBox editBox (CCL::ClassID::EditBox, param, info.rect, Styles::kBorder);
		UnknownPtr<IItemView> (info.view)->setEditControl (editBox);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TranslationList::paramChanged (IParameter* param)
{
	if(param->getTag () >= kEditTag)
	{
		int index = param->getTag () - kEditTag;
		if(StringItem* item = (StringItem*)getItem (index))
		{
			String text;
			param->toString (text);
			EditFunctions (component.getProject ()).editTranslation (*item->getEntry (), text);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TranslationList::appendItemMenu (IContextMenu& menu, ItemIndexRef index, const IItemSelection& selection)
{
	StringItem* item = (StringItem*)resolve (index);
	if(item)
	{
		menu.addCommandItem (XSTR (Copy), "Edit", "Copy", &component);
		menu.addCommandItem (XSTR (Paste), "Edit", "Paste", &component);
		menu.addCommandItem (XSTR (Delete), "Edit", "Delete", &component);
		menu.addCommandItem (XSTR (SetToOriginal), "Edit", "Set To Original", &component);
		menu.addSeparatorItem ();
		menu.addCommandItem (XSTR (FilterByContext), "View", "Filter by Context", &component);
		menu.addCommandItem (XSTR (FilterByFile), "View", "Filter by File", &component);
		menu.addSeparatorItem ();
	}
	menu.addCommandItem (XSTR (ResetFilter), "View", "Reset Filter", &component);
	return true;
}
