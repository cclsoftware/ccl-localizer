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
// Filename    : localizer.po.h
// Description : Portable Object Handler
//
//************************************************************************************************

#ifndef _localizer_po_h
#define _localizer_po_h

#include "localizer.builder.h"

namespace CCL {
class TextFile; }

namespace Localizer {
namespace PO {

//************************************************************************************************
// PO::Handler
//************************************************************************************************

class Handler: public FormatHandler
{
public:
	DECLARE_CLASS (Handler, FormatHandler)

	static const CCL::FileType& getBinaryType ();
	static const CCL::FileType& getTemplateType ();

	// FormatHandler
	int getFlags () const override;
	const CCL::FileType& getFileType () const override;
	bool parseFile (CCL::Attributes& metaInfo, CCL::Container& strings, CCL::UrlRef path) override;
	bool saveFile (const CCL::Attributes& metaInfo, const CCL::Container& strings, CCL::UrlRef path) override;
	bool compileFile (CCL::UrlRef path) override;
	bool updateFile (CCL::UrlRef path, CCL::UrlRef templateFolder) override;
};

//************************************************************************************************
// PO::BinaryImporter
//************************************************************************************************

class BinaryImporter: public ImportHandler
{
public:
	bool parseMO (CCL::Container& strings, CCL::IStream& stream);

	// ImportHandler
	const CCL::FileType& getFileType () const override;
	bool parseFile (CCL::Attributes& metaInfo, CCL::Container& strings, CCL::UrlRef path) override;

protected:
	struct Hook;
};

//************************************************************************************************
// PO::Parser
//************************************************************************************************

class Parser
{
public:
	Parser (CCL::Container& strings, CCL::Attributes& metaInfo, CCL::StringRef fileName);

	PROPERTY_STRING (fileName, FileName)

	bool parse (CCL::TextFile& textFile);

protected:
	CCL::Container& strings;
	CCL::Attributes& metaInfo;

	enum State { kNowhere, kMsgctxt, kMsgid, kMsgstr };
	State state;
	int lineNumber;
	bool headerFound;
	bool msgctxtFound;
	CCL::String currentMsgctxt;
	CCL::String currentMsgid;
	CCL::String currentMsgstr;
	int currentMsgstrLineNumber;
	CCL::StringList currentReferences;
	bool currentIsOrphaned;

	bool flush ();
	void error (CCL::StringRef text);
	bool getLiteral (CCL::String& result, CCL::StringRef line);
};

//************************************************************************************************
// PO::Writer
//************************************************************************************************

class Writer
{
public:
	Writer (const CCL::Container& strings, const CCL::Attributes& metaInfo);

	bool write (CCL::TextFile& textFile);

protected:
	const CCL::Container& strings;
	const CCL::Attributes& metaInfo;
};

//************************************************************************************************
// PO::Gettext
//************************************************************************************************

class Gettext
{
public:
	static bool getLocation (CCL::Url& path);
	static bool makeMO (CCL::UrlRef poPath, CCL::UrlRef moPath);
	static bool merge (CCL::UrlRef poPath, CCL::UrlRef potPath);

protected:
	static const CCL::String msgfmtName;
	static const CCL::String msgmergeName;

	static int execute (CCL::StringList& output, CCL::UrlRef exePath, CCL::ArgsRef args);
};

} // namespace PO
} // namespace Localizer

#endif // _localizer_po_h
