#include "stdafx.h"
#include "MsiTestHelper.h"
#include "CppUnitTest.h"
#include <fstream>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MsiTest
{
  void PerformAction(PMSIHANDLE2 &hMsi, const wstring &dllName, const char *function)
  {
    HMODULE lib = LoadLibrary(dllName.c_str());
    if (!lib)
    {
      Assert::Fail((L"Fail - could not open" + dllName + L".\n").c_str());
    }
    CustomActionFn *search = (CustomActionFn *)GetProcAddress(lib, function);
    if (!search)
    {
      Assert::Fail(L"Unable to find function.");
    }
    search(hMsi);
  }

  void CreateTextFile(const string &filenameWithoutExt, const string &content)
  {
    ofstream f(filenameWithoutExt + ".txt");
    f << content << endl;
  }

  wstring ReadTextFile(const string &filenameWithoutExt)
  {
    wifstream f(filenameWithoutExt + ".txt");
    wstring line;
    std::getline(f, line);
    return line;
  }

}