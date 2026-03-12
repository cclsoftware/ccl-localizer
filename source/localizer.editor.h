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
// Filename    : localizer.editor.h
// Description : Localizer Editing Component
//
//************************************************************************************************

#ifndef _localizer_editor_h
#define _localizer_editor_h

#include "localizer.model.h"

#include "ccl/app/component.h"

#include "ccl/app/controls/listviewmodel.h"

#include "ccl/public/gui/commanddispatch.h"

namespace Localizer {

class BasicList;
class FileList;
class ScopeList;
class TranslationList;
class OccuranceList;
class SearchResultList;
class StringInspector;

//************************************************************************************************
// SearchOptions
//************************************************************************************************

class SearchOptions
{
public:
	SearchOptions ();

	PROPERTY_STRING (searchTerms, SearchTerms)

	enum Mode { kAll, kKey, kTranslation, kScope };
	PROPERTY_VARIABLE (Mode, mode, Mode)

	PROPERTY_BOOL (exactMatch, ExactMatch)
	PROPERTY_BOOL (caseSensitive, CaseSensitive)

	bool matches (const StringEntry& entry) const;
};

//************************************************************************************************
// ProjectComponent
//************************************************************************************************

class ProjectComponent: public CCL::Component,
						public CCL::CommandDispatcher<ProjectComponent>
{
public:
	DECLARE_CLASS_ABSTRACT (ProjectComponent, Component)

	ProjectComponent (Project& project);
	~ProjectComponent ();

	Project& getProject ();
	StringInspector& getInspector ();
	void getSearchOptions (SearchOptions& options) const;
	void collectStrings (CCL::ObjectArray& entries, bool filterDuplicates) const;
	void importTranslations (const CCL::Container& entries);
	bool focusEntry (StringEntry* entry);
	int getFocusEntryIndex () const;
	bool focusEntryAtIndex (int index);
	void setFilter (const CCL::Container& files, const CCL::StringList& scopes);
	void restoreFilter (CCL::StringRef fileNames);
	void storeFilter (CCL::String& fileNames) const;
	void resetFilter ();
	void sortBy (CCL::StringID id);

	// Filter
	bool matchesScope (const ScopeEntry& entry) const;
	bool matchesString (const StringEntry& entry) const;

	// Notifications
	void onFocusItemChanged (BasicList* list);

	// Commands
	DECLARE_COMMANDS (ProjectComponent)
	DECLARE_COMMAND_CATEGORY2 ("Edit", "Pseudo", Component)
	bool onEditCopy (CCL::CmdArgs);
	bool onEditPaste (CCL::CmdArgs);
	bool onEditDelete (CCL::CmdArgs);
	bool onEditCleanup (CCL::CmdArgs);
	bool onEditSetOriginal (CCL::CmdArgs);
	bool onPseudoLocalization (CCL::CmdArgs);
	bool onStripContext (CCL::CmdArgs);
	bool onAutoComplete (CCL::CmdArgs);
	bool onNavigate (CCL::CmdArgs);
	bool onFilter (CCL::CmdArgs);
	bool onResetFilter (CCL::CmdArgs);

	// Component
	CCL::tresult CCL_API initialize (IUnknown* context = 0) override;
	CCL::tresult CCL_API terminate () override;
	IUnknown* CCL_API getObject (CCL::StringID name, CCL::UIDRef classID) override;
	CCL::tbool CCL_API paramChanged (CCL::IParameter* param) override;
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;

protected:
	struct SuspendGuard
	{
		ProjectComponent& c;
		SuspendGuard (ProjectComponent& c, bool state);
		~SuspendGuard ();
	};

	Project& project;
	FileList* fileList;
	ScopeList* scopeList;
	TranslationList* stringList;
	OccuranceList* occuranceList;
	SearchResultList* searchResults;
	StringInspector* inspector;
	bool projectUpdatesSuspended;

	struct FocusKeeper
	{
		CCL::ItemIndex focusItem;
		CCL::ListViewModel* listModel;

		FocusKeeper (CCL::ListViewModel* listModel)
		: listModel (listModel)
		{
			if(CCL::IItemView* itemView = listModel->getItemView ())
				itemView->getFocusItem (focusItem);
		}
		~FocusKeeper ()
		{
			if(CCL::IItemView* itemView = listModel->getItemView ())
			{
				if(focusItem.getIndex () > listModel->countFlatItems ())
					focusItem = CCL::ItemIndex (0);
				itemView->setFocusItem (focusItem);
			}
		}
	};

	enum StringMode { kShowAll, kShowTranslated, kShowUntranslated, kShowOrphaned, kShowUntranslatedAndOrphaned, kShowTranslationWarnings };
	StringMode stringMode;

	bool matchesMode (const StringEntry& entry) const;
	void updateParams ();
};

//************************************************************************************************
// StringInspector
//************************************************************************************************

class StringInspector: public CCL::Component
{
public:
	DECLARE_CLASS (StringInspector, Component)

	StringInspector ();

	StringEntry* getCurrentEntry () const;
	bool setCurrentEntry (StringEntry* entry);
	void refresh ();

	// Component
	CCL::tbool CCL_API paramChanged (CCL::IParameter* param) override;

protected:
	CCL::SharedPtr<StringEntry> currentEntry;
};

//************************************************************************************************
// BasicList
//************************************************************************************************

class BasicList: public CCL::ListViewModel
{
public:
	BasicList (ProjectComponent& component);

	virtual void rebuild () = 0;

protected:
	ProjectComponent& component;

