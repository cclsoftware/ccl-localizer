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
// Filename    : localizer.model.h
// Description : Localizer Data Model
//
//************************************************************************************************

#ifndef _localizer_model_h
#define _localizer_model_h

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/text/cstring.h"

#include "ccl/app/actions/iactioncontext.h"

namespace Localizer {

class FileEntry;
class StringEntry;

//************************************************************************************************
// ProjectMeta
//************************************************************************************************

class ProjectMeta
{
public:
	//DECLARE_STRINGID_MEMBER (kLanguage)
};

//************************************************************************************************
// Project
//************************************************************************************************

class Project: public CCL::Object,
			   public CCL::IActionContext
{
public:
	DECLARE_CLASS (Project, Object)

	Project (CCL::ActionJournal* actionJournal = 0);
	~Project ();

	DECLARE_STRINGID_MEMBER (kStringChanged)
	DECLARE_STRINGID_MEMBER (kFileChanged)
	DECLARE_STRINGID_MEMBER (kSaved)
	DECLARE_STRINGID_MEMBER (kErrorsChanged)

	void addFile (FileEntry* file);			///< shared!
	CCL::Iterator* getFiles () const;
	bool hasModifiedFiles () const;

	CCL::Iterator* getScopes () const;

	void addString (StringEntry* string);	///< shared!
	void removeString (StringEntry* string);
	CCL::Iterator* getStrings () const;
	void getOccurances (CCL::Container& result, CCL::StringRef keyString) const;
	StringEntry* findEqual (const StringEntry& entry, bool ignoreScope = false) const;
	CCL::String findFirstSuggestion (const StringEntry& entry) const;

	bool isEmpty () const;
	void removeAll ();

	// IActionContext
	CCL::ActionJournal* getActionJournal () const override;

	CLASS_INTERFACE (IActionContext, Object)

protected:
	CCL::ActionJournal* actionJournal;
	CCL::ObjectArray files;
	CCL::ObjectArray scopes;
	CCL::ObjectArray strings;
};

//************************************************************************************************
// FileEntry
//************************************************************************************************

class FileEntry: public CCL::Object
{
public:
	DECLARE_CLASS (FileEntry, Object)

	FileEntry (CCL::UrlRef path = CCL::Url ());

	PROPERTY_POINTER (Project, project, Project)

	PROPERTY_OBJECT (CCL::Url, path, Path)
	PROPERTY_STRING (fileName, FileName)
	PROPERTY_BOOL (modified, Modified)
	PROPERTY_VARIABLE (int, translatedCount, TranslatedCount)

	void addString (StringEntry* string);	///< shared!
	void removeString (StringEntry* string);
	StringEntry* findStringAtLine (int lineNumber) const;
	
	int getStringCount () const;
	const CCL::Container& getStrings () const;
	bool hasScopeName (CCL::StringRef scopeName) const;

	CCL::StringList& getErrors ();

	void updateStats ();
	void updateTranslationWarnings ();

protected:
	CCL::StringList scopes;
	CCL::ObjectArray strings;
	CCL::StringList errors;
};

//************************************************************************************************
// ScopeEntry
//************************************************************************************************

class ScopeEntry: public CCL::Boxed::String
{
public:
	DECLARE_CLASS (ScopeEntry, CCL::Boxed::String)

	ScopeEntry (CCL::StringRef string = 0);
};

//************************************************************************************************
// StringEntry
//************************************************************************************************

class StringEntry: public CCL::Object
{
public:
	DECLARE_CLASS (StringEntry, Object)

	StringEntry ();

	PROPERTY_POINTER (FileEntry, file, File)
	PROPERTY_VARIABLE (int, lineNumber, LineNumber) ///< line number starts at 1
	CCL::String getFileAndLine () const;

	PROPERTY_STRING (scopeName, ScopeName)
	PROPERTY_STRING (keyString, KeyString)
	PROPERTY_STRING (translation, Translation)
	PROPERTY_BOOL (orphaned, Orphaned)
	PROPERTY_BOOL (modified, Modified)
	PROPERTY_STRING (errorMessage, ErrorMessage)
	PROPERTY_STRING (translationWarnings, TranslationWarnings)

	bool isTranslated () const;
	
	void addReference (CCL::StringRef ref);
	const CCL::StringList& getReferences () const;

	void updateTranslationWarnings ();
	bool hasTranslationWarnings () const;

protected:
	CCL::StringList references;
};

} // namespace Localizer

#endif // _localizer_model_h
