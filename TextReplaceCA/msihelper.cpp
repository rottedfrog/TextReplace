#include <algorithm>
#include <sstream>
#include <regex>
#include <functional>
#include "msihelper.h"

using namespace std;

wstring MsiSession::EvaluateFormattedString(const wstring &str)
{
  wchar_t buffer[1024];
  DWORD bufferSize = 1024, e;

  if (str.length() == 0)
  {
    return str;
  }

  MSIHANDLE hRecord = MsiCreateRecord(1);
  e = MsiRecordSetString(hRecord, 0, str.c_str());

  if (e != ERROR_SUCCESS)
  {
    MsiErr(e, L"Function: " __FUNCTIONW__);
    return wstring();
  }

  e = MsiFormatRecord(_hInstall, hRecord, buffer, &bufferSize);
  if (e == ERROR_SUCCESS)
  { return buffer; }
  if (e == ERROR_MORE_DATA)
  {
    unique_ptr<wchar_t[]> bigbuffer(new wchar_t[bufferSize]);
    e = MsiFormatRecord(_hInstall, hRecord, bigbuffer.get(), &bufferSize);
    if (e == ERROR_SUCCESS)
    { return bigbuffer.get(); }
  }
  MsiErr(e, L"Function: " __FUNCTIONW__);
  _ASSERT(FALSE);//should never get here
  return wstring();
}

void MsiSession::LogMessage(const wchar_t *function, const wchar_t *message, ...)
{
	va_list argptr;
	va_start(argptr, message);
	int bufsiz = _vscwprintf(message, argptr);
	// if there is an error, then we can't sensibly log anything. Best option is to fail silently.
	if (bufsiz)
	{
		wchar_t *formattedmessage = new wchar_t[bufsiz + 1];
		if (vswprintf_s(formattedmessage, bufsiz + 1, message, argptr))
		{
			PMSIHANDLE hRecord = MsiCreateRecord(2);
			MsiRecordSetString(hRecord, 0, L"[1]: [2]");
			MsiRecordSetString(hRecord, 1, function);
			MsiRecordSetString(hRecord, 2, formattedmessage);
			MsiProcessMessage(_hInstall, INSTALLMESSAGE_INFO, hRecord);
#ifdef _DEBUG
			OutputDebugString(formattedmessage);
			OutputDebugString(L"\n");
#endif
		}
		delete[] formattedmessage;
	}
	va_end(argptr);
}

void MsiSession::LogLastDllError(const wchar_t *function)
{
	DWORD err = GetLastError();
	wchar_t *msgbuffer;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, err, 0, (LPWSTR)&msgbuffer, BUFSIZ, NULL);
	LogMessage(function, msgbuffer);
	LocalFree(msgbuffer);
}

bool MsiSession::EvaluateCondition(const wchar_t *condition)
{
	MSICONDITION ret = MsiEvaluateCondition(_hInstall, condition);
	if (ret == MSICONDITION_ERROR)
	{
		throw msi_exception(std::wstring(L"Invalid handle or condition while evaluating \"") + condition + L"\"");
	}
	return ret != MSICONDITION_FALSE;
}

void MsiSession::LogLastMsiError(const wchar_t *function)
{
	MSIHANDLE rec = MsiGetLastErrorRecord();
#ifdef _DEBUG
	{
		DWORD msgsize = 0;
		MsiFormatRecord(_hInstall, rec, (wchar_t *)&msgsize, &msgsize);
		wchar_t *message = new wchar_t[++msgsize];
		MsiFormatRecord(_hInstall, rec, message, &msgsize);
		wprintf(L"%s\n", message);
		delete[] message;
	}
#endif
	MsiProcessMessage(_hInstall, INSTALLMESSAGE_ERROR, rec);
}

MsiView MsiSession::OpenView(const wchar_t *query)
{
	return MsiView(*this, query);
}

void MsiSession::ExecuteQuery(const wchar_t *query)
{
	PMSIHANDLE db = MsiGetActiveDatabase(_hInstall);
	MSIHANDLE view;
	std::wstring errcontext(std::wstring(L"Could not execute query: ") + query);
	if (!db)
	{
		throw msi_exception(0, L"Could not get the active database", errcontext);
	}
	MsiErr(MsiDatabaseOpenView(db, query, &view), errcontext);
	MsiErr(MsiViewExecute(view, 0), errcontext);
	MsiViewClose(view);
}

std::wstring MsiField::GetFormattedString() const
{
  return _session.EvaluateFormattedString(GetString());
}

std::wstring MsiField::GetString() const
{
	wchar_t *buffer = NULL;
	unsigned int bufsiz = 0, e;
	if (ERROR_MORE_DATA == (e = MsiRecordGetStringW(_record.hRecord(), _index, (wchar_t *)&buffer, (LPDWORD)&bufsiz)))
	{
		buffer = new wchar_t[++bufsiz];
		e = MsiRecordGetString(_record.hRecord(), _index, buffer, (LPDWORD)&bufsiz);
		std::wstring s(buffer);
		delete[] buffer;
		return s;
	}
	else
	{
		_session.MsiErr(e, errorcontext(__FUNCTIONW__));
		return std::wstring();
	}
}