	// ListViewModel
	void onSelectionChanged () override;
	CCL::tbool CCL_API onItemFocused (CCL::ItemIndexRef index) override;
};

//************************************************************************************************
// FileList
//************************************************************************************************

class FileList: public BasicList
{
public:
	DECLARE_STRINGID_MEMBER (kFileIconID)
	DECLARE_STRINGID_MEMBER (kItemsID)

	class FileItem: public CCL::ListViewItem
	{
	public:
		PROPERTY_SHARED_AUTO (FileEntry, entry, Entry)

		// ListViewItem
		bool getDetail (CCL::Variant& value, CCL::StringID id) const override;
		bool drawDetail (const IItemModel::DrawInfo& info, CCL::StringID id, CCL::AlignmentRef alignment) override;
		bool getTooltip (CCL::String& tooltip, CCL::StringID id) override;
	};

	FileList (ProjectComponent& component);

	FileItem* findFileItem (const FileEntry* entry) const;
	bool isAnyFileWithScopeChecked (CCL::StringRef scopeName) const;

	// BasicList
	void rebuild () override;
	CCL::tbool CCL_API openItem (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
	CCL::tbool CCL_API appendItemMenu (CCL::IContextMenu& menu, CCL::ItemIndexRef index, const CCL::IItemSelection& selection) override;
};

//************************************************************************************************
// ScopeList
//************************************************************************************************

class ScopeList: public BasicList
{
public:
	class ScopeItem: public CCL::ListViewItem
	{
	public:
		PROPERTY_SHARED_AUTO (ScopeEntry, entry, Entry)
	};

	ScopeList (ProjectComponent& component);

	ScopeItem* findScopeItem (CCL::StringRef scopeName) const;

	// BasicList
	void rebuild () override;
	CCL::tbool CCL_API openItem (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
	CCL::tbool CCL_API appendItemMenu (CCL::IContextMenu& menu, CCL::ItemIndexRef index, const CCL::IItemSelection& selection) override;
};

//************************************************************************************************
// StringEntryItem
//************************************************************************************************

class StringEntryItem: public CCL::ListViewItem
{
public:
	DECLARE_CLASS (StringEntryItem, ListViewItem)

	PROPERTY_SHARED_AUTO (StringEntry, entry, Entry)
};

//************************************************************************************************
// StringEntryList
//************************************************************************************************

class StringEntryList: public BasicList
{
public:
	StringEntryList (ProjectComponent& component);

	int getIndexForEntry (const StringEntry* entry) const;

	PROPERTY_MUTABLE_CSTRING (hiliteColumnID, HiliteColumnID)
	PROPERTY_MUTABLE_CSTRING (errorColumnID, ErrorColumnID)

	// BasicList
	CCL::tbool CCL_API drawCell (CCL::ItemIndexRef index, int column, const DrawInfo& info) override;
	CCL::tbool CCL_API canInsertData (CCL::ItemIndexRef index, int column, const CCL::IUnknownList& data, CCL::IDragSession* session, CCL::IView* targetView = 0) override;
	CCL::tbool CCL_API insertData (CCL::ItemIndexRef index, int column, const CCL::IUnknownList& data, CCL::IDragSession* session) override;
};

//************************************************************************************************
// TranslationList
//************************************************************************************************

class TranslationList: public StringEntryList,
					   public CCL::IParamObserver
{
public:
	DECLARE_STRINGID_MEMBER (kScopeID)
	DECLARE_STRINGID_MEMBER (kTranslationID)
	DECLARE_STRINGID_MEMBER (kFileID)

	static CCL::StringRef KeyString ();
	static CCL::StringRef Translation ();
	static CCL::StringRef ScopeName ();

	static CCL::StringRef ResetFilter ();

	TranslationList (ProjectComponent& component);

	class StringItem: public StringEntryItem
	{
	public:
		// StringEntryItem
		bool getDetail (CCL::Variant& value, CCL::StringID id) const override;
		bool getTooltip (CCL::String& tooltip, CCL::StringID id) override;
	};

	// StringEntryList
	void rebuild () override;
	CCL::tbool CCL_API editCell (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
	CCL::tbool CCL_API openItem (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
	CCL::tbool CCL_API appendItemMenu (CCL::IContextMenu& menu, CCL::ItemIndexRef index, const CCL::IItemSelection& selection) override;
	void onSortColumnChanged (CCL::StringID columnID, bool upwards) override;

	CLASS_INTERFACE (IParamObserver, StringEntryList)

protected:
	enum Tags { kEditTag = 'Edit' };

	// IParamObserver
	CCL::tbool CCL_API paramChanged (CCL::IParameter* param) override;
	void CCL_API paramEdit (CCL::IParameter* param, CCL::tbool begin) override {}
};

//************************************************************************************************
// OccuranceList
//************************************************************************************************

class OccuranceList: public StringEntryList
{
public:
	DECLARE_STRINGID_MEMBER (kTranslationID)
	DECLARE_STRINGID_MEMBER (kOccuranceID)

	class OccuranceItem: public StringEntryItem
	{
	public:
		// StringEntryItem
		bool getDetail (CCL::Variant& value, CCL::StringID id) const override;
		bool getTooltip (CCL::String& tooltip, CCL::StringID id) override;
	};

	OccuranceList (ProjectComponent& component);

	// StringEntryList
	void rebuild () override;
	CCL::tbool CCL_API openItem (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
};

//************************************************************************************************
// SearchResultList
//************************************************************************************************

class SearchResultList: public TranslationList
{
public:
	SearchResultList (ProjectComponent& component);

	// BasicList
	void rebuild () override;
	CCL::tbool CCL_API openItem (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
	CCL::tbool CCL_API appendItemMenu (CCL::IContextMenu& menu, CCL::ItemIndexRef index, const CCL::IItemSelection& selection) override;
};

} // namespace Localizer

#endif // _localizer_editor_h
