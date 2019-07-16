
// Log window support functions
// see log.h for the macros which are required to use this stuff

// TODO (chs): truncate long messages

#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#include <ws2ipdef.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"

#pragma comment(lib, "Ws2_32.lib")

// tracing

#if defined(CHS_DEBUG) && defined(_DEBUG)
#include "str.h"
#include "trace.h"
using chs::$;
#else
static void $(...)
{
}
#define trace(x) \
    if(false) {  \
        x;       \
    } else {     \
    }
#endif

// local

namespace
{

const int MINOR_VERSION = 1;
const int MAJOR_VERSION = 1;

typedef unsigned char byte;
typedef unsigned __int16 uint16;

// a simple mutex

struct Mutex
{
public:
    Mutex()
    {
        mutex = CreateMutex(NULL, FALSE, NULL);
    }
    void Acquire()
    {
        DWORD r = WaitForSingleObject(mutex, INFINITE);
        if(r != WAIT_OBJECT_0) {
            DebugBreak();
        }
    }
    void Release()
    {
        DWORD r = ReleaseMutex(mutex);
        if(r == 0) {
            DebugBreak();
        }
    }
    HANDLE mutex;
};

// TODO (chs): Locker & SocketCloser are similar, make a base + traits thingy

// mutex grabber/releaser

struct Locker
{
public:
    Locker(Mutex &locker) : lock(locker)
    {
        lock.Acquire();
    }
    ~Locker()
    {
        lock.Release();
    }
    Mutex &lock;
};

// for closing a socket at scope exit (if an error happened)

struct SocketCloser
{
public:
    SocketCloser(SOCKET &s) : socket(s), relinquished(false)
    {
    }
    void Relinquish()
    {
        relinquished = true;
    }
    ~SocketCloser()
    {
        if(!relinquished) {
            trace("Closing socket!");
            closesocket(socket);
            socket = INVALID_SOCKET;
        }
    }

private:
    SOCKET &socket;
    bool relinquished;
};

// message is a LogEntry

#pragma pack(push, 4)
struct LogEntry
{
    FILETIME timestamp;
    UINT32 severity;
    UINT32 line_number;
    UINT32 channel_len;
    UINT32 text_len;
    UINT32 filename_len;

