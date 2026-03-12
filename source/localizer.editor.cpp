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
// Filename    : localizer.editor.cpp
// Description : Localizer Editing Component
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "localizer.editor.h"
#include "localizer.builder.h"
#include "editfunctions.h"

#include "ccl/app/params.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/guiservices.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (All, "All")
	XSTRING (Translated, "Translated")
	XSTRING (Untranslated, "Untranslated")
	XSTRING (Orphaned, "Orphaned")
	XSTRING (TranslationWarnings, "Warnings")
	XSTRING (AskCleanup, "Are you sure you want to remove all orphaned translations?\n\nThis action cannot be undone.")
	XSTRING (ItemNotDisplayed, "This item is currently not displayed.")
	XSTRING (AskResetFilter, "Do you want to reset the view filter?")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ProjectComponentTags
	{
		kSortBy = 100,
		kStringMode,
		kStringCount,
		kSearchString,
		kSearchMode,
		kSearchExactMatch,
		kSearchCaseSensitive
	};

	enum StringInspectorTags
	{
		kKeyString = 100,
		kTranslation
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (ProjectComponent)
	DEFINE_COMMAND ("Edit", "Copy",  ProjectComponent::onEditCopy)
	DEFINE_COMMAND ("Edit", "Paste",  ProjectComponent::onEditPaste)
	DEFINE_COMMAND ("Edit", "Delete",  ProjectComponent::onEditDelete)
	DEFINE_COMMAND ("Edit", "Cleanup", ProjectComponent::onEditCleanup)
	DEFINE_COMMAND ("Edit", "Set To Original", ProjectComponent::onEditSetOriginal)
	DEFINE_COMMAND ("Pseudo", nullptr,  ProjectComponent::onPseudoLocalization)
	DEFINE_COMMAND ("Edit", "Strip Context",  ProjectComponent::onStripContext)
	DEFINE_COMMAND ("Edit", "Autocomplete",  ProjectComponent::onAutoComplete)
	DEFINE_COMMAND ("Navigation", "Next Item", ProjectComponent::onNavigate)
	DEFINE_COMMAND ("Navigation", "Previous Item", ProjectComponent::onNavigate)
	DEFINE_COMMAND ("View", "Filter by Context", ProjectComponent::onFilter)
	DEFINE_COMMAND ("View", "Filter by File", ProjectComponent::onFilter)
	DEFINE_COMMAND ("View", "Reset Filter", ProjectComponent::onResetFilter)
END_COMMANDS (ProjectComponent)

//************************************************************************************************
// ProjectComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ProjectComponent, Component)
IMPLEMENT_COMMANDS (ProjectComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectComponent::ProjectComponent (Project& project)
: Component (String ("Editor")),
  project (project),
  fileList (nullptr),
  scopeList (nullptr),
  stringList (nullptr),
  occuranceList (nullptr),
  searchResults (nullptr),
  inspector (nullptr),
  projectUpdatesSuspended (false),
  stringMode (kShowAll)
{
	project.retain ();
	project.addObserver (this);

	fileList = NEW FileList (*this);
	fileList->rebuild ();
	fileList->addObserver (this);

	scopeList = NEW ScopeList (*this);
	scopeList->rebuild ();
	scopeList->addObserver (this);

	stringList = NEW TranslationList (*this);
	stringList->rebuild ();

	searchResults = NEW SearchResultList (*this);

	occuranceList = NEW OccuranceList (*this);

	// sort options
	ListParam* sortBy = unknown_cast<ListParam> (paramList.addList (CSTR ("sortBy"), Tag::kSortBy));
	sortBy->appendObject (return_shared (stringList->getSorterWithID (TranslationList::kTitleID)));
	sortBy->appendObject (return_shared (stringList->getSorterWithID (TranslationList::kScopeID)));
	sortBy->appendObject (return_shared (stringList->getSorterWithID (TranslationList::kFileID)));
	//sortBy->setStorable (true); saved by ListView

	// show options
	ListParam* stringMode = unknown_cast<ListParam> (paramList.addList (CSTR ("stringMode"), Tag::kStringMode));
	stringMode->appendString (XSTR (All)); // kShowAll
	stringMode->appendString (XSTR (Translated)); // kShowTranslated
	stringMode->appendString (XSTR (Untranslated)); // kShowUntranslated
	stringMode->appendString (XSTR (Orphaned)); // kShowOrphaned
	stringMode->appendString (String () << XSTR (Untranslated) << " + " << XSTR (Orphaned)); // kShowUntranslatedAndOrphaned
	stringMode->appendString (XSTR (TranslationWarnings)); // kShowTranslationWarnings
	stringMode->setStorable (true);

	paramList.addInteger (0, NumericLimits::kMaxInt, CSTR ("stringCount"), Tag::kStringCount);

	// commands
	paramList.addCommand ("Edit", "Set To Original", CSTR ("setToOriginal"));
	paramList.addCommand ("Navigation", "Next Item", CSTR ("nextItem"));
	paramList.addCommand ("Navigation", "Previous Item", CSTR ("prevItem"));

	// inspector
	addComponent (inspector = NEW StringInspector);

	// search
	paramList.addString (CSTR ("searchString"), Tag::kSearchString);
	ListParam* searchMode = unknown_cast<ListParam> (paramList.addList (CSTR ("searchMode"), Tag::kSearchMode));
	searchMode->appendString (XSTR (All)); // SearchOptions::kAll
	searchMode->appendString (TranslationList::KeyString ()); // SearchOptions::kKey
	searchMode->appendString (TranslationList::Translation ()); // SearchOptions::kTranslation
	searchMode->appendString (TranslationList::ScopeName ()); // SearchOptions::kScope
	searchMode->setStorable (true);
	paramList.addParam (CSTR ("searchExactMatch"), Tag::kSearchExactMatch)->setStorable (true);
	paramList.addParam (CSTR ("searchCaseSensitive"), Tag::kSearchCaseSensitive)->setStorable (true);
	
	paramList.checkCommandStates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectComponent::~ProjectComponent ()
{
	searchResults->release ();

	occuranceList->release ();
	stringList->release ();

	scopeList->removeObserver (this);
	scopeList->release ();

	fileList->removeObserver (this);
	fileList->release ();

	project.removeObserver (this);
	project.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ProjectComponent::initialize (IUnknown* context)
{
	SuperClass::initialize (context);

	paramList.restoreSettings (getName ());

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ProjectComponent::terminate ()
{
	paramList.storeSettings (getName ());

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Project& ProjectComponent::getProject ()
{
	return project;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringInspector& ProjectComponent::getInspector ()
{
	ASSERT (inspector != nullptr)
	return *inspector;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::getSearchOptions (SearchOptions& options) const
{
	String string;
	paramList.byTag (Tag::kSearchString)->toString (string);
	options.setSearchTerms (string);
	options.setMode ((SearchOptions::Mode)paramList.byTag (Tag::kSearchMode)->getValue ().asInt ());
	options.setExactMatch (paramList.byTag (Tag::kSearchExactMatch)->getValue ());
	options.setCaseSensitive (paramList.byTag (Tag::kSearchCaseSensitive)->getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::collectStrings (ObjectArray& entries, bool filterDuplicates) const
{
	auto contains = [&] (StringRef keyString)
	{
		ArrayForEach (entries, StringEntry, entry)
			if(entry->getKeyString () == keyString)
				return true;
		EndFor
		return false;
	};

	for(int i = 0; i < stringList->countFlatItems (); i++)
	{
		TranslationList::StringItem* item = (TranslationList::StringItem*)stringList->getItem (i);
		StringEntry* entry = item->getEntry ();

		if(filterDuplicates && contains (entry->getKeyString ()))
			continue;

		entries.add (entry);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectComponent::SuspendGuard::SuspendGuard (ProjectComponent& c, bool state)
: c (c)
{
	c.projectUpdatesSuspended = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectComponent::SuspendGuard::~SuspendGuard ()
{
	if(c.projectUpdatesSuspended)
	{
		c.projectUpdatesSuspended = false;

		String savedFilter; // keep filter!
		c.storeFilter (savedFilter);
		c.project.signal (Message (kChanged));
		c.restoreFilter (savedFilter);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::importTranslations (const Container& entries)
{
	SuspendGuard guard (*this, entries.count () > 500); // suspend for large imports

	EditFunctions functions (project);
	functions.beginMultiple (0);
	ForEach (entries, StringEntry, entry)
		if(entry->isOrphaned () || entry->getTranslation ().isEmpty ())
			continue;
		StringEntry* existing = project.findEqual (*entry);
		if(!existing) // second try ignoring scope
			existing = project.findEqual (*entry, true);
		if(existing)
			functions.editTranslation (*existing, entry->getTranslation ());
		#if DEBUG_LOG
		else if(!projectUpdatesSuspended)
		{
			CCL_PRINT ("Import failed for: ")
			CCL_PRINTLN (entry->getKeyString ())
		}
		#endif
	EndFor
	functions.endMultiple ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ProjectComponent::getObject (StringID name, UIDRef classID)
{
	if(name == "fileList")
		return ccl_as_unknown (fileList);
	else if(name == "scopeList")
		return ccl_as_unknown (scopeList);
	else if(name == "stringList")
		return ccl_as_unknown (stringList);
	else if(name == "occuranceList")
		return ccl_as_unknown (occuranceList);
	else if(name == "searchResults")
		return ccl_as_unknown (searchResults);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProjectComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kSortBy :
		if(ListParam* list = unknown_cast<ListParam> (param))
			if(ListViewSorter* sorter = list->getObject<ListViewSorter> (list->getValue ()))
				sortBy (sorter->getID ());
		break;

	case Tag::kStringMode :
		{
			FocusKeeper focusKeeper (stringList);

			stringMode = (StringMode)param->getValue ().asInt ();
			stringList->rebuild ();
			updateParams ();
		}
		break;

	case Tag::kSearchString :
	case Tag::kSearchMode :
	case Tag::kSearchExactMatch :
	case Tag::kSearchCaseSensitive :
		searchResults->rebuild ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProjectComponent::notify (ISubject* subject, MessageRef msg)
{
	// filter changed
	if(subject == fileList || subject == scopeList)
	{
		if(msg == BasicList::kItemChecked)
		{
			FocusKeeper focusKeeper (stringList);

			if(subject == fileList)
				scopeList->rebuild ();
			
			stringList->rebuild ();

			updateParams ();
		}
	}
	// project changed
	else if(subject == &project)
	{
		if(projectUpdatesSuspended)
			return;

		if(msg == kChanged)
		{
			// rebuild everything
			fileList->rebuild ();
			scopeList->rebuild ();
			stringList->rebuild ();
			searchResults->rebuild ();
			occuranceList->rebuild ();

			updateParams ();
		}
		else if(msg == Project::kStringChanged)
		{
			FocusKeeper focusKeeper (stringList);

			StringEntry* entry = unknown_cast<StringEntry> (msg[0].asUnknown ());
			ASSERT (entry != nullptr)

			if(stringMode != kShowAll)
				stringList->rebuild ();
			else
				stringList->invalidate ();

			fileList->invalidate ();
			occuranceList->rebuild ();
			searchResults->rebuild ();

			if(entry == inspector->getCurrentEntry ())
				inspector->refresh ();

			updateParams ();
		}
		else if(msg == Project::kFileChanged)
		{
			FileEntry* entry = unknown_cast<FileEntry> (msg[0].asUnknown ());
			ASSERT (entry != nullptr)

			fileList->invalidate ();
		}
		else if(msg == Project::kSaved)
		{
			FocusKeeper focusKeeper (stringList);

			// project has been saved => remove modified entries from filtered list
			stringList->rebuild ();

			// invalidate hilites of modified strings
			occuranceList->invalidate ();
			searchResults->invalidate ();

			updateParams ();
		}
		else if(msg == Project::kErrorsChanged)
		{
			// error state of files/string entries has changed => invalidate lists
			fileList->invalidate ();
			stringList->invalidate ();
			occuranceList->invalidate ();
			searchResults->invalidate ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::updateParams ()
{
	paramList.byTag (Tag::kStringCount)->setValue (stringList->countFlatItems ());

	paramList.checkCommandStates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::focusEntry (StringEntry* entry)
{
	IItemView* itemView = stringList->getItemView ();
	if(!itemView)
		return false;

	int index = stringList->getIndexForEntry (entry);
	if(index == -1)
	{
		String text;
		text << XSTR (ItemNotDisplayed) << "\n\n" << XSTR (AskResetFilter);
		if(Alert::ask (text) == Alert::kYes)
		{
			paramList.byTag (Tag::kStringMode)->setValue (0, true); // set string mode to all
			resetFilter ();
			index = stringList->getIndexForEntry (entry);
		}
	}

	if(index != -1)
	{
		itemView->setFocusItem (ItemIndex (index));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ProjectComponent::getFocusEntryIndex () const
{
	if(IItemView* itemView = stringList->getItemView ())
	{
		ItemIndex focusItem;
		if(itemView->getFocusItem (focusItem))
			return focusItem.getIndex ();
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::focusEntryAtIndex (int index)
{
	IItemView* itemView = stringList->getItemView ();
	if(!itemView)
		return false;
	if(index < 0 || index >= stringList->countFlatItems ())
		return false;
	return itemView->setFocusItem (ItemIndex (index)) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::setFilter (const Container& files, const StringList& scopes)
{
	ListViewModel* changedList = nullptr;
	int focusIndex = -1;

	if(!files.isEmpty ())
	{
		int index = 0;
		ForEach (*fileList, FileList::FileItem, item)
			bool checked = files.contains (item->getEntry ());
			if(checked != item->isChecked ())
			{
				item->setChecked (checked);
				changedList = fileList;
			}			
			if(checked && focusIndex == -1)
				focusIndex = index;
			index++;
		EndFor
	}
	else if(!scopes.isEmpty ())
	{
		int index = 0;
		ForEach (*scopeList, ScopeList::ScopeItem, item)
			bool checked = scopes.contains (*item->getEntry ());
			if(checked != item->isChecked ())
			{
				item->setChecked (checked);
				changedList = scopeList;
			}
			if(checked && focusIndex == -1)
				focusIndex = index;
			index++;
		EndFor
	}
		
	if(changedList)
	{
		changedList->invalidate ();
		changedList->signal (Message (ListViewModel::kItemChecked));

		if(focusIndex != -1)
			if(IItemView* itemView = changedList->getItemView ())
				itemView->setFocusItem (ItemIndex (focusIndex));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::restoreFilter (StringRef fileNames)
{
	bool changed = false;
	ForEach (*fileList, FileList::FileItem, item)
		bool checked = !fileNames.contains (item->getEntry ()->getFileName ());
		if(checked != item->isChecked ())
			changed = true;
		item->setChecked (checked);
	EndFor

	if(changed)
		fileList->signal (Message (FileList::kItemChecked));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::storeFilter (String& fileNames) const
{
	StringList list;
	ForEach (*fileList, FileList::FileItem, item)
		if(item->isChecked () == false)
			list.add (item->getEntry ()->getFileName ());
	EndFor
	fileNames = list.concat (CCLSTR (";"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::resetFilter ()
{
	fileList->checkAll (true);
	scopeList->checkAll (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::sortBy (StringID id)
{
	ListViewSorter* sorter = stringList->getSorterWithID (id);
	ASSERT (sorter != nullptr)
	stringList->sortBy (sorter);

	// search results respect the same sorting
	searchResults->sortBy (searchResults->getSorterWithID (id));

	// sync parameter
	ListParam* list = paramList.byTag<ListParam> (Tag::kSortBy);
	int index = list->getObjectIndex (sorter);
	ASSERT (index != -1)
	list->setValue (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectComponent::onFocusItemChanged (BasicList* list)
{
	//CCL_PRINTLN ("Focus item changed")

	if(list == stringList)
	{
		TranslationList::StringItem* item = (TranslationList::StringItem*)stringList->getFocusItem ();
		StringEntry* entry = item ? item->getEntry () : nullptr;
		if(inspector->setCurrentEntry (entry))
		{
			occuranceList->rebuild ();
			paramList.checkCommandStates ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::matchesScope (const ScopeEntry& entry) const
{
	if(fileList)
		if(fileList->isAnyFileWithScopeChecked (entry))
			return true;

	return false; // default is off
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::matchesString (const StringEntry& entry) const
{
	if(!matchesMode (entry))
		return false;

	bool fileChecked = false;
	if(fileList)
		if(ListViewItem* fileItem = fileList->findFileItem (entry.getFile ()))
			fileChecked = fileItem->isChecked ();

	if(!fileChecked)
		return false;

	if(scopeList)
		if(ListViewItem* scopeItem = scopeList->findScopeItem (entry.getScopeName ()))
			if(scopeItem->isChecked ())
				return true;

	return false; // default is off
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::matchesMode (const StringEntry& entry) const
{
	// Note: modified strings are kept in list until next save!
	switch(stringMode)
	{
	case kShowTranslated : return entry.isTranslated () || entry.isModified ();
	case kShowUntranslated : return !entry.isTranslated () || entry.isModified ();
	case kShowOrphaned : return entry.isOrphaned ();
	case kShowUntranslatedAndOrphaned : return !entry.isTranslated () || entry.isModified () || entry.isOrphaned ();
	case kShowTranslationWarnings : return entry.hasTranslationWarnings ();
	}
	return true; // kShowAll
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onEditCopy (CmdArgs args)
{
	StringEntry* entry = inspector->getCurrentEntry ();
	if(entry == nullptr)
		return false;

	if(!args.checkOnly ())
	{
		CCL_PRINTLN ("Entry copied")
		System::GetClipboard ().setText (entry->getTranslation ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onEditPaste (CmdArgs args)
{
	StringEntry* entry = inspector->getCurrentEntry ();
	if(entry == nullptr)
		return false;

	//if(System::GetClipboard ().isEmpty ())
	//	return false;

	if(!args.checkOnly ())
	{
		String text;
		if(System::GetClipboard ().getText (text))
		{
			CCL_PRINTLN ("Entry pasted")
			EditFunctions (project).editTranslation (*entry, text);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onEditDelete (CmdArgs args)
{
	ObjectArray selected;
	stringList->getSelectedItems (selected);
	if(selected.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		EditFunctions functions (project);
		functions.beginMultiple (0);

		ArrayForEach (selected, StringEntryItem, item)
			StringEntry* entry = item->getEntry ();
			functions.editTranslation (*entry, String::kEmpty); 
		EndFor

		functions.endMultiple ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onEditCleanup (CmdArgs args)
{
	if(project.isEmpty ())
		return false;

	if(!args.checkOnly ())
		if(Alert::ask (XSTR (AskCleanup)) == Alert::kYes)
			EditFunctions (project).removeOrphanedEntries ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onEditSetOriginal (CmdArgs args)
{
	ObjectArray selected;
	stringList->getSelectedItems (selected);
	if(selected.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		EditFunctions functions (project);
		functions.beginMultiple (0);

		ArrayForEach (selected, StringEntryItem, item)
			StringEntry* entry = item->getEntry ();
			functions.editTranslation (*entry, entry->getKeyString ()); 
		EndFor

		functions.endMultiple ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onPseudoLocalization (CmdArgs args)
{
	bool all = args.name.contains ("Batch");
	bool useKeyString = args.name.contains ("Original");

	if(all)
	{
		if(stringList->countFlatItems () == 0)
			return false;

		if(!args.checkOnly ())
		{
			EditFunctions functions (project);
			functions.beginMultiple (0);

			for(int i = 0; i < stringList->countFlatItems (); i++)
			{
				TranslationList::StringItem* item = (TranslationList::StringItem*)stringList->getItem (i);
				StringEntry* entry = item->getEntry ();
				functions.pseudoizeTranslation (*entry, useKeyString);
			}

			functions.endMultiple ();
		}
	}
	else
	{
		StringEntry* entry = inspector->getCurrentEntry ();
		if(entry == nullptr)
			return false;

		if(!args.checkOnly ())
			EditFunctions (project).pseudoizeTranslation (*entry, useKeyString);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onStripContext (CmdArgs args)
{
	if(stringList->countFlatItems () == 0)
		return false;

	if(!args.checkOnly ())
	{
		EditFunctions functions (project);
		functions.beginMultiple (0);

		for(int i = 0; i < stringList->countFlatItems (); i++)
		{
			TranslationList::StringItem* item = (TranslationList::StringItem*)stringList->getItem (i);
			StringEntry* entry = item->getEntry ();
			if(!entry->getScopeName ().isEmpty ())
			{
				String escapedScope;
				escapedScope << "[" << entry->getScopeName () << "]";
				bool hasScope = entry->getTranslation().startsWith (escapedScope);
				if(!hasScope && entry->getTranslation ().startsWith ("["))
				{
					int endIndex = entry->getTranslation ().subString (1).index ("]");
					if(endIndex != -1)
					{
						escapedScope = entry->getTranslation ().subString (0, endIndex + 2);
						hasScope = true;
					}					
				}

				if(hasScope)
				{
					String newTranslation (entry->getTranslation ());
					newTranslation.remove (0, escapedScope.length ());
					functions.editTranslation (*entry, newTranslation);
				}
			}
		}

		functions.endMultiple ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onAutoComplete (CmdArgs args)
{
	if(stringList->countFlatItems () == 0)
		return false;

	if(!args.checkOnly ())
	{
		SuspendGuard guard (*this, true); // suspend updates

		EditFunctions functions (project);
		functions.beginMultiple (0);

		for(int i = 0; i < stringList->countFlatItems (); i++)
		{
			TranslationList::StringItem* item = (TranslationList::StringItem*)stringList->getItem (i);
			StringEntry* entry = item->getEntry ();
			if(entry->getTranslation ().isEmpty ())
			{
				String suggestion = project.findFirstSuggestion (*entry);
				if(!suggestion.isEmpty ())
					functions.editTranslation (*entry, suggestion);
			}
		}

		functions.endMultiple ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onNavigate (CmdArgs args)
{
	IItemView* itemView = stringList->getItemView ();
	if(!itemView)
		return false;

	ItemIndex current;
	if(!itemView->getFocusItem (current))
		return false;

	int direction = args.name.startsWith ("Next") ? +1 : -1;
	int newIndex = current.getIndex () + direction;
	newIndex = ccl_bound (newIndex, 0, stringList->countFlatItems ()-1);	
	if(newIndex == current.getIndex ())
		return false;
	
	if(!args.checkOnly ())
		itemView->setFocusItem (ItemIndex (newIndex));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onFilter (CmdArgs args)
{
	ObjectArray selected;
	stringList->getSelectedItems (selected);
	if(selected.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		bool byFiles = args.name.contains ("File");

		ObjectArray files;
		StringList scopeNames;
		ArrayForEach (selected, StringEntryItem, item)
			if(byFiles)
			{
				FileEntry* file = item->getEntry ()->getFile ();
				if(!files.contains (file))
					files.add (file);
			}
			else
				scopeNames.addOnce (item->getEntry ()->getScopeName ());
		EndFor

		setFilter (files, scopeNames);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectComponent::onResetFilter (CmdArgs args)
{
	if(fileList->isEmpty ())
		return false;
	if(!args.checkOnly ())
		resetFilter ();
	return true;
}

//************************************************************************************************
// StringInspector
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringInspector, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringInspector::StringInspector ()
: Component (String ("Inspector"))
{
	paramList.addString (CSTR ("keyString"), Tag::kKeyString);
	paramList.addString (CSTR ("translation"), Tag::kTranslation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringEntry* StringInspector::getCurrentEntry () const
{
	return currentEntry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringInspector::setCurrentEntry (StringEntry* entry)
{
	if(entry == currentEntry)
		return false;

	currentEntry = entry;
	refresh ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringInspector::refresh ()
{
	bool enabled = currentEntry != nullptr;

	String keyString, translation;
	if(currentEntry)
	{
		keyString = currentEntry->getKeyString ();
		translation = currentEntry->getTranslation ();
	}

	paramList.byTag (Tag::kKeyString)->fromString (keyString);
	IParameter* p = paramList.byTag (Tag::kTranslation);
	p->fromString (translation);
	p->enable (enabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringInspector::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kTranslation :
		if(currentEntry)
		{
			String string;
			param->toString (string);

			Project* project = currentEntry->getFile ()->getProject ();
			EditFunctions (*project).editTranslation (*currentEntry, string);
		}
		break;
	}
	return true;
}

//************************************************************************************************
// BasicList
//************************************************************************************************

BasicList::BasicList (ProjectComponent& component)
: component (component)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BasicList::onSelectionChanged ()
{
	component.onFocusItemChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BasicList::onItemFocused (ItemIndexRef index) 
{
	component.onFocusItemChanged (this);
	return true;
}
