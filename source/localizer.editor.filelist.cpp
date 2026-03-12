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
// Filename    : localizer.editor.filelist.cpp
// Description : File List
//
//************************************************************************************************

#include "localizer.editor.h"

#include "ccl/app/utilities/fileicons.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/ibitmapfilter.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (File, "File")
	XSTRING (Items, "Items")
END_XSTRINGS

//************************************************************************************************
// FileList::FileItem
//************************************************************************************************
		
bool FileList::FileItem::getDetail (Variant& value, StringID id) const
{
	String string;
	if(id == kItemsID)
		string << entry->getTranslatedCount () << " / " << entry->getStringCount ();
	value = Variant (string, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileList::FileItem::drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment)
{
	if(id == kFileIconID)
	{
		static AutoPtr<IBitmapPainter> painter = ccl_new<IBitmapPainter> (ClassID::BitmapPainter);
		ASSERT (painter != nullptr)

		bool modified = entry->isModified ();
		bool allDone = entry->getTranslatedCount () == entry->getStringCount ();
		bool hasErrors = !entry->getErrors ().isEmpty ();
		
		Color color = hasErrors ? Colors::kRed : allDone ? Colors::kGreen : Colors::kYellow;

		Rect iconRect;
		if(icon)
		{
			icon->setCurrentFrame (icon->getFrameIndex ("small"));
			Rect srcRect (0, 0, icon->getWidth (), icon->getHeight ());
			iconRect = srcRect;
			iconRect.center (info.rect);
			painter->drawTinted (info.graphics, icon, srcRect, iconRect, color);
		}
		else
		{
			iconRect (0, 0, 16, 16);
			iconRect.center (info.rect);
			info.graphics.fillRect (iconRect, SolidBrush (color));
		}

		if(modified)
			info.graphics.drawString (iconRect, CCLSTR ("*"), info.style.font, info.style.textBrush, Alignment::kCenter);
		return true;
	}
	else
		return ListViewItem::drawDetail (info, id, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileList::FileItem::getTooltip (String& tooltip, StringID id)
{
	tooltip.empty ();
	if(!entry->getErrors ().isEmpty ())
		for(int i = 0, count = entry->getErrors ().count (); i < count; i++)
			tooltip << entry->getErrors ()[i] << String::getLineEnd ();
	return true;
}

//************************************************************************************************
// FileList
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (FileList, kFileIconID, "fileicon")
DEFINE_STRINGID_MEMBER_ (FileList, kItemsID, "items")

//////////////////////////////////////////////////////////////////////////////////////////////////

FileList::FileList (ProjectComponent& component)
: BasicList (component)
{
	int flags = IColumnHeaderList::kSizable|IColumnHeaderList::kMoveable;

	getColumns ().addColumn (24, 0, kCheckBoxID);
	getColumns ().addColumn (20, 0, kFileIconID);
	getColumns ().addColumn (170, XSTR (File), kTitleID, 40, flags);
	getColumns ().addColumn (60, XSTR (Items), kItemsID, 30, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileList::rebuild ()
{
	removeAll ();

	IterForEach (component.getProject ().getFiles (), FileEntry, entry)
		FileItem* item = NEW FileItem;
		item->setEntry (entry);
		item->setTitle (entry->getFileName ());
		AutoPtr<IImage> icon = FileIcons::instance ().createIcon (entry->getPath ());
		item->setIcon (icon);
		item->setChecked (true);
		addItem (item);
	EndFor

	resort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	FileItem* item = (FileItem*)resolve (index);
	if(!item)
		return false;

	checkAlone (index.getIndex (), true/*!item->isChecked ()*/);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileList::appendItemMenu (IContextMenu& menu, ItemIndexRef index, const IItemSelection& selection)
{
	menu.addCommandItem (TranslationList::ResetFilter (), "View", "Reset Filter", &component);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileList::FileItem* FileList::findFileItem (const FileEntry* entry) const
{
	ArrayForEach (items, FileItem, item)
		if(item->getEntry () == entry)
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileList::isAnyFileWithScopeChecked (CCL::StringRef scopeName) const
{
	ArrayForEach (items, FileItem, item)
		if(item->isChecked ())
			if(item->getEntry ()->hasScopeName (scopeName))
				return true;
	EndFor
	return false;
}
