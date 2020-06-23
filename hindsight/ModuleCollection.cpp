#include "ModuleCollection.hpp"

using namespace Hindsight::Debugger;

/// <summary>
/// Construct a new Module instance with the base address, module size and path known. No information
/// has to be obtained from the remote process. 
/// </summary>
/// <param name="base">Module base address.</param>
/// <param name="size">Module size in memory.</param>
/// <param name="path">Module filepath.</param>
Module::Module(const ModulePointer& base, const size_t& size, const std::wstring& path) 
	: Base(base), Size(size), Path(path) {

}

/// <summary>
/// Construct a new Module instance without knowing its size, this constructor will try to retrieve that 
/// from the debugged process by processing the image headers.
/// </summary>
/// <param name="hProcess">A handle to the debugged process with the required permissions.</param>
/// <param name="base">Module base address.</param>
/// <param name="path">Module filepath.</param>
Module::Module(HANDLE hProcess, const ModulePointer& base, const std::wstring& path)
	: Base(base), Size(GetRemoteModuleSize(hProcess, base)), Path(path) {

}

/// <summary>
/// Default constructor for default initialization, used in structs.
/// </summary>
Module::Module() {}

/// <summary>
/// Determine if a symbol address is within the range of this loaded module.
/// </summary>
/// <param name="addr">The address to check.</param>
/// <returns>true is returned when this module contains the address.</returns>
inline bool Module::ContainsAddress(const void* addr) const {
	return (addr >= Base && addr < ((uint8_t*)Base + Size));
}

/// <summary>
/// Resolve the module size by reading the image headers from the debugged process.
/// </summary>
/// <param name="hProcess">A handle to the debugged process with the required permissions.</param>
/// <param name="base">Module base address.</param>
/// <returns>The module size, or 0 when something went wrong.</returns>
const size_t Module::GetRemoteModuleSize(HANDLE hProcess, const ModulePointer& base) {
	SIZE_T read;
	uint32_t offset;
	uint16_t machine;

	IMAGE_NT_HEADERS32 pe32;
	IMAGE_NT_HEADERS64 pe64;

	// Read the offset to the PE header from the DOS stub header
	if (!ReadProcessMemory(hProcess, (LPCVOID)((uint8_t*)base + 0x3c), &offset, sizeof(uint32_t), &read))
		return 0;

	// Read the machine type from the PE header
	if (!ReadProcessMemory(hProcess, (LPCVOID)((uint8_t*)base + offset + 4), &machine, sizeof(uint16_t), &read))
		return 0;

	switch (machine) {
		case IMAGE_FILE_MACHINE_AMD64: /* read the AMD64 NT headers */
			if (!ReadProcessMemory(hProcess, (LPCVOID)((uint8_t*)base + offset), &pe64, sizeof(IMAGE_NT_HEADERS64), &read))
				return 0;
			return pe64.OptionalHeader.SizeOfImage;
		case IMAGE_FILE_MACHINE_I386:  /* read the I386 NT headers*/
			if (!ReadProcessMemory(hProcess, (LPCVOID)((uint8_t*)base + offset), &pe32, sizeof(IMAGE_NT_HEADERS32), &read))
				return 0;
			return pe32.OptionalHeader.SizeOfImage;
		default:
			return 0;
	}
}

/// <summary>
/// Determines if a module with a certain path has been seen (loaded) before during the lifetime of this object.
/// </summary>
/// <param name="path">The module path.</param>
/// <returns>true is returned when the module has been seen before during the lifetime of this object.</returns>
bool ModuleCollection::Contains(const std::wstring& path) const {
	return m_ModuleIndexMap.count(path) != 0;
}

/// <summary>
/// Determines if a module with a certain path is currenty loaded in the state of this collection.
/// </summary>
/// <param name="path">The module path.</param>
/// <returns>true is returned when the module is currently active/loaded.</returns>
bool ModuleCollection::Active(const std::wstring& path) const {
	return m_ModuleHandleMap.count(path) != 0 && m_ModuleHandleMap.at(path).size() != 0;
}

/// <summary>
/// Determines if a module with a certain base address is currenty loaded in the state of this collection.
/// </summary>
/// <param name="moduleHandle">The module base address.</param>
/// <returns>true is returned when the module is currently active/loaded.</returns>
bool ModuleCollection::Active(ModulePointer moduleHandle) const {
	return m_ModuleMap.count(moduleHandle);
}

