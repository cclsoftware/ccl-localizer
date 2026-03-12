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
// Filename    : localizer.text.cpp
// Description : Text File Handler
//
//************************************************************************************************

#include "localizer.text.h"

#include "ccl/base/storage/textfile.h"

using namespace CCL;
using namespace Localizer;
using namespace TXT;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (TextHandler, kFirstRun)
{
	FormatRegistry::instance ().addHandler (NEW Handler);
	FormatRegistry::instance ().addHandler (NEW CSVExporter);
	return true;
}

//************************************************************************************************
// TXT::Handler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Handler, FormatHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

int Handler::getFlags () const
{
	return kCanEdit;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& Handler::getFileType () const
{
	return FileTypes::Text ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Handler::parseFile (Attributes& metaInfo, Container& strings, UrlRef path)
{
	String content = TextUtils::loadString (path, String::getLineEnd (), Text::kUTF8); // enforce UTF-8 if encoding not specified in file

	String fileName;
	path.getName (fileName, false);

	StringEntry* e = NEW StringEntry;
	e->setKeyString (fileName);
	e->setTranslation (content);
	strings.add (e);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Handler::saveFile (const Attributes& metaInfo, const Container& strings, UrlRef path)
{
	StringEntry* e = (StringEntry*)strings.at (0);
	ASSERT (e != nullptr)
	if(!e)
		return false;

	TextFile textFile (path, Text::kUTF8);
	if(!textFile.isValid ())
		return false;

	return textFile->writeString (e->getTranslation ()) != 0;
}

//************************************************************************************************
// TXT::CSVExporter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CSVExporter, ExportHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CSVExporter::getFileType () const
{
	return FileTypes::Csv ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CSVExporter::saveFile (const Attributes& metaInfo, const Container& strings, UrlRef path)
{
	// see http://tools.ietf.org/html/rfc4180
	TextFile textFile (path, Text::kUTF8, Text::kCRLFLineFormat);
	if(!textFile.isValid ())
		return false;

	static const String delimiter (";");

	String header;
	header << "Context" << delimiter << "Original" << delimiter << "Translation" << delimiter << "File";
	if(!textFile->writeLine (header))
		return false;

	auto quote = [] (StringRef string)
	{
		return String () << "\"" << string << "\"";
	};

	ForEach (strings, StringEntry, entry)
		String line;
		line << entry->getScopeName () << delimiter << quote (entry->getKeyString ()) << delimiter << quote (entry->getTranslation ()) << delimiter << entry->getFileAndLine ();
		if(!textFile->writeLine (line))
			return false;
	EndFor

	return true;
}
