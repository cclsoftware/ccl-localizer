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
// Filename    : localizer.model.cpp
// Description : Localizer Data Model
//
//************************************************************************************************

#include "localizer.model.h"

#include "ccl/app/actions/actionjournal.h"
#include "ccl/public/text/translation.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (VariableMismatch, "Variable mismatch")
	XSTRING (ValueMismatch, "Mismatch in use of @value")
	XSTRING (CommandMismatch, "Mismatch in command title usage")
	XSTRING (SpaceAtStart, "Original starts with space but translation does not")
	XSTRING (SpaceAtEnd, "Original ends with space but translation does not")
END_XSTRINGS

//************************************************************************************************
// ProjectMeta
//************************************************************************************************

//DEFINE_STRINGID_MEMBER_ (ProjectMeta, kLanguage, "language")

//************************************************************************************************
// Project
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (Project, kStringChanged, "stringChanged")
DEFINE_STRINGID_MEMBER_ (Project, kFileChanged, "fileChanged")
DEFINE_STRINGID_MEMBER_ (Project, kSaved, "saved")
DEFINE_STRINGID_MEMBER_ (Project, kErrorsChanged, "errorsChanged")

DEFINE_CLASS_HIDDEN (Project, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Project::Project (ActionJournal* _actionJournal)
: actionJournal (_actionJournal)
{
	if(actionJournal == nullptr)
		actionJournal = NEW ActionJournal;

	files.objectCleanup (true);
	scopes.objectCleanup (true);
	strings.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Project::~Project ()
{
	safe_release (actionJournal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournal* Project::getActionJournal () const
{
	return actionJournal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Project::addFile (FileEntry* file)
{
	file->setProject (this);
	files.add (return_shared (file));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Project::getFiles () const
{
	return files.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Project::hasModifiedFiles () const
{
	ArrayForEach (files, FileEntry, file)
		if(file->isModified ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Project::getScopes () const
{
	return scopes.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Project::addString (StringEntry* string)
{
	strings.add (return_shared (string));

	// add scope
	if(!scopes.contains (ScopeEntry (string->getScopeName ())))
		scopes.add (NEW ScopeEntry (string->getScopeName ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Project::removeString (StringEntry* string)
{
	strings.remove (string);
	string->release ();

	// TODO: what about scopes???
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Project::getStrings () const
{
	return strings.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Project::getOccurances (Container& result, StringRef keyString) const
{
	ArrayForEach (strings, StringEntry, entry)
		if(entry->getKeyString ().compare (keyString, false) == 0) // ignore case
			result.add (entry);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringEntry* Project::findEqual (const StringEntry& entry, bool ignoreScope) const
{
	ArrayForEach (strings, StringEntry, e)
		if((e->getScopeName () == entry.getScopeName () || ignoreScope) &&
		   e->getKeyString () == entry.getKeyString ())
		   return e;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Project::findFirstSuggestion (const StringEntry& entry) const
{
	ArrayForEach (strings, StringEntry, e)
		if(!e->getTranslation ().isEmpty ())
		{
			if(e->getKeyString ().compare (entry.getKeyString (), false) == Text::kEqual) // ignore case
				return e->getTranslation ();

			#if 1 // special cases if "..." or ":" has been removed from the original
			auto tryWithoutEnding = [&] (StringRef ending)
			{
				if(e->getKeyString ().endsWith (ending))
				{
					String shortenedKey (e->getKeyString ());
					shortenedKey.truncate (shortenedKey.length () - ending.length ());
					if(shortenedKey.compare (entry.getKeyString (), false) == Text::kEqual)
					{
						String t (e->getTranslation ());
						if(t.endsWith (ending))
							t.truncate (t.length () - ending.length ());
						return t;
					}
				}
				return String ();
			};

			{
				static const String threeDots ("...");
				String suggestion = tryWithoutEnding (threeDots);
				if(!suggestion.isEmpty ())
					return suggestion;
			}
			{
				static const String colon (":");
				String suggestion = tryWithoutEnding (colon);
				if(!suggestion.isEmpty ())
					return suggestion;
			}
			#endif
		}
	EndFor
	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Project::isEmpty () const
{
	return files.isEmpty () && scopes.isEmpty () && strings.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Project::removeAll ()
{
	files.removeAll ();
	scopes.removeAll ();
	strings.removeAll ();
}

//************************************************************************************************
// FileEntry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileEntry, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileEntry::FileEntry (UrlRef path)
: project (nullptr),
  path (path),
  modified (false),
  translatedCount (0)
{
	strings.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileEntry::addString (StringEntry* string)
{
	string->setFile (this);

	strings.add (return_shared (string));

	scopes.addOnce (string->getScopeName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileEntry::removeString (StringEntry* string)
{
	ASSERT (string && string->getFile () == this)
	
	string->setFile (nullptr);
	strings.remove (string);
	string->release ();

	// TODO: what about scopes???
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringEntry* FileEntry::findStringAtLine (int lineNumber) const
{
	ArrayForEach (strings, StringEntry, entry)
		if(entry->getLineNumber () == lineNumber)
			return entry;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FileEntry::getStringCount () const
{
	return strings.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& FileEntry::getStrings () const
{
	return strings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileEntry::hasScopeName (StringRef scopeName) const
{
	return scopes.contains (scopeName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringList& FileEntry::getErrors ()
{
	return errors;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileEntry::updateStats ()
{
	translatedCount = 0;
	ArrayForEach (strings, StringEntry, entry)
		if(entry->isTranslated ())
			translatedCount++;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileEntry::updateTranslationWarnings ()
{
	ArrayForEach (strings, StringEntry, entry)
		if(entry->isTranslated () && !entry->isOrphaned ())
			entry->updateTranslationWarnings ();
	EndFor
}

//************************************************************************************************
// ScopeEntry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScopeEntry, Boxed::String)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScopeEntry::ScopeEntry (CCL::StringRef string)
: Boxed::String (string)
{}

//************************************************************************************************
// StringEntry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringEntry, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringEntry::StringEntry ()
: file (nullptr),
  lineNumber (0),
  orphaned (false),
  modified (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StringEntry::getFileAndLine () const
{
	if(lineNumber > 0)
		return String () << file->getFileName () << ":" << lineNumber;
	else
		return file->getFileName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringEntry::isTranslated () const
{
	return !translation.isEmpty (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringEntry::updateTranslationWarnings ()
{
	static StringRef kVariableStart = "%(";
	static StringRef kValueStart = "@value";
	static StringRef kCommandTitleStart = "@cmd.title[";
	static StringRef kCommandTitleEnd = "]";

	translationWarnings.empty ();
	
	auto appendTranslationWarning = [&] (StringRef warning)
	{
		if(!translationWarnings.isEmpty ())
			translationWarnings.append (String::getLineEnd ());
		translationWarnings.append (warning);
	};

	// check for spaces in beginning or end of original or translation
	if(keyString.startsWith (" ") && !translation.startsWith (" "))
		appendTranslationWarning (XSTR (SpaceAtStart));
	if(keyString.endsWith (" ") && !translation.endsWith (" "))
		appendTranslationWarning (XSTR (SpaceAtEnd));

	// check string format variables
	bool valueMismatch = false;
	for(int i = 1; i <= 4; i++)
	{
		String variableFormat (kVariableStart);
		variableFormat.appendIntValue (i);
		if(keyString.contains (variableFormat) && !translation.contains (variableFormat))
			valueMismatch = true;
	}
	if(valueMismatch)
		appendTranslationWarning (XSTR (VariableMismatch));

	// check @value usage
	if(keyString.contains (kValueStart) && !translation.contains (kValueStart))
		appendTranslationWarning (XSTR (ValueMismatch));

	// check @cmd.title usage
	if(keyString.contains (kCommandTitleStart))
	{
		int startIndex = keyString.index (kCommandTitleStart) + kCommandTitleStart.length ();
		String commandName = keyString.subString (startIndex);
		int endIndex = commandName.index (kCommandTitleEnd);
		if(endIndex > 0)
		{
			commandName = commandName.subString (0, endIndex);
			String fullCommandString;
			fullCommandString << kCommandTitleStart << commandName << kCommandTitleEnd;
			if(!translation.contains (fullCommandString))
				appendTranslationWarning (XSTR (CommandMismatch));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringEntry::hasTranslationWarnings () const
{
	return !translationWarnings.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringEntry::addReference (StringRef ref)
{
	references.add (ref); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StringList& StringEntry::getReferences () const
{
	return references; 
}
