#include "DebugContext.hpp"

using namespace Hindsight::Debugger;

/// <summary>
/// Construct a new DebugContext instance based on process handle and thread handle, these handles
/// must be opened with all access. This overload will fetch the thread context based on what mode 
/// the process is running in (native or WOW64).
/// </summary>
/// <param name="hProcess">The process handle.</param>
/// <param name="hThread">The thread handle to fetch the context for.</param>
DebugContext::DebugContext(HANDLE hProcess, HANDLE hThread) 
	: hProcess(hProcess), hThread(hThread) {

	IsWow64Process(hProcess, &IsWow64);

	if (IsWow64) {
		X86.ContextFlags = CONTEXT_ALL;
		Wow64GetThreadContext(hThread, &X86);
	} else {
		X64.ContextFlags = CONTEXT_ALL;
		GetThreadContext(hThread, &X64);
	}
}

/// <summary>
/// Construct a new DebugContext instance specifically for a native 64-bit process. It will still 
/// determine what mode the process is running in, so make sure to only use this constructor if you
/// are sure that this is a 64-bit native mode process.
/// </summary>
/// <param name="hProcess">The process handle.</param>
/// <param name="hThread">The thread handle that the <paramref name="ctx"/> belongs to.</param>
/// <param name="ctx">The thread context.</param>
DebugContext::DebugContext(HANDLE hProcess, HANDLE hThread, const CONTEXT& ctx)
	: hProcess(hProcess), hThread(hThread) {

	IsWow64Process(hProcess, &IsWow64);

	X64 = ctx;
}

/// <summary>
/// Construct a new DebugContext instance specifically for a WOW64 32-bit process. It will still 
/// determine what mode the process is running in, so make sure to only use this constructor if you
/// are sure that this is a 32-bit WOW64 mode process.
/// </summary>
/// <param name="hProcess">The process handle.</param>
/// <param name="hThread">The thread handle that the <paramref name="ctx"/> belongs to.</param>
/// <param name="ctx">The thread context.</param>
DebugContext::DebugContext(HANDLE hProcess, HANDLE hThread, const WOW64_CONTEXT& ctx) 
	: hProcess(hProcess), hThread(hThread) {

	IsWow64Process(hProcess, &IsWow64);

	X86 = ctx;
}

/// <summary>
/// Construct a new DebugContext instance based on a <see cref="PROCESS_INFORMATION"/> struct instance and 
/// a previously obtained thread context, for 64-bit native mode processes.
/// </summary>
/// <param name="pi">A const reference to a <see cref="PROCESS_INFORMATION"/> instance sourcing the <paramref name="ctx"/>.</param>
/// <param name="ctx">A const reference to the thread context.</param>
DebugContext::DebugContext(const PROCESS_INFORMATION& pi, const CONTEXT& ctx)
	: hProcess(pi.hProcess), hThread(pi.hThread), IsWow64(false), X64(ctx) {

}

#pragma warning( push )
#pragma warning( disable : 26495 ) /* no initialization required, only the initialized value is accessed */
/// <summary>
/// Construct a new DebugContext instance based on a <see cref="PROCESS_INFORMATION"/> struct instance and 
/// a previously obtained thread context, for 32-bit WOW64 mode processes.
/// </summary>
/// <param name="pi">A const reference to a <see cref="PROCESS_INFORMATION"/> instance sourcing the <paramref name="ctx"/>.</param>
/// <param name="ctx">A const reference to the thread context.</param>
DebugContext::DebugContext(const PROCESS_INFORMATION& pi, const WOW64_CONTEXT& ctx) 
	: hProcess(pi.hProcess), hThread(pi.hThread), IsWow64(true), X86(ctx) {

}
#pragma warning( pop )

/// <summary>
/// Determine if the context belongs to a 64-bit native process
/// </summary>
/// <returns>True is returned when the process for this context is a native 64-bit process.</returns>
bool DebugContext::Is64() const {
	return !IsWow64;
}

/// <summary>
/// Get a pointer to the internal <see cref="CONTEXT"/> or <see cref="WOW64_CONTEXT"/> instance contained in this DebugContext instance.
/// </summary>
/// <returns>A void pointer to either context.</returns>
/// <remarks>
/// Note that the memory pointed to by the resulting void pointer can be either context struct, be sure to use <see cref="Is64"/> to determine
/// what to do with this result.
/// </remarks>
const void* DebugContext::Get() const {
	return (IsWow64 ? (const void*)&X86 : (const void*)&X64);
}

/// <summary>
/// Get a const reference to the <see cref="WOW64_CONTEXT"/> instance contained within this DebugContext instance. 
/// </summary>
/// <returns>A const reference to the <see cref="WOW64_CONTEXT"/> part of the internal context union.</returns>
/// <remarks>
/// Ensure to only use this value when the process is a WOW64 mode process, otherwise the struct will be corrupted.
/// </remarks>
const WOW64_CONTEXT& DebugContext::Get86() const {
	return X86;
}

/// <summary>
/// Get a const reference to the <see cref="CONTEXT"/> instance contained within this DebugContext instance. 
/// </summary>
/// <returns>A const reference to the <see cref="CONTEXT"/> part of the internal context union.</returns>
/// <remarks>
/// Ensure to only use this value when the process is a 64-bit native mode process, otherwise the struct will be corrupted.
/// </remarks>
const CONTEXT& DebugContext::Get64() const {
	return X64;
}

/// <summary>
/// Returns the process handle for this context.
/// </summary>
/// <returns>A process handle, this handle will be closed by the debugger after processing an event.</returns>
const HANDLE DebugContext::GetProcess() const {
	return hProcess;
}

/// <summary>
/// Returns the thread handle for this context.
/// </summary>
/// <returns>A thread handle, this handle will be closed by the debugger after processing an event.</returns>
const HANDLE DebugContext::GetThread() const {
	return hThread;
}
