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
// Filename    : localizer.builder.h
// Description : Localizer Data Builder
//
//************************************************************************************************

#ifndef _localizer_builder_h
#define _localizer_builder_h

#include "localizer.model.h"

#include "ccl/base/singleton.h"

namespace CCL {
interface IProgressNotify; 
interface IErrorContext; }

namespace Localizer {

//************************************************************************************************
// ProjectBuilder
//************************************************************************************************

class ProjectBuilder: public CCL::Object
{
public:
	DECLARE_CLASS_ABSTRACT (ProjectBuilder, Object)

	ProjectBuilder (Project& project);
	~ProjectBuilder ();

	bool addFolder (CCL::UrlRef path, CCL::IUrlFilter* filter = 0, CCL::IProgressNotify* progress = 0);
	bool addSingleFile (CCL::UrlRef path);

	void removeAll ();

	bool saveAll (CCL::IProgressNotify* progress = 0, const CCL::IUrl* destFolder = 0); ///< save all modified files or save copy to dest folder
	bool compileAll (CCL::IProgressNotify* progress = 0);
	bool updateAll (CCL::UrlRef templateFolder, CCL::IProgressNotify* progress = 0);

protected:
	Project& project;

	void clearErrors ();
	bool checkErrors (FileEntry& file, CCL::IErrorContext* errorContext);

	bool addFile (CCL::UrlRef path);
	bool saveFile (FileEntry& file, const CCL::IUrl* destFolder = 0);
	bool compileFile (FileEntry& file, bool& hasErrors);
	bool updateFile (FileEntry& file, CCL::UrlRef templateFolder, bool& hasErrors);
};

//************************************************************************************************
// FormatHandler
//************************************************************************************************

class FormatHandler: public CCL::Object
{
public:
	DECLARE_CLASS_ABSTRACT (FormatHandler, Object)

	enum Flags 
	{
		kCanEdit = 1<<0,		///< format can be opened for editing
		kCanExport = 1<<1,		///< format can be exported
		kCanImport = 1<<2		///< format can be imported
	};

	bool canEdit () const	{ return (getFlags () & kCanEdit) != 0; }
	bool canExport () const	{ return (getFlags () & kCanExport) != 0; }
	bool canImport () const	{ return (getFlags () & kCanImport) != 0; }

	DECLARE_STRINGID_MEMBER (kInconsistentLineEndings)

	virtual int getFlags () const = 0;
	virtual const CCL::FileType& getFileType () const = 0;
	virtual bool parseFile (CCL::Attributes& metaInfo, CCL::Container& strings, CCL::UrlRef path) = 0;
	virtual bool saveFile (const CCL::Attributes& metaInfo, const CCL::Container& strings, CCL::UrlRef path) = 0;
	virtual bool compileFile (CCL::UrlRef path) = 0;
	virtual bool updateFile (CCL::UrlRef path, CCL::UrlRef templateFolder) = 0;
};

//************************************************************************************************
// ImportHandler
//************************************************************************************************

class ImportHandler: public FormatHandler
{
public:
	DECLARE_CLASS_ABSTRACT (ImportHandler, FormatHandler)

	// FormatHandler
	int getFlags () const override { return kCanImport; }
	bool saveFile (const CCL::Attributes& metaInfo, const CCL::Container& strings, CCL::UrlRef path) override { /*nothing to save*/ return false; }
	bool compileFile (CCL::UrlRef path) override { /*nothing to compile*/ return false; }
	bool updateFile (CCL::UrlRef path, CCL::UrlRef templateFolder) override { /*nothing to update*/ return false; }
};

//************************************************************************************************
// ExportHandler
//************************************************************************************************

class ExportHandler: public FormatHandler
{
public:
	DECLARE_CLASS_ABSTRACT (ExportHandler, FormatHandler)

	// FormatHandler
	int getFlags () const override { return kCanExport; }
	bool parseFile (CCL::Attributes& metaInfo, CCL::Container& strings, CCL::UrlRef path) override { /*nothing to parse*/ return false; }
	bool compileFile (CCL::UrlRef path) override { /*nothing to compile*/ return false; }
	bool updateFile (CCL::UrlRef path, CCL::UrlRef templateFolder) override { /*nothing to update*/ return false; }
};

//************************************************************************************************
// FormatRegistry
//************************************************************************************************

class FormatRegistry: public CCL::Object,
					  public CCL::Singleton<FormatRegistry>
{
public:
	DECLARE_CLASS (FormatRegistry, Object)

	FormatRegistry ();

	void addHandler (FormatHandler* handler);
	FormatHandler* findHandler (const CCL::FileType& fileType) const;
	CCL::Iterator* newIterator () const;

protected:
	CCL::ObjectArray handlers;
};

} // namespace Localizer

#endif // _localizer_builder_h