    // followed by the 3 UTF8 strings (channel, text, filename) without null termination
};
#pragma pack(pop)

// we get one of these back from the debugger

#pragma pack(push, 1)
struct ServerAddress
{
    uint16 port;         // port allocated by server (might be localhost)
    byte family;         // AF_INET or AF_INET6
    byte version;        // major:minor
    byte address[16];    // for IPV4 only 1st 4 are used
};
#pragma pack(pop)

// overlapped + some bits

struct SendDetails
{
    WSAOVERLAPPED overlapped;
    WSABUF buffer;
    DWORD sent;
};

// buffer to sprintf Text into
WCHAR text_buffer[4096];

// max size of final Text
const size_t text_buffer_size = ARRAYSIZE(text_buffer);

// how to ensure this is unused by someone else?
UINT const my_exception_code = 0x00123456;

// send buffer admin
byte *buffer_head = nullptr;
int num_buffers = 0;
int const max_buffers = 4;
size_t const buffer_size = 4096;
HANDLE buffer_sent_event = CreateEvent(NULL, FALSE, FALSE, NULL);

// the socket we'll send log messages through
SOCKET log_socket = INVALID_SOCKET;

// some locks
Mutex pipe_lock;
Mutex file_lock;
Mutex socket_lock;
Mutex buffer_lock;

// wrong version / no plugin admin
const byte version = MINOR_VERSION | (MAJOR_VERSION << 4);
bool no_plugin = false;

// open connection to the debugger

void OpenSocket(ServerAddress &server_address)
{

    if(server_address.family != AF_INET && server_address.family != AF_INET6 || server_address.port < 1000 || server_address.version != version) {

        no_plugin = true;
        return;
    }

    Locker closer(pipe_lock);

    trace("OpenSocket\n");

    WSADATA wsa_data;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if(rc != 0) {
        return;
    }

    union ADDR
    {
        struct
        {
            USHORT sa_family;
            USHORT sin_port;
        } Header;
        SOCKADDR_IN6 V6;
        SOCKADDR_IN V4;
    };

    ADDR address = { 0 };
    int addressSize;

    log_socket = INVALID_SOCKET;

    // create the sockaddr
    trace($("Create SockAddr, family = %d, port = %d\n", server_address.family, server_address.port));

    address.Header.sa_family = server_address.family;
    address.Header.sin_port = htons(server_address.port);    // !#$*(&#*($&! htons
    if(server_address.family == AF_INET) {
        memcpy(&address.V4.sin_addr, &server_address.address, 4);
        addressSize = sizeof(SOCKADDR_IN);
    } else if(server_address.family == AF_INET6) {
        memcpy(&address.V6.sin6_addr, &server_address.address, 16);
        addressSize = sizeof(SOCKADDR_IN6);
    } else {
        return;
    }

    // create the socket

    trace("WSASocket\n");

    log_socket = WSASocket(server_address.family, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if(log_socket == INVALID_SOCKET) {
        trace($("Failed: %d\n", WSAGetLastError()));
        return;
    }

    SocketCloser socket_closer(log_socket);

    // more magic crap

    trace("ioctlsocket\n");

    u_long iMode = 1;
    int rs = ioctlsocket(log_socket, FIONBIO, &iMode);
    if(rs != NO_ERROR) {
        trace($("Failed: %d\n", WSAGetLastError()));
        return;
    }

    // connect it

    trace("WSAConnect\n");

    int bSuccess = WSAConnect(log_socket, (const sockaddr *)&address, addressSize, NULL, NULL, NULL, NULL);
    if(bSuccess != 0) {
        int w = WSAGetLastError();
        if(w == WSAEWOULDBLOCK) {
            fd_set writer;
            writer.fd_array[0] = log_socket;
            writer.fd_count = 1;
            trace("select\n");
            int sb = select(0, NULL, &writer, NULL, NULL);
            if(sb != 1) {
                trace($("Failed: %d\n", WSAGetLastError()));
            } else {
                socket_closer.Relinquish();
                trace("Success\n");
            }
        } else {
            trace($("Failed: %d\n", w));
        }
    }
}

void CloseSocket()
{
    trace("Closing socket\n");
    if(log_socket != INVALID_SOCKET) {
        linger l;
        l.l_linger = 5;
        l.l_onoff = 1;
        if(setsockopt(log_socket, SOL_SOCKET, SO_LINGER, (char const *)&l, sizeof(l)) != 0) {
            trace($("setsockopt failed: %d\n", WSAGetLastError()));
            return;
        }
        while(true) {
            int r = closesocket(log_socket);
            if(r == 0) {
                trace("Socket closed OK");
                break;
            }
            int w = WSAGetLastError();
            if(w != WSAEWOULDBLOCK) {
                trace($("Error closing socket: %d\n", w));
                break;
            } else {
                trace("Socket busy, hang on...");
                Sleep(1);
            }
        }
    }
}

// get debugger connection open

bool OpenLogSocket()
{
    if(!no_plugin && log_socket == INVALID_SOCKET) {

        atexit(CloseSocket);

        // debugger fills this in during the exception trigger
        ServerAddress server_address = { 0 };

        trace("raising exception\n");

        ULONG_PTR args[] = { (ULONG_PTR)sizeof(server_address), (ULONG_PTR)&server_address };
        __try {
            RaiseException(my_exception_code, 0, ARRAYSIZE(args), args);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            OpenSocket(server_address);
        }
    }
    return log_socket != INVALID_SOCKET;
}

// Write some UTF8 to a file

void WriteToFile(CHAR const *text)
{
    Locker lock(file_lock);
    static DWORD mode = CREATE_ALWAYS;
    HANDLE file = CreateFile(TEXT("D:\\LogWindow.txt"), GENERIC_WRITE, FILE_SHARE_READ, NULL, mode, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file != INVALID_HANDLE_VALUE) {
        mode = OPEN_EXISTING;
        DWORD wrote;
        WriteFile(file, text, (DWORD)strnlen_s(text, 65536), &wrote, NULL);
        CloseHandle(file);
    }
}

void WriteLogEntryToFile(LogEntry &log_entry)
{
    // TODO (chs): write the log entry as a csv compatible string
}

// add some ascii to the message buffer

int addToBuffer(LPSTR &buffer, CHAR const *text, int max)
{
    int len = (int)strnlen_s(text, max);
    memcpy(buffer, text, len);
    buffer += len;
    return len;
}

// add some UTF16 to the buffer

int addToBuffer(LPSTR &buffer, WCHAR const *text, int max)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer, max, NULL, NULL) - 1;
    buffer += len;
    return len;
}

