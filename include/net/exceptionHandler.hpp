#pragma once

#ifdef _WIN32
#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string>
#include <new.h>
#include <signal.h>
#include <exception>
#include <sys/stat.h>
#include <process.h>
#include <psapi.h>
#include <rtcapi.h>
#include <Shellapi.h>
#include <dbghelp.h>
#include <WinSock2.h>

#pragma comment(lib,"imagehlp.lib")
#pragma comment(lib,"Ws2_32.lib")

namespace mln::net
{

#ifndef _AddressOfReturnAddress

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

	EXTERNC void* _AddressOfReturnAddress(void);
	EXTERNC void* _ReturnAddress(void);
#endif //_AddressOfReturnAddress

	class ExceptionHandler
	{
	public:
		const static unsigned int BUFFER_COUNT = 1024;

		typedef void(*DUMP_FUNC)(EXCEPTION_POINTERS* pExcPtrs);
		inline static DUMP_FUNC s_dump_func = nullptr;

		inline static char s_filename[MAX_PATH] = { 0, };
		inline static char s_dump_path[MAX_PATH] = { 0, };

		static void generate_filename(char out[MAX_PATH], const char* header, const char* ext)
		{
			char hostname[50] = { 0, };
			char target_dir[MAX_PATH] = { 0, };

			memset(hostname, 0, sizeof(hostname));
			gethostname(hostname, sizeof(hostname));

			SYSTEMTIME st;
			GetLocalTime(&st);

			_snprintf_s(target_dir, MAX_PATH, _TRUNCATE
				, "%s\\%04d%02d%02d"
				, ExceptionHandler::s_dump_path, st.wYear, st.wMonth, st.wDay);

			// recursive creating directories
			try {
				std::string path_base(target_dir);
				std::string::size_type pos = 0;
				do
				{
					pos = path_base.find_first_of("\\/", pos + 1);
					CreateDirectory(path_base.substr(0, pos).c_str(), NULL);
				} while (pos != std::string::npos);
			}
			catch (...)
			{
				strncpy_s(ExceptionHandler::s_dump_path, MAX_PATH, ".", 1);
			}

			_snprintf_s(out, MAX_PATH, _TRUNCATE, "%s\\%s_%s_%s_%04d%02d%02d_%02d%02d%02d.%s"
				, target_dir
				, header
				, s_filename
				, hostname
				, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond
				, ext
			);
		}

		static void init(
			const char* root_path = nullptr
			, const char* header = nullptr
			, const bool isFullDump = true
		) {
			if (root_path && 0 < strlen(root_path)){
				strncpy_s(s_dump_path, sizeof(s_dump_path), root_path, _TRUNCATE);
			}
			else
			{
				char pathModule[MAX_PATH], drive[MAX_PATH], dir[MAX_PATH];
				memset(pathModule, 0, sizeof(pathModule));
				memset(drive, 0, sizeof(drive));
				memset(dir, 0, sizeof(dir));

				::GetModuleFileName(NULL, pathModule, MAX_PATH);
				_splitpath_s(pathModule, drive, sizeof(drive), dir, sizeof(dir), nullptr, 0, nullptr, 0);

				_snprintf_s(s_dump_path, MAX_PATH, _TRUNCATE
					, "%s\\%s"
					, drive, dir);
			}

			for (size_t i = strlen(s_dump_path) - 1; i >= 0; --i)
			{
				if ('\\' == s_dump_path[i] || '/' == s_dump_path[i])
					s_dump_path[i] = 0;
				else
					break;
			}

			if (0 >= strlen(s_dump_path))
			{
				s_dump_path[0] = '.';
			}

			// header
			if (header && 0 < strlen(header))
			{
				strncpy_s(s_filename, sizeof(s_filename), header, _TRUNCATE);
			}
			else
			{
				// getting filename
				char path_buffer[MAX_PATH];
				memset(path_buffer, 0, sizeof(path_buffer));
				memset(s_filename, 0, sizeof(s_filename));

				::GetModuleFileName(NULL, path_buffer, MAX_PATH);
				_splitpath_s(path_buffer, nullptr, 0, nullptr, 0, s_filename, sizeof(s_filename), nullptr, 0);
			}

			if (isFullDump)
				ExceptionHandler::s_dump_func = ExceptionHandler::CreateFullDump;
			else
				ExceptionHandler::s_dump_func = ExceptionHandler::CreateMiniDump;

			SetProcessExceptionHandlers();
			SetThreadExceptionHandlers();
		}

