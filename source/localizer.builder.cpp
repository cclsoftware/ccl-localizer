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
// Filename    : localizer.builder.cpp
// Description : Localizer Data Builder
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "localizer.builder.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/app/actions/actionjournal.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Localizer;

//************************************************************************************************
// ProjectBuilder
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ProjectBuilder, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectBuilder::ProjectBuilder (Project& project)
: project (project)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectBuilder::~ProjectBuilder ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::addFolder (UrlRef path, IUrlFilter* filter, IProgressNotify* progress)
{
	bool result = false;
	ForEachFile (System::GetFileSystem ().newIterator (path), p)
		if(p->isFile ())
		{			
			if(progress)
			{
				String fileName;
				p->getName (fileName);
				progress->updateAnimated (fileName);
			}

			if(filter && !filter->matches (*p))
			{
				#if DEBUG_LOG
				FormatHandler* handler = FormatRegistry::instance ().findHandler (p->getFileType ());
				if(handler && handler->canEdit ())
				{
					String fileName;
					p->getName (fileName);
					CCL_PRINTF ("File excluded by filter: %s\n", MutableCString (fileName).str ())
				}
				#endif
				continue;
			}

			if(addFile (*p))
			{
				result = true;
				#if (0 && DEBUG)
				break; // break on first file for testing
				#endif
			}
		}
	EndFor

	if(result)
		project.signal (Message (kChanged));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::addSingleFile (UrlRef path)
{
	if(!addFile (path))
		return false;

	project.signal (Message (kChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::addFile (UrlRef path)
{
	FormatHandler* handler = FormatRegistry::instance ().findHandler (path.getFileType ());
	if(handler == nullptr)
		return false;
	if(!handler->canEdit ()) // ignore import/export formats
		return false;

	Attributes metaInfo;
	ObjectArray strings;
	strings.objectCleanup (true);

	if(!handler->parseFile (metaInfo, strings, path))
		return false;

	AutoPtr<FileEntry> fileEntry = NEW FileEntry (path);
	String fileName;
	path.getName (fileName);
	fileEntry->setFileName (fileName);
	project.addFile (fileEntry);

	if(metaInfo.getBool (FormatHandler::kInconsistentLineEndings))
		fileEntry->setModified (true);

	ForEach (strings, StringEntry, stringEntry)
		fileEntry->addString (stringEntry);
		project.addString (stringEntry);
	EndFor

	fileEntry->updateStats ();
	fileEntry->updateTranslationWarnings ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectBuilder::removeAll ()
{
	if(!project.isEmpty ())
	{
		project.removeAll ();
		project.getActionJournal ()->removeAll (); // clear action stack
		project.signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::saveAll (IProgressNotify* progress, const IUrl* destFolder)
{
	bool saveAsCopy = destFolder != nullptr;
	if(!saveAsCopy)
		clearErrors ();

	IterForEach (project.getFiles (), FileEntry, entry)
		if(entry->isModified () || saveAsCopy)
		{
			if(progress)
				progress->updateAnimated (entry->getFileName ());

			if(!saveFile (*entry, destFolder))
				return false;
		}
	EndFor

	if(!saveAsCopy)
	{
		project.getActionJournal ()->setSavedNow ();
		project.signal (Message (Project::kSaved));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::saveFile (FileEntry& file, const IUrl* destFolder)
{
	FormatHandler* handler = FormatRegistry::instance ().findHandler (file.getPath ().getFileType ());
	ASSERT (handler != nullptr)
	if(handler == nullptr)
		return false;
	
	Url destPath (file.getPath ());
	bool saveAsCopy = destFolder != nullptr;
	if(saveAsCopy)
	{
		destPath = *destFolder;
		String fileName;
		file.getPath ().getName (fileName);
		destPath.descend (fileName);
	}

	Attributes metaInfo; // TODO
	if(!handler->saveFile (metaInfo, file.getStrings (), destPath))
		return false;

	if(!saveAsCopy)
	{
		file.setModified (false);
		ForEach (file.getStrings (), StringEntry, e)
			e->setModified (false);
		EndFor

		project.signal (Message (Project::kFileChanged, file.asUnknown ()));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::compileAll (IProgressNotify* progress)
{
	clearErrors ();
	bool anyErrors = false;

	IterForEach (project.getFiles (), FileEntry, entry)
		if(progress)
			progress->updateAnimated (entry->getFileName ());

		bool hasErrors = false;
		if(!compileFile (*entry, hasErrors))
			return false;
		if(hasErrors)
			anyErrors = true;
	EndFor

	if(anyErrors)
		project.signal (Message (Project::kErrorsChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::updateAll (UrlRef templateFolder, IProgressNotify* progress)
{
	clearErrors ();
	bool anyErrors = false;

	IterForEach (project.getFiles (), FileEntry, entry)
		if(progress)
			progress->updateAnimated (entry->getFileName ());

		bool hasErrors = false;
		if(!updateFile (*entry, templateFolder, hasErrors))
			return false;
		if(hasErrors)
			anyErrors = true;
	EndFor

	if(anyErrors)
		project.signal (Message (Project::kErrorsChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectBuilder::clearErrors ()
{
	bool changed = false;
	IterForEach (project.getFiles (), FileEntry, entry)
		if(!entry->getErrors ().isEmpty ())
		{
			changed = true;
			entry->getErrors ().removeAll ();

			ForEach (entry->getStrings (), StringEntry, entry)
				entry->setErrorMessage (String::kEmpty);
			EndFor
		}
	EndFor

	if(changed)
		project.signal (Message (Project::kErrorsChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::checkErrors (FileEntry& file, IErrorContext* errorContext)
{
	bool hasErrors = false;
	for(int i = 0; i < errorContext->getEventCount (); i++)
	{
		AlertEventRef e = errorContext->getEvent (i);
		if(e.fileName == file.getFileName ())
		{
			// add error to file
			hasErrors = true;
			file.getErrors ().add (e.message);

			// add error to string
			if(StringEntry* entry = file.findStringAtLine (e.lineNumber))
				entry->setErrorMessage (e.message);
		}
	}
	return hasErrors;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::compileFile (FileEntry& file, bool& hasErrors)
{
	FormatHandler* handler = FormatRegistry::instance ().findHandler (file.getPath ().getFileType ());
	ASSERT (handler != nullptr)
	if(handler == nullptr)
		return false;

	ErrorContextGuard errorContext;
	bool result = handler->compileFile (file.getPath ());
	if(errorContext.hasErrors ())
	{
		hasErrors = checkErrors (file, errorContext);
		errorContext.pushToParent (); // repeat errors in parent context
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectBuilder::updateFile (FileEntry& file, UrlRef templateFolder, bool& hasErrors)
{
	FormatHandler* handler = FormatRegistry::instance ().findHandler (file.getPath ().getFileType ());
	ASSERT (handler != nullptr)
	if(handler == nullptr)
		return false;

	ErrorContextGuard errorContext;
	bool result = handler->updateFile (file.getPath (), templateFolder);
	if(errorContext.hasErrors ())
	{
		hasErrors = checkErrors (file, errorContext);
		errorContext.pushToParent (); // repeat errors in parent context
	}
	return result;
}

//************************************************************************************************
// FormatHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FormatHandler, Object)
DEFINE_STRINGID_MEMBER_ (FormatHandler, kInconsistentLineEndings, "InconsistentLineEndings")

//************************************************************************************************
// ImportHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ImportHandler, FormatHandler)

//************************************************************************************************
// ExportHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ExportHandler, FormatHandler)

//************************************************************************************************
// FormatRegistry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FormatRegistry, Object)
DEFINE_SINGLETON (FormatRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

FormatRegistry::FormatRegistry ()
{
	handlers.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormatRegistry::addHandler (FormatHandler* handler)
{
	handlers.add (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FormatHandler* FormatRegistry::findHandler (const FileType& fileType) const
{
	ForEach (handlers, FormatHandler, handler)
		if(handler->getFileType () == fileType)
			return handler;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* FormatRegistry::newIterator () const
{
	return handlers.newIterator ();
}