// get a buffer for sending a message

byte *GetBuffer()
{

    Locker lock(buffer_lock);
    while(true) {
        byte *p = buffer_head;
        if(p != nullptr) {
            buffer_head = *(byte **)buffer_head;
            return p;
        } else if(num_buffers < max_buffers) {
            ++num_buffers;
            p = new byte[buffer_size];
            return p;
        } else {
            // wait for a buffer to have been sent
            WaitForSingleObjectEx(buffer_sent_event, INFINITE, TRUE);
        }
    }
    // can't get here
    return nullptr;
}

// return a buffer to the pool after send complete

void ReleaseBuffer(byte *buffer)
{
    Locker lock(buffer_lock);
    *(byte **)buffer = buffer_head;
    buffer_head = buffer;
}

// wsaoverlapped completed - a buffer was sent, free it and set the event so anyone waiting knows to look again

void CALLBACK send_complete(IN DWORD dwError, IN DWORD cbTransferred, IN LPWSAOVERLAPPED lpOverlapped, IN DWORD dwFlags)
{
    DWORD transfer;
    DWORD flags;
    WSAGetOverlappedResult(log_socket, lpOverlapped, &transfer, FALSE, &flags);
    SendDetails *details = (SendDetails *)lpOverlapped->hEvent;
    ReleaseBuffer((byte *)details);
    SetEvent(buffer_sent_event);
}

// Send to Output window
static WCHAR buffer[16384];

char const *severity_names[] = { "DEBUG", "VERBOSE", "INFO ", "WARN ", "ERROR" };

char const *SeverityName(int s)
{
    if(s < 0) {
        s = 0;
    }
    if(s >= _countof(severity_names)) {
        s = _countof(severity_names) - 1;
    }
    return severity_names[s];
}

#define PSTR(c)                                                                           \
    {                                                                                     \
        _snwprintf_s(buffer, _countof(buffer), c, SeverityName(severity), channel, text); \
        OutputDebugString(buffer);                                                        \
    }

void SendToOutput(CHAR const *channel, CHAR const *text, int severity)
{
    PSTR(L"@LOG [%S] [%S] %S\n");
}
void SendToOutput(CHAR const *channel, WCHAR const *text, int severity)
{
    PSTR(L"@LOG [%S] [%S] %s\n");
}
void SendToOutput(WCHAR const *channel, CHAR const *text, int severity)
{
    PSTR(L"@LOG [%S] [%s] %S\n");
}
void SendToOutput(WCHAR const *channel, WCHAR const *text, int severity)
{
    PSTR(L"@LOG [%S] [%s] %s\n");
}

// assemble and send a LogMessage