	private:

		// Sets exception handlers that work on per-process basis
		static void SetProcessExceptionHandlers() {
			SetUnhandledExceptionFilter(SehHandler);

			_set_purecall_handler(PureCallHandler);
			//	_set_new_handler(NewHandler);
			_set_invalid_parameter_handler(InvalidParameterHandler);
			_set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
			signal(SIGABRT, SigabrtHandler);
			signal(SIGINT, SigintHandler);
			signal(SIGTERM, SigtermHandler);
		}

		// Installs C++ exception handlers that function on per-thread basis
		static void SetThreadExceptionHandlers() {
			set_terminate(TerminateHandler);
			set_unexpected(UnexpectedHandler);
			typedef void(*sigh)(int);
			signal(SIGFPE, (sigh)SigfpeHandler);
			signal(SIGILL, SigillHandler);
			signal(SIGSEGV, SigsegvHandler);
		}

		static void GetExceptionPointers(DWORD dwExceptionCode, EXCEPTION_POINTERS** ppExceptionPointers){
			// The following code was taken from VC++ 8.0 CRT (invarg.c: line 104)

			EXCEPTION_RECORD ExceptionRecord;
			CONTEXT ContextRecord;
			memset(&ContextRecord, 0, sizeof(CONTEXT));

#ifdef _X86_
			__asm {
				mov dword ptr[ContextRecord.Eax], eax
				mov dword ptr[ContextRecord.Ecx], ecx
				mov dword ptr[ContextRecord.Edx], edx
				mov dword ptr[ContextRecord.Ebx], ebx
				mov dword ptr[ContextRecord.Esi], esi
				mov dword ptr[ContextRecord.Edi], edi
				mov word ptr[ContextRecord.SegSs], ss
				mov word ptr[ContextRecord.SegCs], cs
				mov word ptr[ContextRecord.SegDs], ds
				mov word ptr[ContextRecord.SegEs], es
				mov word ptr[ContextRecord.SegFs], fs
				mov word ptr[ContextRecord.SegGs], gs
				pushfd
				pop[ContextRecord.EFlags]
			}

			ContextRecord.ContextFlags = CONTEXT_CONTROL;
#pragma warning(push)
#pragma warning(disable:4311)
			ContextRecord.Eip = (ULONG)_ReturnAddress();
			ContextRecord.Esp = (ULONG)_AddressOfReturnAddress();
#pragma warning(pop)
			ContextRecord.Ebp = *((ULONG*)_AddressOfReturnAddress() - 1);


#elif defined (_IA64_) || defined (_AMD64_)

			/* Need to fill up the Context in IA64 and AMD64. */
			RtlCaptureContext(&ContextRecord);

#else  /* defined (_IA64_) || defined (_AMD64_) */

			ZeroMemory(&ContextRecord, sizeof(ContextRecord));

#endif  /* defined (_IA64_) || defined (_AMD64_) */

			ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));

			ExceptionRecord.ExceptionCode = dwExceptionCode;
			ExceptionRecord.ExceptionAddress = _ReturnAddress();

			///

			EXCEPTION_RECORD* pExceptionRecord = new EXCEPTION_RECORD;
			memcpy(pExceptionRecord, &ExceptionRecord, sizeof(EXCEPTION_RECORD));
			CONTEXT* pContextRecord = new CONTEXT;
			memcpy(pContextRecord, &ContextRecord, sizeof(CONTEXT));

