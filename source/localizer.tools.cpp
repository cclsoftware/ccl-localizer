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
// Filename    : localizer.tools.cpp
// Description : Language Pack Builder
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "localizer.tools.h"
#include "appversion.h"
#include "localizer.po.h"

#include "ccl/extras/tools/publisher.h"

#include "ccl/app/components/consolecomponent.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

namespace Localizer {

//************************************************************************************************
// LanguagePackImporter
//************************************************************************************************

class LanguagePackImporter: public ImportHandler
{
public:
	// ImportHandler
	const CCL::FileType& getFileType () const override;
	bool parseFile (CCL::Attributes& metaInfo, CCL::Container& strings, CCL::UrlRef path) override;

protected:
	CCL::StringList scannedFiles;

	void parseFolder (CCL::Container& strings, CCL::IFileSystem& fileSystem, CCL::UrlRef folder);
};

} // namespace Localizer

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (LanguagePackImporter, kFirstRun+1)
{
	FormatRegistry::instance ().addHandler (NEW LanguagePackImporter);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (LanguageBuildFile, "Language Build File")
	XSTRING (BuildSuccessful, "Build successful.")
	XSTRING (BuildFailed, "Build failed. See output window for further details.")
END_XSTRINGS

static FileType languageBuildFileType (nullptr, "langbuild", APP_MIME_TYPE "-langbuild");

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (LanguagePackBuilder)
	DEFINE_COMMAND ("Tools", "Build Language Pack", LanguagePackBuilder::onCmd)
END_COMMANDS (LanguagePackBuilder)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (ContextConverter)
	DEFINE_COMMAND ("Tools", "Convert Context", ContextConverter::onCmd)
END_COMMANDS (ContextConverter)

//************************************************************************************************
// ToolsComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ToolsComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolsComponent::ToolsComponent (StringRef name)
: Component (name),
  path (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolsComponent::onCmd (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		if(selectPath ())
		{
			setup ();

			AutoPtr<ConsoleComponent> console = NEW ConsoleComponent;
			console->setDirectUpdate (true);
			console->setActive (true);

			AutoPtr<IView> consoleForm = getTheme ()->createView ("Console", console->asUnknown ());
			ASSERT (consoleForm != nullptr)
			if(consoleForm)
				FormBox (consoleForm).openWindow ();

			run ();

			if(consoleForm)
				FormBox (consoleForm).closeWindow ();
			console->setActive (false);

			cleanup ();
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolsComponent::selectPath ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolsComponent::setup ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolsComponent::run ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolsComponent::cleanup ()
{}

//************************************************************************************************
// LanguagePackBuilder
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LanguagePackBuilder, ToolsComponent)
IMPLEMENT_COMMANDS (LanguagePackBuilder, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

LanguagePackBuilder::LanguagePackBuilder ()
: ToolsComponent (String ("Builder")),
  outputPath (nullptr)
{
	languageBuildFileType.setDescription (XSTR (LanguageBuildFile));
	System::GetFileTypeRegistry ().registerFileType (languageBuildFileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePackBuilder::selectPath ()
{
	AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
	fs->addFilter (languageBuildFileType);
	if(fs->run (IFileSelector::kOpenFile))
		path = NEW Url (*fs->getPath ());
	else
		path = nullptr;

	return path != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LanguagePackBuilder::setup ()
{
	// ensure that everything is compiled - could be unrelated!
	System::GetCommandTable ().performCommand (CommandMsg ("File", "Compile All"));

	outputPath = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LanguagePackBuilder::run ()
{
	ASSERT (path != nullptr)

	Publisher p;
	if(p.run (*path))
	{
		Url output;
		p.makeOutputPath (output, p.getReturnValue ());
		outputPath = NEW Url (output);
		Alert::info (XSTR (BuildSuccessful));
	}
	else
		Alert::error (XSTR (BuildFailed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LanguagePackBuilder::cleanup ()
{
	if(outputPath != nullptr)
		System::GetSystemShell ().showFile (*outputPath);
}

//************************************************************************************************
// LanguagePackImporter
//************************************************************************************************

const FileType& LanguagePackImporter::getFileType () const
{
	return System::GetLocaleManager ().getLanguagePackFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePackImporter::parseFile (Attributes& metaInfo, Container& strings, UrlRef path)
{
	AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (path);
	if(!packageFile)
		return false;

	IFileSystem* fileSystem = packageFile->getFileSystem ();
	ASSERT (fileSystem != nullptr)

	parseFolder (strings, *fileSystem, Url ());
	scannedFiles.removeAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LanguagePackImporter::parseFolder (Container& strings, IFileSystem& fileSystem, UrlRef folder)
{
	ForEachFile (fileSystem.newIterator (folder), path)
		if(path->isFile ())
		{
			if(path->getFileType () == PO::Handler::getBinaryType ())
			{
				String fileName;
				path->getName (fileName);
				if(!scannedFiles.contains (fileName))
				{
					scannedFiles.add (fileName);

					CCL_PRINTF ("Scanning MO file \"%s\" inside Language Pack...\n", MutableCString (fileName).str ())

					AutoPtr<IStream> stream = fileSystem.openStream (*path);
					ASSERT (stream != nullptr)
					if(stream)
					{
						AutoPtr<IStream> seekableStream = System::GetFileUtilities ().createSeekableStream (*stream);
						PO::BinaryImporter ().parseMO (strings, *seekableStream);
					}
				}
			}
		}
		else
			parseFolder (strings, fileSystem, *path);
	EndFor
}

//************************************************************************************************
// ContextConverter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ContextConverter, ToolsComponent)
IMPLEMENT_COMMANDS (ContextConverter, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ContextConverter::ContextConverter ()
: ToolsComponent (String ("Converter")),
  succeeded (0),
  failed (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContextConverter::processFolder (UrlRef path)
{
	ForEachFile (System::GetFileSystem ().newIterator (path), p)
		if(p->isFolder ())
			processFolder (*p);
		else
			processFile (*p);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContextConverter::processFile (UrlRef path)
{
	static PO::Handler handler;
	if(path.getFileType () != handler.getFileType ())
		return;

	ObjectArray entries;
	entries.objectCleanup (true);
	Attributes metaInfo;

	// Keep short for console window.
	String url;
	path.getName (url);

	// Import supports legacy format, save once for new format.
	bool result = handler.parseFile (metaInfo, entries, path);
	if(result)
		result = handler.saveFile (metaInfo, entries, path);

	result ? succeeded++ : failed++;

	System::GetConsole ().writeLine (String () << "File '" << url << "': " << (result ? "OK" : "FAILED"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContextConverter::selectPath ()
{
	AutoPtr<IFolderSelector> fs = ccl_new<IFolderSelector> (ClassID::FolderSelector);
	if(fs->run ())
		path = NEW Url (fs->getPath ());
	else
		path = nullptr;

	return path != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContextConverter::setup ()
{
	succeeded = 0;
	failed = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContextConverter::run ()
{
	ASSERT (path != nullptr)
	processFolder (*path);

	// Not translated, debug build feature only.
	String msg = String ().appendFormat (CCLSTR ("Succeeded: %(1), failed: %(2)"), succeeded, failed);
	Alert::info (msg);
}
