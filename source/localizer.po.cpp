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
// Filename    : localizer.po.cpp
// Description : Portable Object Handler
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "localizer.po.h"

#include "ccl/base/development.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/file.h"

#include "ccl/main/cclargs.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/translationformat.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace PortableObjectFormat;
using namespace Localizer;
using namespace PO;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (POFile, "PO File")
	XSTRING (MOFile, "MO File")
	XSTRING (POError, "PO Error: %(1) File: %(2), Line: %(3)")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (POHandler, kFirstRun)
{
	FormatRegistry::instance ().addHandler (NEW Handler);
	FormatRegistry::instance ().addHandler (NEW BinaryImporter);
	return true;
}

//************************************************************************************************
// PO::Handler
//************************************************************************************************

const FileType& Handler::getBinaryType ()
{
	static FileType fileType (nullptr, "mo", "application/octet-stream");
	return FileTypes::init (fileType, XSTR (MOFile));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& Handler::getTemplateType ()
{
	static const FileType fileType ("POT File", "pot", "text/plain");
	return fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Handler, FormatHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

int Handler::getFlags () const
{
	return kCanEdit|kCanImport|kCanExport;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& Handler::getFileType () const
{
	static FileType fileType (nullptr, "po", "text/plain");
	return FileTypes::init (fileType, XSTR (POFile));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Handler::parseFile (Attributes& metaInfo, Container& strings, UrlRef path)
{
	TextFile textFile (path, TextFile::kOpen, Text::kUTF8);
	if(!textFile.isValid ())
		return false;

	#if DEBUG_LOG
	CCL_PRINT ("Parse .po file ")
	CCL_PRINTLN (UrlDisplayString (path))
	#endif

	String fileName;
	path.getName (fileName);
	bool result = Parser (strings, metaInfo, fileName).parse (textFile);
	
	// Note: Old Mac OS 9 line format is not recognized by gettext tools, Windows and Unix-style seem to work both fine
	// However, we decided to consistently use LF so everything else is marked as modified
	if(textFile->getLineFormat () != Text::kLFLineFormat) 
		metaInfo.set (kInconsistentLineEndings, true);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Handler::saveFile (const Attributes& metaInfo, const Container& strings, UrlRef path)
{
	// ensure that file is closed when this goes out of scope
	{
		TextFile textFile (path, Text::kUTF8, Text::kLFLineFormat, ITextStreamer::kSuppressByteOrderMark);
		if(!textFile.isValid ())
			return false;

		if(!Writer (strings, metaInfo).write (textFile))
			return false;
	}

	#if 0 // compile to .mo
	compileFile (path);
	#endif

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Handler::compileFile (UrlRef path)
{
	Url moPath (path);
	moPath.setFileType (getBinaryType (), true);
	Gettext::makeMO (path, moPath);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Handler::updateFile (UrlRef path, UrlRef templateFolder)
{
	String fileName;
	path.getName (fileName, false);

	Url potPath (templateFolder);
	potPath.descend (fileName);
	potPath.setFileType (getTemplateType (), true);

	Gettext::merge (path, potPath);
	return true;
}

//************************************************************************************************
// PO::BinaryImporter::Hook
//************************************************************************************************

struct BinaryImporter::Hook: Object, ITranslationTableHook
{
	Container& strings;
	
	Hook (Container& strings)
	: strings (strings)
	{}

	static String escape (StringRef string) // copied from xstringparser.cpp:
	{
		static const struct { String escaped; String unescaped; String temp; } escapedChars[] =
		{
			{CCLSTR ("\\n"), CCLSTR ("\n"), CCLSTR ("")},
			{CCLSTR ("\\r"), CCLSTR ("\r"), CCLSTR ("")},
			{CCLSTR ("\\t"), CCLSTR ("\t"), CCLSTR ("")},
			{CCLSTR ("\\\""), CCLSTR ("\""), CCLSTR ("\n")}
		};

		String result (string);
		for(int i = 0; i < ARRAY_COUNT (escapedChars); i++)
		{
			// workaround for endless loop with " -> \"
			if(escapedChars[i].temp.isEmpty ())
				result.replace (escapedChars[i].unescaped, escapedChars[i].escaped);
			else
			{
				result.replace (escapedChars[i].unescaped, escapedChars[i].temp);
				result.replace (escapedChars[i].temp, escapedChars[i].escaped);
			}
		}
		return result;
	}

	void CCL_API translationAdded (StringID scope, StringID key, StringRef text) override
	{
		StringEntry* e = NEW StringEntry;
		e->setScopeName (String (scope));
		e->setKeyString (escape (String (key))); // escaped form needs to be preserved for editing!
		e->setTranslation (escape (text));
		strings.add (e);
	}

	CLASS_INTERFACE (ITranslationTableHook, Object)
};

//************************************************************************************************
// PO::BinaryImporter
//************************************************************************************************

const FileType& BinaryImporter::getFileType () const
{
	return Handler::getBinaryType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryImporter::parseFile (Attributes& metaInfo, Container& strings, UrlRef path)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path);
	if(!stream)
		return false;

	return parseMO (strings, *stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryImporter::parseMO (Container& strings, IStream& stream)
{
	Hook theHook (strings);
	AutoPtr<ITranslationTable> table = System::CreateTranslationTable ();
	return table->loadStrings (stream, &theHook) == kResultOk;
}

//************************************************************************************************
// PO::Parser
//************************************************************************************************

Parser::Parser (Container& strings, Attributes& metaInfo, StringRef fileName)
: fileName (fileName),
  strings (strings),
  metaInfo (metaInfo),
  state (kNowhere),
  lineNumber (-1),
  headerFound (false),
  msgctxtFound (false),
  currentMsgstrLineNumber (-1),
  currentIsOrphaned (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parser::error (StringRef text)
{
	//CCL_DEBUGGER ("PO Error!")

	String message;
	message.appendFormat (XSTR (POError), text, fileName, lineNumber);
	ccl_raise (message);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::getLiteral (String& result, StringRef line)
{
	result.empty ();

	int start = line.index (quote_char);
	int end = line.lastIndex (quote_char);
	if(start < 0 || end < 0 || start == end)
		return false;

	result = line.subString (start + 1, end - start - 1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::parse (TextFile& textFile)
{
	headerFound = false;
	for(lineNumber = 1; ; lineNumber++)
	{
		String line;
		if(!textFile->readLine (line))
			break;
		
		line.trimWhitespace ();
		if(line.isEmpty ())
		{
			if(!flush ())
				return false;

			// Reset, may not always exist. Do not
			// carry over from previous entry.
			currentMsgctxt.empty ();
			msgctxtFound = false;

			continue;
		}

		currentIsOrphaned = false;
		if(line.startsWith (orphaned_prefix))
		{
			currentIsOrphaned = true;
			line.remove (0, orphaned_prefix.length ());
			line.trimWhitespace ();
		}

		if(line.startsWith (keyword_msgctxt))
		{
			msgctxtFound = true;
			if(!getLiteral (currentMsgctxt, line))
			{
				error ("Malformed msgctxt!");
				return false;
			}
			state = kMsgctxt;
		}
		else if(line.startsWith (keyword_msgid))
		{
			if(!getLiteral (currentMsgid, line))
			{
				error ("Malformed msgid!");
				return false;
			}
			state = kMsgid;
		}
		else if(line.startsWith (keyword_msgstr))
		{
			if(!getLiteral (currentMsgstr, line))
			{
				error ("Malformed msgstr!");
				return false;
			}
			currentMsgstrLineNumber = lineNumber;
			state = kMsgstr;
		}
		else if(line.startsWith (hash_char))
		{
			// TODO: handle comments!

			if(line.startsWith (keyword_reference))
			{
				String ref = line.subString (keyword_reference.length ());
				ref.trimWhitespace ();
				if(!ref.isEmpty ())
				{
					currentReferences.add (ref);
					//CCL_PRINT ("Reference ")
					//CCL_PRINTLN (ref)
				}
			}
		}
		else
		{
			if(state == kMsgctxt || state == kMsgid || state == kMsgstr)
			{
				if(!line.startsWith (quote_char))
				{
					error ("Unknown keyword!");
					return false;
				}

				String appendix;
				if(!getLiteral (appendix, line))
				{
					error ("Malformed literal!");
					return false;
				}
				switch(state)
				{
				case kMsgctxt :
					currentMsgctxt << appendix;
					break;
				case kMsgid :
					currentMsgid << appendix;
					break;
				case kMsgstr :
					currentMsgstr << appendix;
					break;
				}
			}
		}
	}

	return flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::flush ()
{
	if(state == kMsgstr)
	{
		if(currentMsgid.isEmpty ())
		{
			if(headerFound)
			{
				error ("Another empty msgid found!");
				return false;
			}

			headerFound = true;

			// check if file is UTF-8
			static const String charSetUtf8 ("charset=utf-8");
			if(!currentMsgstr.contains (charSetUtf8, false))
			{
				error ("File not UTF-8!");
				return false;
			}
		}
		else
		{
			// Support for legacy scope format.
			MutableCString scope;
			MutableCString original (currentMsgid, Text::kUTF8);
			CStringPtr start = original.str ();

			// Tolerate non-scope [] in msgid if msgctxt is present.
			if(msgctxtFound)
				scope = currentMsgctxt;
			else
			{
				#if 1 // extract scope name (copied from translationtable.cpp)
				CStringPtr ptr = start;
				if(*ptr == '[')
				{
					while(*ptr && *ptr != ']')
						ptr++;

					ASSERT (*ptr == ']')
					if(*ptr == ']')
					{
						start = ptr + 1;
						int count = (int) (start - original.str () - 2);
						scope.append (original.str () + 1, count);
					}
				}
				#endif
			}

			StringEntry* e = NEW StringEntry;
			e->setLineNumber (currentMsgstrLineNumber);
			e->setOrphaned (currentIsOrphaned);
			e->setScopeName (String (scope));
			e->setKeyString (String (start));
			e->setTranslation (currentMsgstr);
			for(int i = 0, count = currentReferences.count (); i < count; i++)
			{
				e->addReference (currentReferences[i]);
				#if (0 && DEBUG)
				if(i > 0)
				{
					int breakPointHere = 0;
				}
				#endif
			}
			strings.add (e);
		}

		currentReferences.removeAll ();
		state = kNowhere;
	}
	return true;
}

//************************************************************************************************
// PO::Writer
//************************************************************************************************

Writer::Writer (const Container& strings, const Attributes& metaInfo)
: strings (strings),
  metaInfo (metaInfo)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Writer::write (TextFile& textFile)
{
	FormatWriter formatWriter (textFile);
	if(!formatWriter.writeHeader ())
		return false;

	ForEach (strings, StringEntry, entry)
		for(int i = 0, count = entry->getReferences ().count (); i < count; i++)
		{
			String ref = entry->getReferences ()[i];
			ASSERT (!ref.isEmpty ())
			if(!formatWriter.writeReference (ref))
				return false;
		}

		if(!formatWriter.writeMessage (entry->getScopeName (), entry->getKeyString (), 
									   entry->getTranslation (), entry->isOrphaned ()))
			return false;
	
		int lineNumber = formatWriter.getLastMessageLineNumber ();
		SOFT_ASSERT (lineNumber == entry->getLineNumber (), "Line number changed!\n")
		entry->setLineNumber (lineNumber);
	EndFor
	return true;
}

//************************************************************************************************
// PO::Gettext
//************************************************************************************************

#if CCL_PLATFORM_WINDOWS
const String Gettext::msgfmtName ("msgfmt.exe");
const String Gettext::msgmergeName ("msgmerge.exe");
#else
const String Gettext::msgfmtName ("msgfmt");
const String Gettext::msgmergeName ("msgmerge");
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Gettext::getLocation (Url& path)
{
	Url defaultPath;
    
	#if CCL_PLATFORM_WINDOWS
	GET_DEVELOPMENT_FOLDER_LOCATION (defaultPath, CCL_FRAMEWORK_DIRECTORY "tools", "bin/win/gettext")

	if(defaultPath.isEmpty () || !System::GetFileSystem ().fileExists (defaultPath))
	{
		System::GetSystem ().getLocation (defaultPath, System::kAppSupportFolder);
		defaultPath.descend (CCLSTR ("gettext"), Url::kFolder);
	}
    #endif

    #if CCL_PLATFORM_MAC
    System::GetSystem ().getLocation (defaultPath, System::kAppSupportFolder);
    defaultPath.descend (CCLSTR ("Frameworks"), Url::kFolder);
    #endif
    
    path = defaultPath;
	return System::GetFileSystem ().fileExists (path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Gettext::execute (StringList& output, UrlRef exePath, ArgsRef args)
{
	int flags = System::kSuppressProcessGUI|System::kWaitForProcessExit;

	TempFile tempFile ("gettext_output");
	AutoPtr<IStream> outputStream = tempFile.open (IStream::kOpenMode|IStream::kWriteMode);
	flags |= System::kRedirectProcessOutput;

	Threading::ProcessID processId = 0;
	int exitCode = static_cast<int> (System::GetExecutableLoader ().execute (processId, exePath, args, flags, outputStream));

	outputStream.release ();
	TextUtils::loadStringList (output, tempFile.getPath (), true, Text::kUTF8);

	return exitCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void decodeOutput (const StringList& output, StringRef poString, StringRef poFileName, StringRef exeString)
{
	for(int i = 0, count = output.count (); i < count; i++)
	{
		String line (output[i]);
		if(line.replace (poString, poFileName, false) == 1) // error message starts with file name
		{
			String string = line.subString (poFileName.length ());
			if(string.at (0) == ':' && Unicode::isDigit (string.at (1))) // error message contains line number
			{
				string.remove (0, 1);
				int64 lineNumber = 0;
				if(string.getIntValue (lineNumber))
				{
					Alert::Event e (line, Alert::kError);
					e.fileName = poFileName;
					e.lineNumber = (int)lineNumber;
					ccl_raise (e);
					continue;
				}
			}
		}
		else
			line.replace (exeString, poFileName, false);

		ccl_raise (line);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Gettext::makeMO (UrlRef poPath, UrlRef moPath)
{
	String poFileName;
	poPath.getName (poFileName);
	
	String poString;
	poPath.toDisplayString (poString);
	String moString;
	moPath.toDisplayString (moString);

	String args[3];
	args[0] = poString;
	args[1] = "-o";
	args[2] = moString;

	Url exePath;
	getLocation (exePath);
	exePath.descend (msgfmtName);
	String exeString;
	exePath.toDisplayString (exeString);

	StringList output;
	int exitCode = execute (output, exePath, ArgumentList (3, args));
	if(exitCode != 0)
	{
		if(!output.isEmpty ())
			decodeOutput (output, poString, poFileName, exeString);
		else
			ccl_raise (String () << msgfmtName << " " << moString << ": " << exitCode);
	}
	return exitCode == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Gettext::merge (UrlRef poPath, UrlRef potPath)
{
	String poFileName;
	poPath.getName (poFileName);

	String poString;
	poPath.toDisplayString (poString);
	String potString;
	potPath.toDisplayString (potString);

	static const int kMergeArgCount = 6;
	String args[kMergeArgCount];
	args[0] = "-U"; // update input file
	args[1] = "-N"; // no fuzzy matching
	args[2] = "--backup=none";
	args[3] = "--no-wrap"; // do not linebreak long message lines
	args[4] = poString;
	args[5] = potString;

	Url exePath;
	getLocation (exePath);
	exePath.descend (msgmergeName);
	String exeString;
	exePath.toDisplayString (exeString);

	StringList output;
	int exitCode = execute (output, exePath, ArgumentList (kMergeArgCount, args));
	if(exitCode != 0)
	{
		if(!output.isEmpty ())
			decodeOutput (output, poString, poFileName, exeString);
		else
			ccl_raise (String () << msgmergeName << " " << poString << ": " << exitCode);
	}
	return exitCode == 0;
}
