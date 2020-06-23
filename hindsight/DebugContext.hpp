#pragma once

#ifndef debugger_debug_context_h
#define debugger_debug_context_h
	#include <Windows.h>
	#include <DbgHelp.h>

	namespace Hindsight {
		namespace Debugger {

			/// <summary>
			/// The DebugContext class describes a thread CPU context snapshot, either a 64-bit context 
			/// or a WOW64 context (a 32-bit context through WOW64_CONTEXT)
			/// </summary>
			class DebugContext {
				private:
					BOOL IsWow64; /* if true, use X86 */

					union {
						CONTEXT			X64 = { 0 };
						WOW64_CONTEXT	X86;
					};

				public:
					const HANDLE hProcess;
					const HANDLE hThread;

					/// <summary>
					/// Construct a new DebugContext instance based on process handle and thread handle, these handles
					/// must be opened with all access. This overload will fetch the thread context based on what mode 
					/// the process is running in (native or WOW64).
					/// </summary>
					/// <param name="hProcess">The process handle.</param>
					/// <param name="hThread">The thread handle to fetch the context for.</param>
					DebugContext(HANDLE hProcess, HANDLE hThread);

					/// <summary>
					/// Construct a new DebugContext instance specifically for a native 64-bit process. It will still 
					/// determine what mode the process is running in, so make sure to only use this constructor if you
					/// are sure that this is a 64-bit native mode process.
					/// </summary>
					/// <param name="hProcess">The process handle.</param>
					/// <param name="hThread">The thread handle that the <paramref name="ctx"/> belongs to.</param>
					/// <param name="ctx">The thread context.</param>
					DebugContext(HANDLE hProcess, HANDLE hThread, const CONTEXT& ctx);

					/// <summary>
					/// Construct a new DebugContext instance specifically for a WOW64 32-bit process. It will still 
					/// determine what mode the process is running in, so make sure to only use this constructor if you
					/// are sure that this is a 32-bit WOW64 mode process.
					/// </summary>
					/// <param name="hProcess">The process handle.</param>
					/// <param name="hThread">The thread handle that the <paramref name="ctx"/> belongs to.</param>
					/// <param name="ctx">The thread context.</param>
					DebugContext(HANDLE hProcess, HANDLE hThread, const WOW64_CONTEXT& ctx);

					/// <summary>
					/// Construct a new DebugContext instance based on a <see cref="PROCESS_INFORMATION"/> struct instance and 
					/// a previously obtained thread context, for 64-bit native mode processes.
					/// </summary>
					/// <param name="pi">A const reference to a <see cref="PROCESS_INFORMATION"/> instance sourcing the <paramref name="ctx"/>.</param>
					/// <param name="ctx">A const reference to the thread context.</param>
					DebugContext(const PROCESS_INFORMATION& pi, const CONTEXT& ctx);

					/// <summary>
					/// Construct a new DebugContext instance based on a <see cref="PROCESS_INFORMATION"/> struct instance and 
					/// a previously obtained thread context, for 32-bit WOW64 mode processes.
					/// </summary>
					/// <param name="pi">A const reference to a <see cref="PROCESS_INFORMATION"/> instance sourcing the <paramref name="ctx"/>.</param>
					/// <param name="ctx">A const reference to the thread context.</param>
					DebugContext(const PROCESS_INFORMATION& pi, const WOW64_CONTEXT& ctx);

					/// <summary>
					/// Determine if the context belongs to a 64-bit native process
					/// </summary>
					/// <returns>True is returned when the process for this context is a native 64-bit process.</returns>
					bool Is64() const;

					/// <summary>
					/// Get a pointer to the internal <see cref="CONTEXT"/> or <see cref="WOW64_CONTEXT"/> instance contained in this DebugContext instance.
					/// </summary>
					/// <returns>A void pointer to either context.</returns>
					/// <remarks>
					/// Note that the memory pointed to by the resulting void pointer can be either context struct, be sure to use <see cref="Is64"/> to determine
					/// what to do with this result.
					/// </remarks>
					const void* Get() const;

					/// <summary>
					/// Get a const reference to the <see cref="WOW64_CONTEXT"/> instance contained within this DebugContext instance. 
					/// </summary>
					/// <returns>A const reference to the <see cref="WOW64_CONTEXT"/> part of the internal context union.</returns>
					/// <remarks>
					/// Ensure to only use this value when the process is a WOW64 mode process, otherwise the struct will be corrupted.
					/// </remarks>
					const WOW64_CONTEXT& Get86() const;

					/// <summary>
					/// Get a const reference to the <see cref="CONTEXT"/> instance contained within this DebugContext instance. 
					/// </summary>
					/// <returns>A const reference to the <see cref="CONTEXT"/> part of the internal context union.</returns>
					/// <remarks>
					/// Ensure to only use this value when the process is a 64-bit native mode process, otherwise the struct will be corrupted.
					/// </remarks>
					const CONTEXT& Get64() const;

					/// <summary>
					/// Returns the process handle for this context.
					/// </summary>
					/// <returns>A process handle, this handle will be closed by the debugger after processing an event.</returns>
					const HANDLE GetProcess() const;

					/// <summary>
					/// Returns the thread handle for this context.
					/// </summary>
					/// <returns>A thread handle, this handle will be closed by the debugger after processing an event.</returns>
					const HANDLE GetThread() const;
			};

		}
	}

#endif 