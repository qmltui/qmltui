#include "DetachedProcess.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <QDir>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace qmltui::runner_cli {

namespace {

// Formats one argument using Win32 CommandLineToArgvW / MSVC CRT quoting rules.
std::wstring quoteWindowsArgument(const std::wstring& arg) {
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) return arg;
    std::wstring out;
    out.push_back(L'"');
    for (auto it = arg.begin();; ++it) {
        std::size_t backslashes = 0;
        while (it != arg.end() && *it == L'\\') {
            ++it;
            ++backslashes;
        }
        if (it == arg.end()) {
            out.append(backslashes * 2, L'\\');
            break;
        }
        if (*it == L'"') {
            out.append(backslashes * 2 + 1, L'\\');
            out.push_back(L'"');
        } else {
            out.append(backslashes, L'\\');
            out.push_back(*it);
        }
    }
    out.push_back(L'"');
    return out;
}

} // namespace

bool startDetachedDaemon(const QString& program, const QStringList& arguments,
                         const QString& workingDirectory, qint64* pid) {
    if (program.isEmpty()) return false;

    const std::wstring appPath = QDir::toNativeSeparators(program).toStdWString();
    std::wstring cmdLine = quoteWindowsArgument(appPath);
    for (const QString& arg : arguments) {
        cmdLine.push_back(L' ');
        cmdLine += quoteWindowsArgument(arg.toStdWString());
    }
    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(L'\0');

    const std::wstring workDir = workingDirectory.isEmpty()
                                     ? std::wstring()
                                     : QDir::toNativeSeparators(workingDirectory).toStdWString();

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE nullHandle =
        CreateFileW(L"\\\\.\\NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nullHandle == INVALID_HANDLE_VALUE) return false;

    SIZE_T attrSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);
    if (attrSize == 0) {
        CloseHandle(nullHandle);
        return false;
    }

    std::vector<std::uint64_t> attrStorage((attrSize + sizeof(std::uint64_t) - 1) /
                                           sizeof(std::uint64_t));
    auto* attrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrStorage.data());
    if (!InitializeProcThreadAttributeList(attrList, 1, 0, &attrSize)) {
        CloseHandle(nullHandle);
        return false;
    }

    // Whitelist only the NUL handle so piped caller stdout/stderr handles (and
    // any inherited CTest pipes) cannot leak into the detached daemon.
    HANDLE inheritHandles[1] = {nullHandle};
    if (!UpdateProcThreadAttribute(attrList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inheritHandles,
                                   sizeof(inheritHandles), nullptr, nullptr)) {
        DeleteProcThreadAttributeList(attrList);
        CloseHandle(nullHandle);
        return false;
    }

    STARTUPINFOEXW siex{};
    siex.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    siex.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    siex.StartupInfo.hStdInput = nullHandle;
    siex.StartupInfo.hStdOutput = nullHandle;
    siex.StartupInfo.hStdError = nullHandle;
    siex.lpAttributeList = attrList;

    PROCESS_INFORMATION pi{};
    const DWORD creationFlags = EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT |
                                DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP;
    const BOOL ok = CreateProcessW(
        appPath.c_str(), cmdBuffer.data(), nullptr, nullptr, TRUE, creationFlags, nullptr,
        workDir.empty() ? nullptr : workDir.c_str(), &siex.StartupInfo, &pi);

    DeleteProcThreadAttributeList(attrList);
    CloseHandle(nullHandle);
    if (!ok) return false;

    if (pid) *pid = static_cast<qint64>(pi.dwProcessId);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

} // namespace qmltui::runner_cli