template <typename chan_t, typename text_t, typename file_t>
void Writer(chan_t const *channel, text_t const *text, file_t const *file, int line, int severity, FILETIME const &timestamp)
{

    if(OpenLogSocket()) {

        // get a send buffer
        LPSTR buffer_base = (LPSTR)GetBuffer();
        if(buffer_base == NULL) {
            return;
        }

        LPSTR buf_end = buffer_base + buffer_size;

        // details for the callback are at the front of it
        LPSTR message_base = buffer_base + sizeof(SendDetails);

        // then 1st thing actually sent is a LogEntry
        LogEntry *log_entry = (LogEntry *)message_base;

        // then the 3 strings
        LPSTR buffer_cur = message_base + sizeof(LogEntry);
        int channel_len = addToBuffer(buffer_cur, channel, (int)(buf_end - buffer_cur));    // TODO (chs): check for buffer size limit
        int text_len = addToBuffer(buffer_cur, text, (int)(buf_end - buffer_cur));
        int filename_len = addToBuffer(buffer_cur, file, (int)(buf_end - buffer_cur));

        // setup the details
        SendDetails *details = (SendDetails *)buffer_base;
        memset(&details->overlapped, 0, sizeof(WSAOVERLAPPED));
        details->overlapped.hEvent = (WSAEVENT)buffer_base;
        details->buffer.len = (DWORD)(buffer_cur - message_base);
        details->buffer.buf = (CHAR *)message_base;
        details->sent = 0;

        // init the LogEntry
        log_entry->timestamp = timestamp;
        log_entry->severity = severity;
        log_entry->line_number = line;
        log_entry->channel_len = channel_len;
        log_entry->text_len = text_len;
        log_entry->filename_len = filename_len;

        int rc = WSASend(log_socket, &details->buffer, 1, NULL, 0, &details->overlapped, send_complete);
        if(rc == SOCKET_ERROR) {
            int w = WSAGetLastError();
            if(w == WSA_IO_PENDING) {
                trace("Pending!\n");
                return;
            }
            // TODO (chs): report error
        }
        ReleaseBuffer((byte *)buffer_base);
    } else {
        SendToOutput(channel, text, severity);
    }
}

// log output with wide char format string

template <typename chan_t, typename file_t> void Log_OutputInternal(chan_t const *channel, WCHAR const *text, file_t const *file, int line, int severity, va_list v)
{
    if(IsDebuggerPresent()) {
        if(no_plugin) {
            vswprintf_s(text_buffer, text, v);
            SendToOutput(channel, text_buffer, severity);
        } else {
            FILETIME timestamp;
            GetSystemTimeAsFileTime(&timestamp);
            Locker lock(pipe_lock);
            vswprintf_s(text_buffer, text, v);
            Writer(channel, text_buffer, file, line, severity, timestamp);
        }
    }
}

// log output with ascii format string

template <typename chan_t, typename file_t> void Log_OutputInternal(chan_t const *channel, CHAR const *text, file_t const *file, int line, int severity, va_list v)
{
    if(IsDebuggerPresent()) {
        if(no_plugin) {
            vsprintf_s((char *const)text_buffer, text_buffer_size, text, v);    // wrong size by half, but consistent 16k char limit
            SendToOutput(channel, (char *const)text_buffer, severity);
        } else {
            FILETIME timestamp;
            GetSystemTimeAsFileTime(&timestamp);
            Locker lock(pipe_lock);
            vsprintf_s((char *const)text_buffer, text_buffer_size, text, v);    // wrong size by half, but consistent 16k char limit
            Writer(channel, (LPSTR)text_buffer, file, line, severity, timestamp);
        }
    }
}
}    // namespace

// output functions (channel, text, file can each be ascii or wide char const *)

namespace chs
{
namespace Log
{
namespace Internal
{

// can't template this without hoisting a bunch of stuff into the header

void Output(WCHAR const *channel, WCHAR const *text, WCHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
void Output(WCHAR const *channel, WCHAR const *text, CHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
void Output(WCHAR const *channel, CHAR const *text, WCHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
void Output(WCHAR const *channel, CHAR const *text, CHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
void Output(CHAR const *channel, WCHAR const *text, WCHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
void Output(CHAR const *channel, WCHAR const *text, CHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
void Output(CHAR const *channel, CHAR const *text, WCHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
void Output(CHAR const *channel, CHAR const *text, CHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
}
}
}

// C versions (all ascii or wide)

extern "C" void Log_OutputA(CHAR const *channel, CHAR const *text, CHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}

extern "C" void Log_OutputW(WCHAR const *channel, WCHAR const *text, WCHAR const *file, int line, int severity, ...)
{
    va_list v;
    va_start(v, severity);
    Log_OutputInternal(channel, text, file, line, severity, v);
}
