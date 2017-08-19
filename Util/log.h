#pragma once

#if defined(UNICODE)
#define __LOG__TQUOTE(quote) L##quote
#else
#define __LOG__TQUOTE(quote) quote
#endif
#define __LOG_TQUOTE2(x) __LOG__TQUOTE(x)

#if defined(__cplusplus)

namespace chs {

	namespace Log {

		namespace Severity {

        enum {
            Verbose = 0,
            Debug = 1,
            Info = 2,
            Warning = 3,
            Error = 4
        };
        }

		namespace Internal {
			void Output(wchar_t const *channel, wchar_t const *text, wchar_t const *file, int line, int severity, ...);
			void Output(wchar_t const *channel, wchar_t const *text, char const *file, int line, int severity, ...);
			void Output(wchar_t const *channel, char const *text, wchar_t const *file, int line, int severity, ...);
			void Output(wchar_t const *channel, char const *text, char const *file, int line, int severity, ...);
			void Output(char const *channel, wchar_t const *text, wchar_t const *file, int line, int severity, ...);
			void Output(char const *channel, wchar_t const *text, char const *file, int line, int severity, ...);
			void Output(char const *channel, char const *text, wchar_t const *file, int line, int severity, ...);
			void Output(char const *channel, char const *text, char const *file, int line, int severity, ...);
		}
	}
}

// specify severity

#define Log_Write(severity, chan, txt, ...) chs::Log::Internal::Output(chan, txt, __LOG_TQUOTE2(__FILE__), __LINE__, severity, __VA_ARGS__)

// with explicit channel

#define Log_Verbose(chan, txt, ...) Log_Write(chs::Log::Severity::Verbose, chan, txt, __VA_ARGS__)
#define Log_Debug(  chan, txt, ...) Log_Write(chs::Log::Severity::Debug,   chan, txt, __VA_ARGS__)
#define Log_Info(   chan, txt, ...) Log_Write(chs::Log::Severity::Info,    chan, txt, __VA_ARGS__)
#define Log_Warning(chan, txt, ...) Log_Write(chs::Log::Severity::Warning, chan, txt, __VA_ARGS__)
#define Log_Error(  chan, txt, ...) Log_Write(chs::Log::Severity::Error,   chan, txt, __VA_ARGS__)

// short one just uses function name as the channel

#define Log_V(txt, ...) Log_Write(chs::Log::Severity::Verbose, __FUNCTION__, txt, __VA_ARGS__)
#define Log_D(txt, ...) Log_Write(chs::Log::Severity::Debug,   __FUNCTION__, txt, __VA_ARGS__)
#define Log_I(txt, ...) Log_Write(chs::Log::Severity::Info,    __FUNCTION__, txt, __VA_ARGS__)
#define Log_W(txt, ...) Log_Write(chs::Log::Severity::Warning, __FUNCTION__, txt, __VA_ARGS__)
#define Log_E(txt, ...) Log_Write(chs::Log::Severity::Error,   __FUNCTION__, txt, __VA_ARGS__)

#else

// All this C stuff is completely untested

void Log_OutputW(wchar_t const *channel, wchar_t const *text, wchar_t const *file, int line, int severity, ...);
void Log_OutputA(char const *channel, char const *text, char const *file, int line, int severity, ...);

#if defined(UNICODE)
#define Log_Output Log_OutputW
#else
#define Log_Output Log_OutputA
#endif

enum {
	Severity_Verbose = 0,
	Severity_Debug = 1,
	Severity_Info = 2,
	Severity_Warning = 3,
	Severity_Error = 4
};

#define Log_Write(severity, chan, txt, ...) Log_Output(chan, txt, __LOG_TQUOTE2(__FILE__), __LINE__, severity, __VA_ARGS__)

#define Log_Verbose(chan, txt, ...) Log_Write(Severity_Verbose, chan, txt, __VA_ARGS__)
#define Log_Debug(  chan, txt, ...) Log_Write(Severity_Debug,   chan, txt, __VA_ARGS__)
#define Log_Info(   chan, txt, ...) Log_Write(Severity_Info,    chan, txt, __VA_ARGS__)
#define Log_Warning(chan, txt, ...) Log_Write(Severity_Warning, chan, txt, __VA_ARGS__)
#define Log_Error(  chan, txt, ...) Log_Write(Severity_Error,   chan, txt, __VA_ARGS__)

#define Log_V(txt, ...) Log_Write(Severity_Verbose, __FUNCTION__, txt, __VA_ARGS__)
#define Log_D(txt, ...) Log_Write(Severity_Debug,   __FUNCTION__, txt, __VA_ARGS__)
#define Log_I(txt, ...) Log_Write(Severity_Info,    __FUNCTION__, txt, __VA_ARGS__)
#define Log_W(txt, ...) Log_Write(Severity_Warning, __FUNCTION__, txt, __VA_ARGS__)
#define Log_E(txt, ...) Log_Write(Severity_Error,   __FUNCTION__, txt, __VA_ARGS__)

#endif