			*ppExceptionPointers = new EXCEPTION_POINTERS;
			(*ppExceptionPointers)->ExceptionRecord = pExceptionRecord;
			(*ppExceptionPointers)->ContextRecord = pContextRecord;
		}

		static void CreateMiniDump(EXCEPTION_POINTERS* pExcPtrs) {
			HMODULE hDbgHelp = NULL;
			HANDLE hFile = NULL;
			MINIDUMP_EXCEPTION_INFORMATION mei;
			MINIDUMP_CALLBACK_INFORMATION mci;

			// Load dbghelp.dll
			hDbgHelp = LoadLibrary(_T("dbghelp.dll"));
			if (hDbgHelp == NULL)
			{
				// Error - couldn't load dbghelp.dll
				return;
			}

			char filename[MAX_PATH] = { 0, };
			generate_filename(filename, "MiniDump", "dmp");

			// Create the minidump file
			hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				return;
			}

			// Write minidump to the file
			mei.ThreadId = GetCurrentThreadId();
			mei.ExceptionPointers = pExcPtrs;
			mei.ClientPointers = FALSE;
			mci.CallbackRoutine = NULL;
			mci.CallbackParam = NULL;

			typedef BOOL(WINAPI* LPMINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile,
				MINIDUMP_TYPE DumpType,
				CONST		PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
				CONST		PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam,
				CONST		PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

			LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump = (LPMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
			if (!pfnMiniDumpWriteDump)
			{
				// Bad MiniDumpWriteDump function
				return;
			}

			HANDLE hProcess = GetCurrentProcess();
			DWORD dwProcessId = GetCurrentProcessId();

			BOOL bWriteDump = pfnMiniDumpWriteDump(hProcess, dwProcessId, hFile, MiniDumpNormal, &mei, NULL, &mci);

			if (!bWriteDump)
			{
				// Error writing dump.
				return;
			}

			// Close file
			CloseHandle(hFile);

			// Unload dbghelp.dll
			// FreeLibrary(hDbgHelp);
		}

		static void CreateFullDump(EXCEPTION_POINTERS* pExcPtrs) {
			HMODULE hDbgHelp = NULL;
			HANDLE hFile = NULL;
			MINIDUMP_EXCEPTION_INFORMATION mei;
			MINIDUMP_CALLBACK_INFORMATION mci;

			// Load dbghelp.dll
			hDbgHelp = LoadLibrary(_T("dbghelp.dll"));
			if (hDbgHelp == NULL)
			{
				// Error - couldn't load dbghelp.dll
				return;
			}


			char filename[MAX_PATH] = { 0, };
			generate_filename(filename, "FullDump", "dmp");

			// Create the minidump file
			hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				return;
			}

			// Write minidump to the file
			mei.ThreadId = GetCurrentThreadId();
			mei.ExceptionPointers = pExcPtrs;
			mei.ClientPointers = FALSE;
			mci.CallbackRoutine = NULL;
			mci.CallbackParam = NULL;

			typedef BOOL(WINAPI* LPMINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile,
				MINIDUMP_TYPE DumpType,
				CONST		PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
				CONST		PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam,
				CONST		PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

			LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump = (LPMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
			if (!pfnMiniDumpWriteDump)
			{
				// Bad MiniDumpWriteDump function
				return;
			}

			HANDLE hProcess = GetCurrentProcess();
			DWORD dwProcessId = GetCurrentProcessId();

			const DWORD Flags = MiniDumpWithFullMemory |
				MiniDumpWithFullMemoryInfo |
				MiniDumpWithHandleData |
				MiniDumpWithUnloadedModules |
				MiniDumpWithThreadInfo;

			BOOL bWriteDump = pfnMiniDumpWriteDump(hProcess, dwProcessId, hFile, (MINIDUMP_TYPE)Flags, &mei, NULL, &mci);

			if (!bWriteDump)
			{
				// Error writing dump.
				return;
			}

			// Close file
			CloseHandle(hFile);

			// Unload dbghelp.dll
			// FreeLibrary(hDbgHelp);
		}

		static void MyStackWalk(EXCEPTION_POINTERS* pExcPtrs) {
			SYSTEMTIME st;
			GetLocalTime(&st);

			SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);

			if (!SymInitialize(GetCurrentProcess(), NULL, TRUE)) return;



			HANDLE hFile = NULL;
			unsigned long  numOfByteWritten = 0;

			char filename[MAX_PATH] = { 0, };
			generate_filename(filename, "StackDump", "txt");

			hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				return;
			}

			char buff[BUFFER_COUNT] = { 0, };

			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "Date:%d-%d-%d_%d:%d:%d\r\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "Process:%x\r\n", GetCurrentProcessId());
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "Thread:%x\r\n", GetCurrentThreadId());
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "Exception Type:0x%08x\r\n", pExcPtrs->ExceptionRecord->ExceptionCode);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);


			HANDLE		hProcess = GetCurrentProcess();
			HANDLE		hThread = GetCurrentThread();
			CONTEXT& context = *pExcPtrs->ContextRecord;

			STACKFRAME	stackFrame = { 0, };

