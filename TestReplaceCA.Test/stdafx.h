// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include <Windows.h>
#include <Msi.h>
#include <MsiQuery.h>

class PMSIHANDLE2
{
  MSIHANDLE m_h;
  PMSIHANDLE2(const PMSIHANDLE2 &) = delete;
  PMSIHANDLE2 &operator=(const PMSIHANDLE2 &) = delete;
public:
  PMSIHANDLE2() :m_h(0) {}
  PMSIHANDLE2(MSIHANDLE h) :m_h(h) {}
  ~PMSIHANDLE2() { if (m_h != 0) MsiCloseHandle(m_h); }
  void operator =(MSIHANDLE h) { if (m_h) MsiCloseHandle(m_h); m_h = h; }
  operator MSIHANDLE() { return m_h; }
  MSIHANDLE* operator &() { if (m_h) MsiCloseHandle(m_h); m_h = 0; return &m_h; }
  PMSIHANDLE2(PMSIHANDLE2 &&pmsi)
    : m_h(pmsi.m_h)
  {
    pmsi.m_h = 0;
  }
};

// Headers for CppUnitTest
#include "CppUnitTest.h"

// TODO: reference additional headers your program requires here
