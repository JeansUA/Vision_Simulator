#pragma once

#include <afxwin.h>
#include <cstdarg>

class CLogger
{
public:
    static void Log(LPCTSTR format, ...)
    {
        CString strTimestamp = GetTimestamp();
        CString strMessage;

        va_list args;
        va_start(args, format);
        strMessage.FormatV(format, args);
        va_end(args);

        CString strOutput;
        strOutput.Format(_T("[%s] %s\n"), (LPCTSTR)strTimestamp, (LPCTSTR)strMessage);
        OutputDebugString(strOutput);
    }

    static void Error(LPCTSTR format, ...)
    {
        CString strTimestamp = GetTimestamp();
        CString strMessage;

        va_list args;
        va_start(args, format);
        strMessage.FormatV(format, args);
        va_end(args);

        CString strOutput;
        strOutput.Format(_T("[%s][ERROR] %s\n"), (LPCTSTR)strTimestamp, (LPCTSTR)strMessage);
        OutputDebugString(strOutput);
    }

    static void Info(LPCTSTR format, ...)
    {
        CString strTimestamp = GetTimestamp();
        CString strMessage;

        va_list args;
        va_start(args, format);
        strMessage.FormatV(format, args);
        va_end(args);

        CString strOutput;
        strOutput.Format(_T("[%s][INFO] %s\n"), (LPCTSTR)strTimestamp, (LPCTSTR)strMessage);
        OutputDebugString(strOutput);
    }

private:
    static CString GetTimestamp()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);

        CString strTime;
        strTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        return strTime;
    }

    // Prevent instantiation
    CLogger() = delete;
    ~CLogger() = delete;
    CLogger(const CLogger&) = delete;
    CLogger& operator=(const CLogger&) = delete;
};