#if defined(_M_X64) || defined(_M_AMD64)
			stackFrame.AddrPC.Offset = context.Rip;
			stackFrame.AddrPC.Mode = AddrModeFlat;
			stackFrame.AddrStack.Offset = context.Rsp;
			stackFrame.AddrStack.Mode = AddrModeFlat;
			stackFrame.AddrFrame.Offset = context.Rbp;
			stackFrame.AddrFrame.Mode = AddrModeFlat;

			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "rax: %#p\trbx: %#p\r\n", (void*)context.Rax, (void*)context.Rbx);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "rcx: %#p\trdx: %#p\r\n", (void*)context.Rcx, (void*)context.Rdx);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "rsi: %#p\trdi: %#p\r\n", (void*)context.Rsi, (void*)context.Rdi);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "rbp: %#p\trsp: %#p\r\n", (void*)context.Rbp, (void*)context.Rsp);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "r8: %#p\tr9: %#p\r\n", (void*)context.R8, (void*)context.R9);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "r10: %#p\tr11: %#p\r\n", (void*)context.R10, (void*)context.R11);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "r12: %#p\tr13: %#p\r\n", (void*)context.R12, (void*)context.R13);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "r14: %#p\tr15: %#p\r\n", (void*)context.R14, (void*)context.R15);
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);


			for (int walkIdx = 0; walkIdx < 512; ++walkIdx) {
				if (0 == stackFrame.AddrPC.Offset) break;
				if (!StackWalk(IMAGE_FILE_MACHINE_AMD64, hProcess, hThread, &stackFrame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) break;

				PDWORD64			dw64Displacement = 0;
				PDWORD				dwDisplacement = 0;
				char				chSymbol[256] = { 0, };

				PIMAGEHLP_SYMBOL	pSymbol = (PIMAGEHLP_SYMBOL)chSymbol;
				pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
				pSymbol->MaxNameLength = sizeof(chSymbol) - sizeof(PIMAGEHLP_SYMBOL) + 1;

				if (SymGetSymFromAddr(hProcess, stackFrame.AddrPC.Offset, dw64Displacement, pSymbol))
				{
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "%#p - %s() + %lh\r\n", (void*)stackFrame.AddrPC.Offset, pSymbol->Name, (unsigned int)(stackFrame.AddrPC.Offset - pSymbol->Address));
					WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
				}
				else
				{
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "%#p - [Unknown Symbol:Error %u]\r\n", (void*)stackFrame.AddrPC.Offset, GetLastError());
					WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
				}

				IMAGEHLP_MODULE	module = { sizeof(IMAGEHLP_MODULE), 0 };
				if (SymGetModuleInfo(hProcess, stackFrame.AddrPC.Offset, &module) != FALSE)
				{
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\t\t\t\tImageName:%s\r\n", module.ImageName);
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\t\t\t\tLoadedImageName:%s\r\n", module.LoadedImageName);
				}

				IMAGEHLP_LINE line = { sizeof(IMAGEHLP_LINE), 0 };
				for (int lineIdx = 0; lineIdx < 512; ++lineIdx)
				{
					if (SymGetLineFromAddr(hProcess, stackFrame.AddrPC.Offset - lineIdx, dwDisplacement, &line) != FALSE)
					{
						_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\t\t\t\tFileName:%s\r\n", line.FileName);
						WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
						_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\t\t\t\tLineNumber:%u\r\n", line.LineNumber);
						WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
						break;
					}
				}
			}

			BYTE* stack = (BYTE*)context.Rsp;
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "stack %p - %p\r\n", stack, stack + ((sizeof(size_t) * 4) * 16));
			WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			for (int i = 0; i < sizeof(size_t) * 4; ++i) {
				_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "%p : ", stack + i * 16);
				WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
				for (int j = 0; j < 16; ++j) {
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "%02X ", stack[i * 16 + j]);
					WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
				}
				_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
				WriteFile(hFile, buff, static_cast<DWORD>(strlen(buff)), &numOfByteWritten, NULL);
			}
