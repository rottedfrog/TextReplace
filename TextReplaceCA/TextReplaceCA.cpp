#include <SDKDDKVer.h>

#include <string>
#include <regex>
#include <algorithm>
#include <fstream>
#include "MsiHelper.h"

using namespace std;

const wchar_t *xmlSearchQuery = L"SELECT `Id`, `File`, `MatchExpr`, `ReplaceExpr`, `Component_` FROM `TextReplace`";

bool FileExists(const wstring &file)
{
	return GetFileAttributes(file.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::wstring GetTempFileName()
{
	wchar_t tempPath[MAX_PATH];
	wchar_t tempFilename[MAX_PATH];
	if (GetTempPath(MAX_PATH, tempPath) || GetTempFileName(tempPath, L"TF", 0, tempFilename))
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
		inFile >> line;
		outFile << regex_replace(line, rx, replaceExpression);
	}
}

void PerformReplace(MsiRecord &rec, MsiSession &session)
{
	//First check that the component is being installed
	if (!session.EvaluateCondition((L"$" + rec[4].GetString() + L" = 3").c_str()))
	{ return; }
	auto filename = rec[1].GetFormattedString();
	auto matchExpression = rec[2].GetFormattedString();
	auto replaceExpression = rec[3].GetFormattedString();
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
		DeleteFile(tempFilename.c_str());
		throw msi_exception(L"Failed to write to " + filename + L". Maybe the file is locked or read-only.");
	}
	DeleteFile(tempFilename.c_str());
	session.LogMessage(__FUNCTIONW__, (L"Successfully completed TextReplace for " + filename).c_str());
}

extern "C" UINT __stdcall TextReplace(MSIHANDLE hMsi)
{
	MsiSession session(hMsi);

	try
	{
		MsiView view = session.OpenView(xmlSearchQuery);
		for_each(begin(view), end(view), [&](MsiRecord &r) { PerformReplace(r, session); });
		return 0;
	}
	catch (msi_exception &err)
	{
		session.LogMessage(__FUNCTIONW__, err.what().c_str());
		return err.number();
	}
}