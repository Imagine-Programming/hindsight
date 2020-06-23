#pragma once

#ifndef module_collection_h
#define module_collection_h
	#include <vector>
	#include <string>
	#include <map>
	#include <set>
	#include <Windows.h>

	namespace Hindsight {
		namespace Debugger {
			/// <summary>
			/// Use a simple void pointer for referring to module base addresses. These addresses mean nothing
			/// in the address space of hindsight, hence why they should be void. 
			/// </summary>
			using ModulePointer = void*;

			/// <summary>
			/// The Module struct represents a single loaded module in the debugged process, including the main
			/// process module. 
			/// </summary>
			struct Module {
				ModulePointer	Base = 0;
				size_t			Size = 0;
				std::wstring	Path = L"";

				/// <summary>
				/// Construct a new Module instance with the base address, module size and path known. No information
				/// has to be obtained from the remote process. 
				/// </summary>
				/// <param name="base">Module base address.</param>
				/// <param name="size">Module size in memory.</param>
				/// <param name="path">Module filepath.</param>
				Module(const ModulePointer& base, const size_t& size, const std::wstring& path);

				/// <summary>
				/// Construct a new Module instance without knowing its size, this constructor will try to retrieve that 
				/// from the debugged process by processing the image headers.
				/// </summary>
				/// <param name="hProcess">A handle to the debugged process with the required permissions.</param>
				/// <param name="base">Module base address.</param>
				/// <param name="path">Module filepath.</param>
				Module(HANDLE hProcess, const ModulePointer& base, const std::wstring& path);

				/// <summary>
				/// Default constructor for default initialization, used in structs.
				/// </summary>
				Module();

				/// <summary>
				/// Determine if a symbol address is within the range of this loaded module.
				/// </summary>
				/// <param name="addr">The address to check.</param>
				/// <returns>true is returned when this module contains the address.</returns>
				inline bool ContainsAddress(const void* addr) const;

				/// <summary>
				/// Resolve the module size by reading the image headers from the debugged process.
				/// </summary>
				/// <param name="hProcess">A handle to the debugged process with the required permissions.</param>
				/// <param name="base">Module base address.</param>
				/// <returns>The module size, or 0 when something went wrong.</returns>
				static const size_t GetRemoteModuleSize(HANDLE hProcess, const ModulePointer& base);
			};

			/// <summary>
			/// The ModuleCollection class is a container of modules that are loaded in a process and contains 
			/// functionality for resolving symbol (or code) addresses to a module the addresses belong to.
			/// </summary>
			class ModuleCollection {
				private:
					std::vector<std::wstring>							m_Modules;
					std::map<std::wstring, std::set<ModulePointer>>		m_ModuleHandleMap;
					std::map<ModulePointer, Module>						m_ModuleMap;
					std::map<std::wstring, size_t>						m_ModuleIndexMap;

				public:
					/// <summary>
					/// Determines if a module with a certain path has been seen (loaded) before during the lifetime of this object.
					/// </summary>
					/// <param name="path">The module path.</param>
					/// <returns>true is returned when the module has been seen before during the lifetime of this object.</returns>
					bool Contains(const std::wstring& path) const;

					/// <summary>
					/// Determines if a module with a certain path is currenty loaded in the state of this collection.
					/// </summary>
					/// <param name="path">The module path.</param>
					/// <returns>true is returned when the module is currently active/loaded.</returns>
					bool Active(const std::wstring& path) const;

					/// <summary>
					/// Determines if a module with a certain base address is currenty loaded in the state of this collection.
					/// </summary>
					/// <param name="moduleHandle">The module base address.</param>
					/// <returns>true is returned when the module is currently active/loaded.</returns>
					bool Active(ModulePointer moduleHandle) const;

					/// <summary>
					/// Indicate that a module has been loaded, the size of the module will be retrieved from the process that 
					/// loaded it.
					/// </summary>
					/// <param name="hProcess">A process handle for the process that loaded this module.</param>
					/// <param name="path">The module path.</param>
					/// <param name="moduleHandle">The module base address.</param>
					void Load(HANDLE hProcess, const std::wstring& path, ModulePointer moduleHandle);

					/// <summary>
					/// Indicate that a module has been loaded, the size is known so no further information has to be retrieved
					/// from the process that loaded the module.
					/// </summary>
					/// <param name="path">The module path.</param>
					/// <param name="moduleHandle">The module base address.</param>
					/// <param name="size">The module size.</param>
					void Load(const std::wstring& path, ModulePointer moduleHandle, size_t size);

					/// <summary>
					/// Indicate that a module with a certain base address has just been unloaded, which causes it to no longer 
					/// be in the state for address resolution. It will still be kept in a list of modules that have been loaded
					/// during the lifetime of this object for use in <see cref="Contains"/>.
					/// </summary>
					/// <param name="moduleHandle">The module base address.</param>
					void Unload(ModulePointer moduleHandle);

					/// <summary>
					/// Get the module path for a module with a certain base address. 
					/// </summary>
					/// <param name="moduleHandle">The module base address.</param>
					/// <returns>The module path, or an empty string if the module is no longer loaded.</returns>
					const std::wstring Get(ModulePointer moduleHandle) const;

					/// <summary>
					/// Get all the module base addresses by looking up a certain path in the collection. A module can be loaded 
					/// multiple times in different addresses in memory, hence why multiple base addresses can be returned.
					/// </summary>
					/// <param name="path">The module path.</param>
					/// <returns>A set of module base addresses that have the module path.</returns>
					const std::set<ModulePointer> Get(const std::wstring& path) const;

					/// <summary>
					/// Get the index for a loaded module with a certain base address.
					/// </summary>
					/// <param name="moduleHandle">Module base address.</param>
					/// <returns>The module index (load order) in the seen list, or -1 when the module is unknown or unloaded.</returns>
					const int GetIndex(ModulePointer moduleHandle) const;

					/// <summary>
					/// Get the index for a module with a certain path.
					/// </summary>
					/// <param name="path">Module path.</param>
					/// <returns>The module index (load order) in the seen list, or -1 when the module is unknown.</returns>
					const int GetIndex(const std::wstring& path) const;

					/// <summary>
					/// Try to resolve an address of a symbol or instruction to the module that contains that address.
					/// </summary>
					/// <param name="address">The address to resolve.</param>
					/// <returns>A pointer to a <see cref="::Hindsight::Debugger::Module"/> instance, or <see langword="nullptr"/> when the address cannot be resolved.</returns>
					const Module* GetModuleAtAddress(const void* address) const;

					/// <summary>
					/// Retrieve a const vector of module paths, which effectively is the list of seen modules in order of loading at the 
					/// time this method is called.
					/// </summary>
					/// <returns>A list of seen module paths.</returns>
					const std::vector<std::wstring> GetModules() const;
			};
		}
	}

#endif 