#else
			stackFrame.AddrPC.Offset = context.Eip;
			stackFrame.AddrPC.Mode = AddrModeFlat;
			stackFrame.AddrStack.Offset = context.Esp;
			stackFrame.AddrStack.Mode = AddrModeFlat;
			stackFrame.AddrFrame.Offset = context.Ebp;
			stackFrame.AddrFrame.Mode = AddrModeFlat;

			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
			WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "eax: 0x%08x\tebx: 0x%08x\r\n", context.Eax, context.Ebx);
			WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "ecx: 0x%08x\tedx: 0x%08x\r\n", context.Ecx, context.Edx);
			WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "esi: 0x%08x\tedi: 0x%08x\r\n", context.Esi, context.Edi);
			WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "ebp: 0x%08x\tesp: 0x%08x\r\n", context.Ebp, context.Esp);
			WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
			WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);


			for (int walkIdx = 0; walkIdx < 512; ++walkIdx) {
				if (0 == stackFrame.AddrPC.Offset) break;
				if (!StackWalk(IMAGE_FILE_MACHINE_I386, hProcess, hThread, &stackFrame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) break;

				PDWORD				dwDisplacement = 0;
				char				chSymbol[256] = { 0, };

				PIMAGEHLP_SYMBOL	pSymbol = (PIMAGEHLP_SYMBOL)chSymbol;
				pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
				pSymbol->MaxNameLength = sizeof(chSymbol) - sizeof(PIMAGEHLP_SYMBOL) + 1;

				if (SymGetSymFromAddr(hProcess, stackFrame.AddrPC.Offset, dwDisplacement, pSymbol))
				{
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "0x%08x - %s() + %xh\r\n", stackFrame.AddrPC.Offset, pSymbol->Name, stackFrame.AddrPC.Offset - pSymbol->Address);
					WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
				}
				else
				{
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "0x%08x - [Unknown Symbol:Error %d]\r\n", stackFrame.AddrPC.Offset, GetLastError());
					WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
				}

				IMAGEHLP_MODULE	module = { sizeof(IMAGEHLP_MODULE), 0 };
				if (SymGetModuleInfo(hProcess, stackFrame.AddrPC.Offset, &module) != FALSE)
				{
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\t\t\t\tImageName:%s\r\n", module.ImageName);
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\t\t\t\tLoadedImageName:%s\r\n", module.LoadedImageName);
				}

				IMAGEHLP_LINE line = { sizeof(IMAGEHLP_LINE), 0 };
				for (int lineIdx = 0; lineIdx < 512; ++lineIdx)
				{
					if (SymGetLineFromAddr(hProcess, stackFrame.AddrPC.Offset - lineIdx, dwDisplacement, &line) != FALSE)
					{
						_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\t\t\t\tFileName:%s\r\n", line.FileName);
						WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
						_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\t\t\t\tLineNumber:%d\r\n", line.LineNumber);
						WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
						break;
					}
				}
			}

			BYTE* stack = (BYTE*)context.Esp;
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
			WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
			_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "stack %08x - %08x\r\n", stack, stack + ((sizeof(size_t) * 4) * 16));
			WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
			for (int i = 0; i < sizeof(size_t) * 4; ++i) {
				_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "%08X : ", stack + i * 16);
				WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
				for (int j = 0; j < 16; ++j) {
					_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "%02X ", stack[i * 16 + j]);
					WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
				}
				_snprintf_s(buff, BUFFER_COUNT, _TRUNCATE, "\r\n");
				WriteFile(hFile, buff, strlen(buff), &numOfByteWritten, NULL);
			}
