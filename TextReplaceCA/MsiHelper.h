#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <memory>

#include <windows.h>
#include <Msi.h>
#include <MsiQuery.h>

class MsiView;
class MsiRecord;
class MsiSession;

class MsiProperties
{
private:
	MsiSession *_session;
public:
	MsiProperties(MsiSession *session)
		:_session(session)
	{ }

	class Property
	{
	private:
		MsiSession &_session;
	public:
		std::wstring _name;

		Property(MsiSession &session, const std::wstring &name)
			: _session(session), _name(name)
		{ }

		std::wstring errorcontext(const wchar_t *function)
		{
			return std::wstring(L"Property: ") + _name + L"\nFunction" + std::wstring(function);
		}

		void operator=(const std::wstring &value);
		operator std::wstring();
	};

	Property operator[](const std::wstring &name)
	{
		return Property(*_session, name);
	};
};

class MsiSession
{
private:
	MSIHANDLE _hInstall;
public:
	MsiProperties Properties;

	MsiSession(MSIHANDLE hMsi)
#pragma warning(suppress: 4355) // Yes I know I'm using the this pointer in the initialiser list. At this point it's just assignment, so it's fine. Don't bother me about it.
		: _hInstall(hMsi), Properties(this)
	{ }

	operator MSIHANDLE()
	{
		return _hInstall;
	}

	void LogMessage(const wchar_t *function, const wchar_t *message, ...);
	void LogLastDllError(const wchar_t *function);
	void LogLastMsiError(const wchar_t *function);
	MsiView OpenView(const wchar_t *query);
	void MsiErr(unsigned int errorcode, std::wstring &context);
	bool GetMode(MSIRUNMODE mode)
	{
		return MsiGetMode(_hInstall, mode) != 0;
	}

	void ExecuteQuery(const wchar_t *query);
	bool EvaluateCondition(const wchar_t *condition);
};

class MsiField;

class MsiRecord
{
private:
	friend class MsiViewIterator;
	friend class MsiField;
	MsiViewIterator *_it;
	MsiSession &_session;
	std::shared_ptr<void> _record;

	MSIHANDLE hRecord() const
	{
		return (MSIHANDLE)((LONG_PTR)_record.get() & 0xFFFFFFFF);
	}

	MsiRecord();
public:
	std::wstring errorcontext(const wchar_t *function);
	MsiRecord(MsiViewIterator *it, MsiSession &session, MSIHANDLE record)
		: _it(it), _session(session), _record((void *)record, [](void *msihandle) { MsiCloseHandle((MSIHANDLE)((LONG_PTR)msihandle & 0xFFFFFFFF)); })
	{ }

	MsiRecord(MsiSession &session, MSIHANDLE record)
		: _it(nullptr), _session(session), _record((void *)record, [](void *msihandle) { MsiCloseHandle((MSIHANDLE)((LONG_PTR)msihandle & 0xFFFFFFFF)); })
	{ }

	MsiRecord(const MsiRecord &) = default;
	
	void operator=(MsiRecord &&rec)
	{
		_it = rec._it;
		_session = rec._session;
		_record = rec._record;
	}

	MsiField operator[](unsigned int i);

	unsigned int FieldCount() const
	{
		return MsiRecordGetFieldCount(hRecord());
	}

	MsiRecord(MsiRecord &&msirecord)
		: _session(msirecord._session), _record(msirecord._record)
	{ }

};

class MsiField
{
private:
	MsiSession &_session;
	unsigned int _index;
	MsiRecord &_record;
	MsiField();
public:
	MsiField(MsiSession &session, MsiRecord &record, unsigned int index)
		: _session(session), _record(record), _index(index)
	{ }

	std::wstring errorcontext(const wchar_t *function);

	unsigned int Size()
	{
		return MsiRecordDataSize(_record.hRecord(), _index);
	}

	bool IsNull(int value)
	{
		return (MsiRecordIsNull(_record.hRecord(), _index) != 0);
	}

	void SetInt(int value)
	{
		_session.MsiErr(MsiRecordSetInteger(_record.hRecord(), _index, value), errorcontext(__FUNCTIONW__));
	}