void MsiSession::MsiErr(unsigned int errorcode, const std::wstring &context) const
{
	switch (errorcode)
	{
	case ERROR_SUCCESS:
	case ERROR_MORE_DATA:
		return;
	case ERROR_INVALID_DATATYPE:
		throw msi_exception(errorcode, L"Invalid field datatype", context);
	case ERROR_FUNCTION_FAILED:
		throw msi_exception(errorcode, L"Function Failed", context);
	case ERROR_BAD_QUERY_SYNTAX:
		throw msi_exception(errorcode, L"Bad query syntax", context);
	case ERROR_INVALID_HANDLE:
		throw msi_exception(errorcode, L"Invalid msi handle", context);
	case ERROR_INVALID_PARAMETER:
		throw msi_exception(errorcode, L"Invalid parameter", context);
	case ERROR_INVALID_FIELD:
		throw msi_exception(errorcode, L"Invalid field index", context);
	case ERROR_INVALID_HANDLE_STATE:
		throw msi_exception(errorcode, L"Invalid handle state", context);
	default:
		throw msi_exception(errorcode, L"Unknown failure", context);
	}
}

std::wstring MsiField::errorcontext(const wchar_t *function) const
{
	std::wostringstream str;
	str << L"Field Index: " << _index << L"\n"
		<< _record.errorcontext(function);
	return str.str();
}

std::wstring MsiRecord::errorcontext(const wchar_t *function)
{
	if (_it)
	{
		return _it->errorcontext(function);
	}
	else
	{
		return std::wstring(L"Function: ") + function;
	}
}

std::wstring MsiViewIterator::errorcontext(const wchar_t *function)
{
	return _view.errorcontext(function);
}

std::wstring MsiView::errorcontext(const wchar_t *function)
{
	return std::wstring(L"Query: ") + _query + L"\nFunction: " + std::wstring(function);
}

MsiField MsiRecord::operator[](unsigned int i)
{
	return MsiField(_session, *this, i);
}

MsiRecord MsiViewIterator::FetchRecord()
{
	MSIHANDLE record;
	UINT e = MsiViewFetch(hMsiView(), &record);
	if (e == ERROR_FUNCTION_FAILED)
	{
		ExecuteView();
		e = MsiViewFetch(hMsiView(), &record);
	}
	if (e == ERROR_NO_MORE_ITEMS)
	{
		return MsiRecord(_session, 0);
	}
	_session.MsiErr(e, errorcontext(__FUNCTIONW__));
	return MsiRecord(_session, record);
}

void MsiProperties::Property::operator=(const std::wstring &value)
{
	_session.MsiErr(MsiSetProperty(_session, _name.c_str(), value.c_str()), errorcontext(__FUNCTIONW__));
}

MsiProperties::Property::operator std::wstring()
{
	DWORD bufsiz = 0;
	_session.MsiErr(MsiGetProperty(_session, _name.c_str(), (wchar_t *)&bufsiz, &bufsiz), errorcontext(__FUNCTIONW__));
	std::vector<wchar_t> buffer(++bufsiz);
	_session.MsiErr(MsiGetProperty(_session, _name.c_str(), &buffer[0], &bufsiz), errorcontext(__FUNCTIONW__));
	return std::wstring(&buffer[0]);
}

wstring EscapeString(const wstring & str)
{
  return regex_replace(str, wregex(L"\\\\|\""), L"\\$&");
}

void MsiViewEncoder::SerializeRecord(MsiRecord &rec, wstringstream &stream) const
{
  stream << L"[";
  unsigned int count = rec.FieldCount();
  for (unsigned int i = 1; i <= count; ++i)
  {
    if (i > 1)
    { stream << L','; }

    stream << L'"' << EscapeString(((_formatMap >> i) & 1) ? rec[i].GetFormattedString() : rec[i].GetString()) << L'"';
  }
  stream << L']';
}

size_t FindEndQuote(const wstring &str, size_t pos)
{
  bool skipNext = false;
  while (pos < str.size())
  {
    if (str[pos] == L'"' && !skipNext)
    {
      return pos;
    }

    skipNext = !skipNext && (str[pos] == L'\\');
    ++pos;
  }
  return 0;
}

wstring RemoveEscapes(const wstring &str, size_t offset, size_t length)
{
  unique_ptr<wchar_t[]> result(new wchar_t[length + 1]);
  bool nextLit = false;
  size_t pos = 0;
  for (size_t i = offset; i < offset + length; ++i)
  {
    if (nextLit || str[i] != '\\')
    {
      result[pos++] = str[i];
      nextLit = false;
    }
    else
    {
      nextLit = true;
    }
  }

  return wstring(result.get(), result.get() + pos);
}

bool TryGetFields(vector<wstring> &fields, size_t &pos, const wstring &encodedView)
{
  fields.clear();
  if (pos + 1 < encodedView.size() && encodedView[pos] == '[')
  {
    while (encodedView[pos] != ']')
    {
      ++pos;
      if (encodedView[pos] != '\"')
      {
        throw msi_exception(L"Invalid field found in encoded view");
      }
      size_t end = FindEndQuote(encodedView, pos + 1);
      if (end == 0 || (encodedView[end + 1] != L',' && encodedView[end + 1] != L']'))
      {
        throw msi_exception(L"Invalid field found in encoded view");
      }
      fields.emplace_back(RemoveEscapes(encodedView, pos + 1, end - pos - 1));
      pos = end + 1;
    }
    ++pos;
    return true;
  }
  return false;
}

vector<MsiRecord> MsiViewEncoder::Decode(MsiSession &session, wstring encodedView) const
{
  vector<MsiRecord> result;
  size_t pos = 0;
  vector<wstring> fields;
  while (TryGetFields(fields, pos, encodedView))
  {
    result.emplace_back(session, MsiCreateRecord(fields.size()));
    MsiRecord &rec = result.back();
    for (size_t i = 0; i < fields.size(); ++i)
    {
      rec[i + 1] = fields[i];
    }
  }
  return result;
}