#endif
			SymCleanup(hProcess);

			CloseHandle(hFile);
		}

		/* Exception handler functions. */

		// Structured exception handler
		static LONG WINAPI SehHandler(PEXCEPTION_POINTERS pExceptionPtrs) {
			// Write minidump file

			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);


			TerminateProcess(GetCurrentProcess(), 1);

			// Unreacheable code  
			return EXCEPTION_EXECUTE_HANDLER;
		}

		// CRT terminate() call handler
		static void __cdecl TerminateHandler() {
			// Abnormal program termination (terminate() function was called)

			// Retrieve exception information
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);

			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT unexpected() call handler
		static void __cdecl UnexpectedHandler() {
			// Unexpected error (unexpected() function was called)


			// Retrieve exception information
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);


			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT Pure virtual method call handler
		static void __cdecl PureCallHandler() {
			// Pure virtual function call

			// Retrieve exception information
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);


			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT invalid parameter handler
		static void __cdecl InvalidParameterHandler(const wchar_t* expression
			, const wchar_t* function
			, const wchar_t* file
			, unsigned int line
			, [[maybe_unused]] uintptr_t pReserved
		) {
			// Invalid parameter exception

			// Retrieve exception information
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);

			// Write exception note
			do
			{
				char filename[MAX_PATH] = { 0, };
				generate_filename(filename, "InvalidParamDump", "txt");

				HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					unsigned long  numOfByteWritten = 0;
					wchar_t buff[ExceptionHandler::BUFFER_COUNT] = { 0, };

					_snwprintf_s(buff, ExceptionHandler::BUFFER_COUNT, _TRUNCATE, L"Exception : %s\r\n", expression);
					WriteFile(hFile, buff, static_cast<DWORD>(wcslen(buff) * sizeof(wchar_t)), &numOfByteWritten, NULL);
					_snwprintf_s(buff, ExceptionHandler::BUFFER_COUNT, _TRUNCATE, L"Function : %s\r\n", function);
					WriteFile(hFile, buff, static_cast<DWORD>(wcslen(buff) * sizeof(wchar_t)), &numOfByteWritten, NULL);
					_snwprintf_s(buff, ExceptionHandler::BUFFER_COUNT, _TRUNCATE, L"File : %s\r\n", file);
					WriteFile(hFile, buff, static_cast<DWORD>(wcslen(buff) * sizeof(wchar_t)), &numOfByteWritten, NULL);
					_snwprintf_s(buff, ExceptionHandler::BUFFER_COUNT, _TRUNCATE, L"Line : %u\r\n", line);
					WriteFile(hFile, buff, static_cast<DWORD>(wcslen(buff) * sizeof(wchar_t)), &numOfByteWritten, NULL);

					CloseHandle(hFile);
				}
			} while (0);

			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT new operator fault handler
		static int __cdecl NewHandler(size_t) {
			// 'new' operator memory allocation exception

			// Retrieve exception information
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);


			TerminateProcess(GetCurrentProcess(), 1);

			// Unreacheable code
			return 0;
		}

		// CRT SIGABRT signal handler
		static void SigabrtHandler(int) {
			// Caught SIGABRT C++ signal

			// Retrieve exception information
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);


			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT SIGFPE signal handler
		static void SigfpeHandler(int /*code*/, int subcode) {
			// Floating point exception (SIGFPE)

			EXCEPTION_POINTERS* pExceptionPtrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);

			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT sigill signal handler
		static void SigintHandler(int) {
			// Illegal instruction (SIGILL)

			// Retrieve exception information
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);

			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT sigill signal handler
		static void SigillHandler(int) {
			// Illegal instruction (SIGILL)

			// Retrieve exception information
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);

			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT sigint signal handler
		static void SigsegvHandler(int) {
			// Invalid storage access (SIGSEGV)

			PEXCEPTION_POINTERS pExceptionPtrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

			// Write minidump file
			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);

			TerminateProcess(GetCurrentProcess(), 1);
		}

		// CRT SIGSEGV signal handler
		static void SigtermHandler(int) {
			EXCEPTION_POINTERS* pExceptionPtrs = NULL;
			GetExceptionPointers(0, &pExceptionPtrs);

			ExceptionHandler::s_dump_func(pExceptionPtrs);
			MyStackWalk(pExceptionPtrs);

			TerminateProcess(GetCurrentProcess(), 1);
		}
	};

};//namespace mln::net

#endif // #ifdef _WIN32