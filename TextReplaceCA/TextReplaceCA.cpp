#include <SDKDDKVer.h>

#include <string>
#include <regex>
#include <algorithm>
#include <fstream>
#include "MsiHelper.h"

/* Things to test:
 * Happy Path
 * File missing
 * Line endings in text files (preserved ideally)
 * Component matching
 */
using namespace std;

const wchar_t *xmlSearchQuery = L"SELECT `Id`, `Component_`, `File`, `MatchExpr`, `ReplaceExpr` FROM `TextReplace`";

bool FileExists(const wstring &file)
{
	return GetFileAttributes(file.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::wstring GetTempFileName()
{
	wchar_t tempPath[MAX_PATH];
	wchar_t tempFilename[MAX_PATH];
	if (!GetTempPath(MAX_PATH, tempPath) || !GetTempFileName(tempPath, L"TF", 0, tempFilename))
	{
		throw msi_exception(L"Unable to create temporary file.");
	}
	return tempFilename;
}

void Transform(const wstring &infilename, const wstring &outfilename, const wstring &matchExpression, const wstring &replaceExpression)
{
	wifstream inFile(infilename);
	wofstream outFile(outfilename, ios::trunc);
	wregex rx(matchExpression);
	std::wstring line;
	while (!inFile.eof())
	{
    std::getline(inFile, line);
		outFile << regex_replace(line, rx, replaceExpression) << std::endl;
	}
}

void PerformReplace(MsiRecord &rec, MsiSession &session)
{
	//First check that the component is being installed
  session.LogMessage(__FUNCTIONW__, L"Text Replace %s for Component %s", rec[1].GetString().c_str(), rec[2].GetString().c_str());
	auto filename = rec[3].GetString();
	auto matchExpression = rec[4].GetString();
	auto replaceExpression = rec[5].GetString();
	session.LogMessage(__FUNCTIONW__, (L"Performing Replace on " + filename).c_str());
	session.LogMessage(__FUNCTIONW__, (L"Match expression: " + matchExpression).c_str());
	session.LogMessage(__FUNCTIONW__, (L"Replace expression: " + replaceExpression).c_str());
	//Now throw an error if can't find or access the file.
	if (!FileExists(filename))
	{
		throw msi_exception(filename + L" not found.");
	}
	auto tempFilename = GetTempFileName();

	Transform(filename, tempFilename, matchExpression, replaceExpression);
	
	if (!CopyFile(tempFilename.c_str(), filename.c_str(), FALSE))
	{
		session.LogLastDllError(__FUNCTIONW__);
#ifdef DEBUG
		MessageBox(NULL, L"DEBUG Me", L"Failed to copy file", 0);
#endif // DEBUG
		DeleteFile(tempFilename.c_str());
		throw msi_exception(L"Failed to write to " + filename + L". Maybe the file is locked or read-only.");
	}
	DeleteFile(tempFilename.c_str());
	session.LogMessage(__FUNCTIONW__, (L"Successfully completed TextReplace for " + filename).c_str());
}

extern "C" UINT __stdcall InitTextReplace(MSIHANDLE hMsi)
{
	MsiSession session(hMsi);

	try
	{
		MsiView view = session.OpenView(xmlSearchQuery);
    MsiViewEncoder encoder(0b11100); //Marks the last 3 fields as formatted;
    session.LogMessage(__FUNCTIONW__,  session.EvaluateFormattedString(L"$Comp1 = ").c_str());
    INSTALLSTATE installed, action;
    DWORD err = MsiGetComponentState(session, L"Comp1", &installed, &action);
    session.LogMessage(__FUNCTIONW__, L"$Comp1 = [1]", to_wstring((int)action));
    session.Properties[L"TextReplace"] = encoder.Encode(view, [&session](MsiRecord &rec) { return session.EvaluateCondition((L"$" + rec[2].GetString() + L" = 3").c_str()); });
		return 0;
	}
	catch (msi_exception &err)
	{
		session.LogMessage(__FUNCTIONW__, err.what().c_str());
		return err.number();
	}
}

extern "C" UINT __stdcall TextReplace(MSIHANDLE hMsi)
{
	MsiSession session(hMsi);

	try
	{
		MsiViewEncoder encoder(0);
    vector<MsiRecord> table = encoder.Decode(session, session.Properties[L"CustomActionData"]);
    for_each(begin(table), end(table), [&](MsiRecord &rec) { PerformReplace(rec, session); });
    return 0;
	}
	catch (msi_exception &err)
	{
		session.LogMessage(__FUNCTIONW__, err.what().c_str());
		return err.number();
	}
}