	void SetString(wchar_t *value)
	{
		_session.MsiErr(MsiRecordSetString(_record.hRecord(), _index, value), errorcontext(__FUNCTIONW__));
	}

	void SetStream(wchar_t *value)
	{
		_session.MsiErr(MsiRecordSetStream(_record.hRecord(), _index, value), errorcontext(__FUNCTIONW__));
	}

	void GetStream(wchar_t *value, BYTE *buffer, unsigned int bufsiz)
	{
		_session.MsiErr(MsiRecordReadStream(_record.hRecord(), _index, (char *)buffer, (LPDWORD)&bufsiz), errorcontext(__FUNCTIONW__));
	}

	int GetInt()
	{
		return MsiRecordGetInteger(_record.hRecord(), _index);
	}

	std::wstring GetString();
	std::wstring GetFormattedString();
};

class MsiViewIterator : public std::iterator<std::forward_iterator_tag, MsiRecord>
{
private:
	MsiView &_view;
	MsiSession &_session;
	std::shared_ptr<void> _viewhandle;
	MsiRecord _record;
public:
	MSIHANDLE hMsiView() const
	{
		return (MSIHANDLE)((LONG_PTR)_viewhandle.get() & 0xFFFFFFFF);
	}

	void ExecuteView()
	{
		_session.MsiErr(MsiViewExecute(hMsiView(), 0), errorcontext(__FUNCTIONW__));
	}

	MsiRecord FetchRecord();
	MsiViewIterator();
	MsiViewIterator(MsiSession &session, MsiView &view)
		: _view(view), _session(session), _record(session, 0)
	{ }

	std::wstring errorcontext(const wchar_t *function);

	MsiViewIterator(MsiSession &session, MSIHANDLE hMsiView, MsiView &view)
		: _view(view), _session(session), _viewhandle((void *)hMsiView, [](void *handle) { MsiViewClose((MSIHANDLE)((LONG_PTR)handle & 0xFFFFFFFF)); }), _record(FetchRecord())
	{  }

	MsiViewIterator &operator++()
	{
		_record = FetchRecord();
		return *this;
	}

	bool operator==(MsiViewIterator &view)
	{
		return (_record.hRecord() == view._record.hRecord()) &&
			((_viewhandle == view._viewhandle) ||
			(_record.hRecord() == NULL));
	}

	bool operator!=(MsiViewIterator &view)
	{
		return !operator==(view);
	}

	MsiRecord operator*()
	{
		return _record;
	}

};

class MsiView
{
private:
	MsiSession &_session;
	std::wstring _query;
	std::vector<MSIHANDLE> _openViews;

	MSIHANDLE OpenView()
	{
		MSIHANDLE viewhandle;
		PMSIHANDLE db = MsiGetActiveDatabase(_session);
		_session.MsiErr(MsiDatabaseOpenView(db, _query.c_str(), &viewhandle), errorcontext(__FUNCTIONW__));
		_openViews.emplace_back(viewhandle);
		return viewhandle;
	}
	MsiView(const MsiView &) = delete;
	MsiView();
public:
	typedef MsiViewIterator iterator;
	std::wstring errorcontext(const wchar_t *function);
	MsiView(MsiSession &session, const wchar_t *query)
		: _session(session), _query(query)
	{ }

	MsiView(MsiView &&view)
		: _session(view._session), _query(view._query)
	{ }

	~MsiView()
	{
		for (MSIHANDLE view : _openViews)
		{
			MsiViewClose(view);
		}
	}

	MsiViewIterator begin()
	{
		return MsiViewIterator(_session, OpenView(), *this);
	}

	MsiViewIterator end()
	{
		return MsiViewIterator(_session, *this);
	}
};

class msi_exception
{
private:
	const std::wstring _msg;
	unsigned int _number;
public:
	msi_exception(unsigned int number, const std::wstring &msg, const std::wstring &context)
		: _number(number), _msg(msg + L"\n" + context)
	{ }

	msi_exception(const std::wstring &msg)
		: _number(-1), _msg(msg)
	{ }

	const std::wstring &what() const
	{
		return _msg;
	}

	int number() const
	{
		return _number;
	}

	virtual ~msi_exception()
	{ }
};