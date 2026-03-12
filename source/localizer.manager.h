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
// Filename    : localizer.manager.h
// Description : Localizer Management Component
//
//************************************************************************************************

#ifndef _localizer_manager_h
#define _localizer_manager_h

#include "ccl/app/component.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/storableobject.h"

#include "ccl/public/gui/commanddispatch.h"

namespace CCL {
class RecentDocuments; }

namespace Localizer {

class Project;
class ProjectComponent;

//************************************************************************************************
// ProjectLanguage
//************************************************************************************************

class ProjectLanguage: public CCL::Object
{
public:
	DECLARE_CLASS (ProjectLanguage, Object)

	ProjectLanguage ();

	PROPERTY_STRING (languageName, LanguageName)

	void addFolder (CCL::UrlRef folder);
	const CCL::Container& getFolders () const { return folders; }

	// Object
	bool toString (CCL::String& string, int flags = 0) const override;

protected:
	CCL::ObjectArray folders;
};

//************************************************************************************************
// ProjectDataFile
//************************************************************************************************

class ProjectDataFile: public CCL::StorableObject
{
public:
	static const CCL::FileType& getFileType ();

	void getLanguages (CCL::Container& languages, CCL::UrlRef baseFolder) const; ///< ProjectLanguage
	CCL::IUrlFilter* createFileFilter () const;

	// StorableObject
	CCL::tbool CCL_API getFormat (CCL::FileType& format) const override;
	CCL::tbool CCL_API save (CCL::IStream& stream) const override;
	CCL::tbool CCL_API load (CCL::IStream& stream) override;

protected:
	CCL::Attributes attributes;
};

//************************************************************************************************
// ProjectFolderSelector
//************************************************************************************************

class ProjectFolderSelector: public CCL::Component
{
public:
	ProjectFolderSelector (Project& project);

	PROPERTY_SHARED_AUTO (CCL::IUrlFilter, fileFilter, FileFilter)
	void updateLanguages (const CCL::Container& languages);
	bool loadLanguage (const ProjectLanguage& pl);
	void resetAll ();

	// Component
	CCL::tbool CCL_API paramChanged (CCL::IParameter* param) override;

protected:
	Project& project;
};

//************************************************************************************************
// ProjectManager
//************************************************************************************************

class ProjectManager: public CCL::Component,
					  public CCL::CommandDispatcher<ProjectManager>
{
public:
	DECLARE_CLASS (ProjectManager, Component)

	ProjectManager ();
	~ProjectManager ();

	static ProjectManager* getInstance ();

	CCL::RecentDocuments& getRecentPaths ();
	bool openLocation (CCL::UrlRef path); // project, file, or folder
	bool checkSaveNeeded (bool cancelEnabled = true);

	// Component
	CCL::tbool CCL_API canTerminate () const override;
	CCL::tresult CCL_API initialize (IUnknown* context = 0) override;
	CCL::tresult CCL_API terminate () override;

	// Commands
	DECLARE_COMMANDS (ProjectManager)
	DECLARE_COMMAND_CATEGORY2 ("File", "Edit", Component)
	bool onFileOpen (CCL::CmdArgs);
	bool onFileSave (CCL::CmdArgs);
	bool onFileSaveAs (CCL::CmdArgs);
	bool onFileClose (CCL::CmdArgs);
	bool onFileRevert (CCL::CmdArgs);
	bool onFileExport (CCL::CmdArgs);
	bool onFileImport (CCL::CmdArgs);
	bool onUpdateAll (CCL::CmdArgs);
	bool onCompileAll (CCL::CmdArgs);
	bool onEditUndo (CCL::CmdArgs);
	bool onEditRedo (CCL::CmdArgs);
	bool onOpenRecent (CCL::CmdArgs);
	bool onClearRecent (CCL::CmdArgs);

protected:
	static ProjectManager* theInstance;

	ProjectComponent* editor;
	ProjectFolderSelector* folderSelector;
	CCL::RecentDocuments* recentPaths;

	void openFolder (CCL::UrlRef path);
	void openSingleFile (CCL::UrlRef path);
	void openProjectFile (CCL::UrlRef path);
	void updateApplicationTitle (CCL::StringRef projectTitle);
	bool saveAll (const CCL::IUrl* destFolder = 0);
	void updateAll (CCL::UrlRef templateFolder);
	void compileAll ();
};

} // namespace Localizer

#endif // _localizer_manager_h