/// <summary>
/// Indicate that a module has been loaded, the size of the module will be retrieved from the process that 
/// loaded it.
/// </summary>
/// <param name="hProcess">A process handle for the process that loaded this module.</param>
/// <param name="path">The module path.</param>
/// <param name="moduleHandle">The module base address.</param>
void ModuleCollection::Load(HANDLE hProcess, const std::wstring& path, ModulePointer moduleHandle) {
	if (!Contains(path)) {
		m_Modules.push_back(path);
		m_ModuleIndexMap[path] = m_Modules.size() - 1;
	}

	m_ModuleMap.try_emplace(moduleHandle, hProcess, moduleHandle, path);
	m_ModuleHandleMap[path].insert(moduleHandle);
}

/// <summary>
/// Indicate that a module has been loaded, the size is known so no further information has to be retrieved
/// from the process that loaded the module.
/// </summary>
/// <param name="path">The module path.</param>
/// <param name="moduleHandle">The module base address.</param>
/// <param name="size">The module size.</param>
void ModuleCollection::Load(const std::wstring& path, ModulePointer moduleHandle, size_t size) {
	if (!Contains(path)) {
		m_Modules.push_back(path);
		m_ModuleIndexMap[path] = m_Modules.size() - 1;
	}

	m_ModuleMap.try_emplace(moduleHandle, moduleHandle, size, path);
	m_ModuleHandleMap[path].insert(moduleHandle);
}

/// <summary>
/// Indicate that a module with a certain base address has just been unloaded, which causes it to no longer 
/// be in the state for address resolution. It will still be kept in a list of modules that have been loaded
/// during the lifetime of this object for use in <see cref="Contains"/>.
/// </summary>
/// <param name="moduleHandle">The module base address.</param>
void ModuleCollection::Unload(ModulePointer moduleHandle) {
	if (!Active(moduleHandle))
		return;

	m_ModuleHandleMap.at(m_ModuleMap.at(moduleHandle).Path).erase(moduleHandle);
	m_ModuleMap.erase(moduleHandle);
}

/// <summary>
/// Get the module path for a module with a certain base address. 
/// </summary>
/// <param name="moduleHandle">The module base address.</param>
/// <returns>The module path, or an empty string if the module is no longer loaded.</returns>
const std::wstring ModuleCollection::Get(ModulePointer moduleHandle) const {
	if (Active(moduleHandle))
		return m_ModuleMap.at(moduleHandle).Path;

	return std::wstring(L"");
}

/// <summary>
/// Get all the module base addresses by looking up a certain path in the collection. A module can be loaded 
/// multiple times in different addresses in memory, hence why multiple base addresses can be returned.
/// </summary>
/// <param name="path">The module path.</param>
/// <returns>A set of module base addresses that have the module path.</returns>
const std::set<ModulePointer> ModuleCollection::Get(const std::wstring& path) const {
	if (Active(path))
		return m_ModuleHandleMap.at(path);

	return {};
}

/// <summary>
/// Get the index for a loaded module with a certain base address.
/// </summary>
/// <param name="moduleHandle">Module base address.</param>
/// <returns>The module index (load order) in the seen list, or -1 when the module is unknown or unloaded.</returns>
const int ModuleCollection::GetIndex(ModulePointer moduleHandle) const {
	if (Active(moduleHandle))
		return (int)m_ModuleIndexMap.at(m_ModuleMap.at(moduleHandle).Path);
	return -1;
}

/// <summary>
/// Get the index for a module with a certain path.
/// </summary>
/// <param name="path">Module path.</param>
/// <returns>The module index (load order) in the seen list, or -1 when the module is unknown.</returns>
const int ModuleCollection::GetIndex(const std::wstring& path) const {
	if (Contains(path))
		return (int)m_ModuleIndexMap.at(path);
	return -1;
}

/// <summary>
/// Try to resolve an address of a symbol or instruction to the module that contains that address.
/// </summary>
/// <param name="address">The address to resolve.</param>
/// <returns>A pointer to a <see cref="::Hindsight::Debugger::Module"/> instance, or <see langword="nullptr"/> when the address cannot be resolved.</returns>
const Module* ModuleCollection::GetModuleAtAddress(const void* address) const {
	for (const auto& m : m_ModuleMap) {
		if (m.second.ContainsAddress(address))
			return &m.second;
	}

	return nullptr;
}

/// <summary>
/// Retrieve a const vector of module paths, which effectively is the list of seen modules in order of loading at the 
/// time this method is called.
/// </summary>
/// <returns>A list of seen module paths.</returns>
const std::vector<std::wstring> ModuleCollection::GetModules() const {
	return m_Modules;
}