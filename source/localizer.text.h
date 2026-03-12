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
// Filename    : localizer.text.h
// Description : Text File Handler
//
//************************************************************************************************

#ifndef _localizer_text_h
#define _localizer_text_h

#include "localizer.builder.h"

namespace Localizer {
namespace TXT {

//************************************************************************************************
// TXT::Handler
//************************************************************************************************

class Handler: public FormatHandler
{
public:
	DECLARE_CLASS (Handler, FormatHandler)

	// FormatHandler
	int getFlags () const override;
	const CCL::FileType& getFileType () const override;
	bool parseFile (CCL::Attributes& metaInfo, CCL::Container& strings, CCL::UrlRef path) override;
	bool saveFile (const CCL::Attributes& metaInfo, const CCL::Container& strings, CCL::UrlRef path) override;
	bool compileFile (CCL::UrlRef path) override { /*nothing to compile*/ return true; }
	bool updateFile (CCL::UrlRef path, CCL::UrlRef templateFolder) override { /*nothing to update*/ return true; }
};

//************************************************************************************************
// TXT::CSVExporter
//************************************************************************************************

class CSVExporter: public ExportHandler
{
public:
	DECLARE_CLASS (CSVExporter, ExportHandler)

	// ExportHandler
	const CCL::FileType& getFileType () const override;
	bool saveFile (const CCL::Attributes& metaInfo, const CCL::Container& strings, CCL::UrlRef path) override;
};

} // namespace TXT
} // namespace Localizer

#endif // _localizer_text_h
