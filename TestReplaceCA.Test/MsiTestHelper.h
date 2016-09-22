#pragma once
#include <string>

namespace MsiTest
{

  typedef UINT __stdcall CustomActionFn(MSIHANDLE);

  template<size_t _Size>
  void ExecuteQueries(PMSIHANDLE2 &hMsi, const wchar_t * (&queries)[_Size])
  {
    PMSIHANDLE2 db = MsiGetActiveDatabase(hMsi);
    if (!db)
    {
      Assert::Fail(L"Fail - could not retrieve database.");
    }
    for (const wchar_t *query : queries)
    {
      PMSIHANDLE view;
      wchar_t errorbuffer[BUFSIZ * 4];
      DWORD errorbufferlen = _countof(errorbuffer);
      switch (MsiDatabaseOpenView(db, query, &view))
      {
      case ERROR_SUCCESS:
        break;
      case ERROR_BAD_QUERY_SYNTAX:
        Assert::Fail((wstring(L"Bad syntax in query \"") + query + L"\".").c_str());
      case ERROR_INVALID_HANDLE:
        Assert::Fail((wstring(L"Failed to open query \"") + query + L"\", due to an invalid handle.").c_str());
      default:
        Assert::Fail((wstring(L"Failed to open query \"") + query + L"\" (unknown error).").c_str());
      }
      PMSIHANDLE record;
      switch (MsiViewExecute(view, record))
      {
      case ERROR_SUCCESS:
        break;
      case ERROR_FUNCTION_FAILED:
        MsiFormatRecord(hMsi, MsiGetLastErrorRecord(), errorbuffer, &errorbufferlen);
        Assert::Fail((wstring(L"Error executing query \"") + query + L"\" (" + errorbuffer + L").").c_str());
      case ERROR_INVALID_HANDLE:
        Assert::Fail((wstring(L"Failed to execute query \"") + query + L"\", due to an invalid handle.").c_str());
      default:
        Assert::Fail((wstring(L"Failed to execute query \"") + query + L"\" (unknown error).").c_str());
      }
    }
  }

  void PerformAction(PMSIHANDLE2 &hMsi, const std::wstring &dllName, const char *function);
  void CreateTextFile(const std::string &filenameWithoutExt, const std::string &content);
  std::wstring ReadTextFile(const std::string &filenameWithoutExt);

}


