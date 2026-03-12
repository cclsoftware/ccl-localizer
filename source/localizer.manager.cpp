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
// Filename    : localizer.manager.cpp
// Description : Localizer Management Component
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "localizer.manager.h"
#include "localizer.editor.h"
#include "localizer.builder.h"

#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/filefilter.h"

#include "ccl/app/documents/recentdocuments.h"
#include "ccl/app/actions/actionjournal.h"
#include "ccl/app/params.h"

#include "ccl/extras/tools/repositoryinfo.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace Localizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Localizer")
	XSTRING (LoadFailed, "Some files failed to load!")
	XSTRING (SaveFailed, "Some files failed to save!")
	XSTRING (AskSaveChanges, "Do you want to save your changes?")
	XSTRING (AskRevertChanges, "Are you sure you want to revert all changes?")
	XSTRING (ExportOptions, "Export Options")
	XSTRING (FilterDuplicates, "Filter Duplicates")
END_XSTRINGS

BEGIN_XSTRINGS ("FileType")
	XSTRING (ProjectFile, "Localization Project")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ProjectFolderSelectorTags
	{
		kLanguageList = 100
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (ProjectManager)
	DEFINE_COMMAND ("File", "Open",			ProjectManager::onFileOpen)
	DEFINE_COMMAND ("File", "Open Folder",	ProjectManager::onFileOpen)
	DEFINE_COMMAND ("File",	"Save",			ProjectManager::onFileSave)
	DEFINE_COMMAND ("File",	"Save As",		ProjectManager::onFileSaveAs)
	DEFINE_COMMAND ("File",	"Close",		ProjectManager::onFileClose)
	DEFINE_COMMAND ("File",	"Revert",		ProjectManager::onFileRevert)
	DEFINE_COMMAND ("File",	"Export",		ProjectManager::onFileExport)
	DEFINE_COMMAND ("File",	"Import",		ProjectManager::onFileImport)
	DEFINE_COMMAND ("File",	"Update",		ProjectManager::onUpdateAll)
	DEFINE_COMMAND ("File",	"Compile All",	ProjectManager::onCompileAll)
	DEFINE_COMMAND ("Edit",	"Undo",			ProjectManager::onEditUndo)
	DEFINE_COMMAND ("Edit",	"Redo",			ProjectManager::onEditRedo)
	DEFINE_COMMAND ("Recent File", nullptr,		ProjectManager::onOpenRecent)
	DEFINE_COMMAND ("File", "Clear Recent Files",	ProjectManager::onClearRecent)
END_COMMANDS (ProjectManager)

//************************************************************************************************
// ProjectLanguage
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ProjectLanguage, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectLanguage::ProjectLanguage ()
{
	folders.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectLanguage::addFolder (UrlRef folder)
{
	folders.add (NEW Url (folder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectLanguage::toString (String& string, int flags) const
{
	string = languageName;
	return true;
}

//************************************************************************************************
// ProjectDataFile
//************************************************************************************************

const FileType& ProjectDataFile::getFileType ()
{
	static FileType fileType (nullptr, "lzproject", CCL_MIME_TYPE "-lzproject");
	return FileTypes::init (fileType, XSTR (ProjectFile));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectDataFile::getLanguages (Container& languages, UrlRef baseFolder) const
{
    IterForEach (attributes.newQueueIterator ("languages", ccl_typeid<Attributes> ()), Attributes, attr)
		ProjectLanguage* language = NEW ProjectLanguage;
		language->setLanguageName (attr->getString ("name"));

		String repositoryFolder = attr->getString ("repositoryfolder");
		if(!repositoryFolder.isEmpty ())
		{
			RepositoryInfo repositoryInfo;
			repositoryInfo.load (baseFolder);
			ObjectArray paths;
			paths.objectCleanup ();
			repositoryInfo.findAllPaths (paths, RepositoryInfo::kTranslationsDirectories, repositoryFolder);
			ForEach (paths, Url, path)
				Url absolutePath (repositoryFolder, *path, Url::kFolder);
				language->addFolder (absolutePath);
			EndFor
		}
		String relativeFolder = attr->getString ("folder");
		if(!relativeFolder.isEmpty ())
		{
			Url absolutePath (relativeFolder, baseFolder, Url::kFolder);
			language->addFolder (absolutePath);
		}
		else 
		{
			IterForEach (attr->newQueueIterator ("folders", ccl_typeid<Attribute> ()), Attribute, attr)
				relativeFolder = attr->getValue ().asString ();
				Url absolutePath (relativeFolder, baseFolder, Url::kFolder);
				language->addFolder (absolutePath);
			EndFor
		}

		languages.add (language);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter* ProjectDataFile::createFileFilter () const
{
	FileFilter* filter = NEW FileFilter;
	filter->setFilterMode (FileFilter::kIncludeMode);
	IterForEach (attributes.newQueueIterator ("files", ccl_typeid<Attribute> ()), Attribute, attr)
		String fileName = attr->getValue ().asString ();
		if(!fileName.isEmpty ())
			filter->addItem (FileFilter::kIncludeMode, fileName, IUrl::kFile);
	EndFor
	return filter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProjectDataFile::getFormat (FileType& format) const
{
	format = FileTypes::Json ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProjectDataFile::save (IStream& stream) const
{
	return JsonArchive (stream).saveAttributes (nullptr, attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProjectDataFile::load (IStream& stream)
{
	return JsonArchive (stream).loadAttributes (nullptr, attributes);
}

//************************************************************************************************
// ProjectFolderSelector
//************************************************************************************************

ProjectFolderSelector::ProjectFolderSelector (Project& project)
: Component ("FolderSelector"),
  project (project)
{
	paramList.addList ("languageList", Tag::kLanguageList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectFolderSelector::updateLanguages (const Container& languages)
{
	ListParam* lp = paramList.byTag<ListParam> (Tag::kLanguageList);
	lp->removeAll ();
	ForEach (languages, ProjectLanguage, language)
		lp->appendObject (return_shared (language));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectFolderSelector::loadLanguage (const ProjectLanguage& language)
{
	ListParam* lp = paramList.byTag<ListParam> (Tag::kLanguageList);
	int index = lp->getObjectIndex (language);
	if(index == -1)
		return false;

	if(auto manager = ProjectManager::getInstance ())
		manager->checkSaveNeeded (false);

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	progress->setCancelEnabled (false);
	ProgressNotifyScope progressScope (progress);

	ErrorContextGuard errorContext;

	ProjectBuilder (project).removeAll ();
	ForEach (language.getFolders (), Url, folder)
		ProjectBuilder (project).addFolder (*folder, fileFilter, progress);
	EndFor

	if(errorContext.hasErrors ())
	{
		progressScope.finish ();
		Alert::errorWithContext (XSTR (LoadFailed), true);
	}

	lp->setValue (index);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectFolderSelector::resetAll ()
{
	ListParam* lp = paramList.byTag<ListParam> (Tag::kLanguageList);
	lp->removeAll ();
	setFileFilter (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProjectFolderSelector::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kLanguageList)
	{
		ListParam* lp = unknown_cast<ListParam> (param);
		ProjectLanguage* language = ccl_cast<ProjectLanguage> (lp->getSelectedObject ());
		if(language)
			loadLanguage (*language);
	}
	return true;
}

//************************************************************************************************
// ProjectManager
//************************************************************************************************

ProjectManager* ProjectManager::theInstance = nullptr;
ProjectManager* ProjectManager::getInstance () { return theInstance; }

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ProjectManager, Component)
IMPLEMENT_COMMANDS (ProjectManager, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectManager::ProjectManager ()
: Component (String ("Manager")),
  editor (nullptr),
  folderSelector (nullptr),
  recentPaths (nullptr)
{
	ASSERT (theInstance == nullptr)
	theInstance = this;

	AutoPtr<Project> project = NEW Project;
	addComponent (editor = NEW ProjectComponent (*project));
	addComponent (folderSelector = NEW ProjectFolderSelector (*project));
	addComponent (recentPaths = NEW RecentDocuments (CCLSTR ("RecentDocuments"), 20, 20, RecentDocuments::kShowFullPathInMenu));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProjectManager::~ProjectManager ()
{
	ASSERT (theInstance == this)
	theInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecentDocuments& ProjectManager::getRecentPaths ()
{
	return *recentPaths;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ProjectManager::initialize (IUnknown* context)
{
	SuperClass::initialize (context);

	recentPaths->restore ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ProjectManager::terminate ()
{
	recentPaths->store ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProjectManager::canTerminate () const
{
	if(RootComponent::instance ().isQuitting ()) // quit already confirmed by user
		return true;

	bool result = const_cast<ProjectManager*> (this)->checkSaveNeeded ();
	if(result)
	{
		// store focus entry before window goes away
		Attributes& a = Settings::instance ().getAttributes ("ProjectManager");
		int focusIndex = editor->getFocusEntryIndex ();
		if(focusIndex != -1)
			a.set ("focusIndex", focusIndex);
		else
			a.remove ("focusIndex");
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::openLocation (UrlRef path)
{
	if(!checkSaveNeeded ())
		return false;

	if(path.isFolder ())
	{
		openFolder (path);
		return true;
	}
	else if(path.getFileType () == ProjectDataFile::getFileType ())
	{
		openProjectFile (path);
		return true;
	}
	else if(FormatHandler* handler = FormatRegistry::instance ().findHandler (path.getFileType ()))
	{
		if(handler->canEdit ())
		{
			openSingleFile (path);
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectManager::openFolder (UrlRef path)
{
	CCL_PRINT ("Open folder: ")
	CCL_PRINTLN (UrlDisplayString (path))

	folderSelector->resetAll ();
	recentPaths->setRecentPath (path);

	// keep & restore filtered files later
	String savedFileFilter;
	editor->storeFilter (savedFileFilter);

	Project& project = editor->getProject ();
	ProjectBuilder (project).removeAll ();

	ErrorContextGuard errorContext;

	{
		AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
		ProgressNotifyScope progressScope (progress);
		ProjectBuilder (project).addFolder (path, nullptr, progress);
	}

	// TODO: restore editor settings...
	// editor->restoreFilter (...);
	// editor->focusEntryAtIndex (...);

	if(errorContext.hasErrors ())
		Alert::errorWithContext (XSTR (LoadFailed));

	editor->restoreFilter (savedFileFilter);

	String folderName;
	path.getName (folderName);
	updateApplicationTitle (folderName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectManager::openSingleFile (UrlRef path)
{
	CCL_PRINT ("Open single file: ")
	CCL_PRINTLN (UrlDisplayString (path))

	folderSelector->resetAll ();
	recentPaths->setRecentPath (path);

	Project& project = editor->getProject ();
	ProjectBuilder (project).removeAll ();

	ErrorContextGuard errorContext;
	if(!ProjectBuilder (project).addSingleFile (path))
	{
		if(errorContext.hasErrors ())
			Alert::errorWithContext (XSTR (LoadFailed));
	}
	else
	{
		String fileName;
		path.getName (fileName);
		updateApplicationTitle (fileName);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectManager::openProjectFile (UrlRef path)
{		
	CCL_PRINT ("Open project file: ")
	CCL_PRINTLN (UrlDisplayString (path))

	folderSelector->resetAll ();

	ErrorContextGuard errorContext;

	ProjectDataFile data;
	if(data.loadFromFile (path))
	{
		recentPaths->setRecentPath (path);

		Url baseFolder (path);
		baseFolder.ascend ();
		ObjectArray languages;
		languages.objectCleanup (true);
		data.getLanguages (languages, baseFolder);

		AutoPtr<IUrlFilter> filter = data.createFileFilter ();
		folderSelector->setFileFilter (filter);
		folderSelector->updateLanguages (languages);

		if(!languages.isEmpty ())
		{
			ProjectLanguage* first = static_cast<ProjectLanguage*> (languages.at (0));
			folderSelector->loadLanguage (*first);
		}

		String projectTitle;
		path.getName (projectTitle);
		updateApplicationTitle (projectTitle);
	}
	else
		Alert::errorWithContext (XSTR (LoadFailed), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectManager::updateApplicationTitle (StringRef projectTitle)
{
	IWindow* appWindow = System::GetDesktop ().getApplicationWindow ();
	if(appWindow)
	{
		String title (RootComponent::instance ().getApplicationTitle ());
		if(!projectTitle.isEmpty())
		{
			title.append (CCLSTR (" - "));
			title.append (projectTitle);
		}

		appWindow->setWindowTitle (title);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::checkSaveNeeded (bool cancelEnabled)
{
	Project& project = editor->getProject ();
	if(project.hasModifiedFiles ())
	{
		int result = Alert::ask (XSTR (AskSaveChanges), cancelEnabled ? Alert::kYesNoCancel : Alert::kYesNo);
		if(result == Alert::kCancel)
			return false;

		if(result == Alert::kYes)
			if(!saveAll ())
				return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::saveAll (const IUrl* destFolder)
{
	Project& project = editor->getProject ();
	ASSERT (project.hasModifiedFiles () || destFolder != nullptr)

	bool saved = false;
	{
		AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
		ProgressNotifyScope progressScope (progress);
		saved = ProjectBuilder (project).saveAll (progress, destFolder);
	}

	if(!saved)
		Alert::error (XSTR (SaveFailed));
	return saved;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectManager::updateAll (UrlRef templateFolder)
{
	Project& project = editor->getProject ();

	ErrorContextGuard errorContext;

	{
		AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
		ProgressNotifyScope progressScope (progress);

		ProjectBuilder (project).updateAll (templateFolder, progress);
	}

	if(errorContext.hasErrors ())
		Alert::errorWithContext (XSTR (SaveFailed), true); // TODO: explicit error message!?
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProjectManager::compileAll ()
{
	Project& project = editor->getProject ();

	ErrorContextGuard errorContext;

	{
		AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
		ProgressNotifyScope progressScope (progress);
		ProjectBuilder (project).compileAll (progress);
	}

	if(errorContext.hasErrors ())
		Alert::errorWithContext (XSTR (SaveFailed), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onFileOpen (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		if(args.name.contains ("Folder"))
		{
			AutoPtr<IFolderSelector> fs = ccl_new<IFolderSelector> (ClassID::FolderSelector);
			if(fs->run ())
			{
				if(checkSaveNeeded ())
					openFolder (fs->getPath ());
			}
		}
		else
		{
			AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
			fs->addFilter (ProjectDataFile::getFileType ());

			FormatRegistry& formats = FormatRegistry::instance ();
			IterForEach (formats.newIterator (), FormatHandler, handler)
				if(handler->canEdit ())
					fs->addFilter (handler->getFileType ());
			EndFor

			if(fs->run (IFileSelector::kOpenFile))
			{
				if(checkSaveNeeded ())
					openLocation (*fs->getPath (0));
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onFileSave (CmdArgs args)
{
	Project& project = editor->getProject ();
	if(!project.hasModifiedFiles ())
		return false;

	if(!args.checkOnly ())
		saveAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onFileSaveAs (CmdArgs args)
{
	Project& project = editor->getProject ();
	if(project.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		AutoPtr<IFolderSelector> fs = ccl_new<IFolderSelector> (ClassID::FolderSelector);
		if(fs->run ())
			saveAll (&fs->getPath ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onFileClose (CmdArgs args)
{
	Project& project = editor->getProject ();
	if(project.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		if(checkSaveNeeded ())
		{
			ProjectBuilder (project).removeAll ();
			folderSelector->resetAll ();
			updateApplicationTitle (String::kEmpty);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onFileRevert (CmdArgs args)
{
	Project& project = editor->getProject ();
	if(project.isEmpty ())
		return false;
	if(!project.hasModifiedFiles ())
		return false;

	if(!args.checkOnly ())
		if(Alert::ask (XSTR (AskRevertChanges)) == Alert::kYes)
		{
			ProjectBuilder (project).removeAll ();

			if(auto path = recentPaths->at (0))
			{
				Url path2 (*path);
				openLocation (path2);
			}
		}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onFileExport (CmdArgs args)
{
	Project& project = editor->getProject ();
	if(project.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);

		FormatRegistry& formats = FormatRegistry::instance ();
		IterForEach (formats.newIterator (), FormatHandler, handler)
			if(handler->canExport ())
				fs->addFilter (handler->getFileType ());
		EndFor

		if(fs->run (IFileSelector::kSaveFile))
		{
			// export options
			ParamContainer paramList;
			IParameter* filterDuplicates = paramList.addParam (XSTR_REF (FilterDuplicates).getKey ());
			if(DialogBox ()->runWithParameters (CCLSTR ("ExportOptions"), paramList, XSTR (ExportOptions)) != DialogResult::kOkay)
				return true;

			UrlRef path = *fs->getPath ();
			bool done = false;

			if(FormatHandler* handler = formats.findHandler (path.getFileType ()))
				if(handler->canExport ())
				{
					ObjectArray entries;
					editor->collectStrings (entries, filterDuplicates->getValue ());

					Attributes metaInfo;
					done = handler->saveFile (metaInfo, entries, path);
				}

			if(!done)
				Alert::error (XSTR (SaveFailed));
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onFileImport (CmdArgs args)
{
	Project& project = editor->getProject ();
	if(project.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);

		FormatRegistry& formats = FormatRegistry::instance ();
		IterForEach (formats.newIterator (), FormatHandler, handler)
			if(handler->canImport ())
				fs->addFilter (handler->getFileType ());
		EndFor

		if(fs->run (IFileSelector::kOpenMultipleFiles))
		{
			ObjectArray entries;
			entries.objectCleanup (true);
			bool result = false;

			AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
			ProgressNotifyScope progressScope (progress);
			ErrorContextGuard errorContext;

			for(int i = 0; i < fs->countPaths (); i++)
			{
				UrlRef path = *fs->getPath (i);
				String fileName;
				path.getName (fileName);
				progress->updateAnimated (fileName);

				if(FormatHandler* handler = formats.findHandler (path.getFileType ()))
					if(handler->canImport ())
					{
						Attributes metaInfo;
						if(handler->parseFile (metaInfo, entries, path))
							result = true;
					}
			}

			progressScope.finish ();

			if(result)
				editor->importTranslations (entries);
			else
				Alert::errorWithContext (XSTR (LoadFailed));
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onUpdateAll (CmdArgs args)
{
	Project& project = editor->getProject ();
	if(project.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		AutoPtr<IFolderSelector> fs = ccl_new<IFolderSelector> (ClassID::FolderSelector);
		if(fs->run ())
		{
			if(checkSaveNeeded ())
			{
				updateAll (fs->getPath ());

				ProjectBuilder (project).removeAll ();

				if(auto path = recentPaths->at (0))
				{
					Url path2 (*path);
					openLocation (path2);
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onCompileAll (CmdArgs args)
{
	Project& project = editor->getProject ();
	if(project.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		if(checkSaveNeeded ())
			compileAll ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onEditUndo (CmdArgs args)
{
	ActionJournal* journal = editor->getProject ().getActionJournal ();
	if(!journal->canUndo ())
		return false;

	if(!args.checkOnly ())
		journal->undo ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onEditRedo (CmdArgs args)
{
	ActionJournal* journal = editor->getProject ().getActionJournal ();
	if(!journal->canRedo ())
		return false;

	if(!args.checkOnly ())
		journal->redo ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onOpenRecent (CmdArgs args)
{
	if(args.checkOnly ())
		return true;

	int64 index = 0;
	args.name.getIntValue (index);
	if(index >= 1)
	{
		const Url* path = recentPaths->at ((int)index - 1);
		if(path)
			openLocation (*path);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProjectManager::onClearRecent (CmdArgs args)
{
	if(recentPaths->count () < 1)
		return false;

	if(!args.checkOnly ())
		recentPaths->clearAll ();
	return true;
}
