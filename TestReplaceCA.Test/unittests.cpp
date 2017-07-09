#include "stdafx.h"
#include "CppUnitTest.h"
#include "MsiTestHelper.h"
#include <fstream>
#include <regex>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace MsiTest;

wstring EscapeString(const wstring & str)
{
	return regex_replace(str, wregex(L"\\\\|\""), L"\\$&");
}

namespace TestReplaceCATest
{
	TEST_CLASS(UnitTest1)
	{
	public:
		PMSIHANDLE2 GetMsi()
		{
			const wchar_t *createtable[] = {
			  L"CREATE TABLE `TextReplace` ("
			  L"`Id` CHAR(72) NOT NULL, "
			  L"`Component_` CHAR(72) NOT NULL, "
			  L"`File` CHAR(255) NOT NULL LOCALIZABLE, "
			  L"`MatchExpr` CHAR(255) NOT NULL, "
			  L"`ReplaceExpr` CHAR(255) NOT NULL "
			  L"PRIMARY KEY `Id`)",

			  L"INSERT INTO _Validation (`Table`, `Column`, `Nullable`, `Category`) VALUES ('TextReplace', 'Id', 'N', 'Identifier') TEMPORARY",
			  L"INSERT INTO _Validation (`Table`, `Column`, `Nullable`, `Category`) VALUES ('TextReplace', 'Component_', 'N', 'Identifier') TEMPORARY",
			  L"INSERT INTO _Validation (`Table`, `Column`, `Nullable`, `Category`) VALUES ('TextReplace', 'File', 'N', 'Formatted') TEMPORARY",
			  L"INSERT INTO _Validation (`Table`, `Column`, `Nullable`, `Category`) VALUES ('TextReplace', 'MatchExpr', 'N', 'Formatted') TEMPORARY",
			  L"INSERT INTO _Validation (`Table`, `Column`, `Nullable`, `Category`) VALUES ('TextReplace', 'ReplaceExpr', 'N', 'Formatted') TEMPORARY"
			};

			PMSIHANDLE2 handle;
			wchar_t dir[MAX_PATH], buffer[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, dir);
			wcscpy_s(buffer, dir);
			wcscat_s(buffer, L"\\TextReplace.Test.Setup.msi");
			UINT error = MsiOpenPackage(buffer, &handle);
			if (error || !handle)
			{
				Assert::Fail((wstring(L"Fail - could not open \"") + buffer + L"\".\n").c_str());
			}
			MsiSetProperty(handle, L"CurrentDir", dir);
			ExecuteQueries(handle, createtable);
			return handle;
		}

		TEST_METHOD(TestEscapeString)
		{
			Assert::AreEqual(wstring(L"\\\\&\\\""), EscapeString(L"\\&\""));
		}

		TEST_METHOD(TestInitTextReplaceIncludesComponentsThatAreInstalling)
		{
			CreateTextFile(__func__, "Hello World");
			const wchar_t *queries[] = {
			  L"INSERT INTO TextReplace (Id, Component_, File, MatchExpr, ReplaceExpr) VALUES ('_1', 'Comp1', '[CurrentDir]\\TestMethod1.txt', 'Hello', '$&$&') TEMPORARY"
			};
			auto hMsi = GetMsi();
			ExecuteQueries(hMsi, queries);
			MsiDoAction(hMsi, L"CostInitialize");
			MsiDoAction(hMsi, L"FileCost");
			MsiDoAction(hMsi, L"CostFinalize");
			INSTALLSTATE installed, action;
			MsiGetComponentState(hMsi, L"Comp1", &installed, &action);
			MsiSetComponentState(hMsi, L"Comp1", INSTALLSTATE_LOCAL);
			MsiGetComponentState(hMsi, L"Comp1", &installed, &action);
			Assert::AreEqual((int)INSTALLSTATE_LOCAL, (int)action);
			PerformAction(hMsi, L"TextReplaceCa.dll", "InitTextReplace");
			wchar_t data[4096];
			DWORD dataSize = _countof(data);
			MsiGetProperty(hMsi, L"TextReplace", data, &dataSize);
			wchar_t dir[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, dir);
			Assert::AreEqual(wstring(L"[\"_1\",\"Comp1\",\"") + EscapeString(dir) + L"\\\\TestMethod1.txt\",\"Hello\",\"$&$&\"]", wstring(data));
		}

		TEST_METHOD(TestInitTextReplaceFiltersOutComponentsThatAreNotInstalled)
		{
			CreateTextFile(__func__, "Hello World");
			const wchar_t *queries[] = {
			  L"INSERT INTO TextReplace (Id, Component_, File, MatchExpr, ReplaceExpr) VALUES ('_1', 'Comp1', '[CurrentDir]\\TestMethod1.txt', 'Hello', '$&$&') TEMPORARY"
			};
			auto hMsi = GetMsi();
			ExecuteQueries(hMsi, queries);
			PerformAction(hMsi, L"TextReplaceCa.dll", "InitTextReplace");
			wchar_t data[4096];
			DWORD dataSize = _countof(data);
			MsiGetProperty(hMsi, L"TextReplace", data, &dataSize);
			Assert::AreEqual(L"", data);
		}

		TEST_METHOD(TestReplaceReplacesTextFromFileSpecifiedInCustomActionDataProperty)
		{
			CreateTextFile("TestHappyPath", "Hello World");			
			auto hMsi = GetMsi();
			wchar_t dir[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, dir);
			MsiSetProperty(hMsi, L"CustomActionData", 
				(wstring(L"[\"_1\",\"Comp1\",\"") + EscapeString(dir) + L"\\\\TestHappyPath.txt\",\"Hello\",\"$&$&\"]").c_str());
			PerformAction(hMsi, L"TextReplaceCa.dll", "TextReplace");
			Assert::AreEqual(L"HelloHello World", ReadTextFile("TestHappyPath").c_str());
		}
	};
}