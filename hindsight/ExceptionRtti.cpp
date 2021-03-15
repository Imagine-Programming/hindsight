#include "ExceptionRtti.hpp"
#include "String.hpp"
#include <stdexcept>

using namespace Hindsight::Debugger::CxxExceptions;
using namespace Hindsight::Utilities;

/// <summary>
/// Process the RTTI as a 64-bit structure using RVA (relative to module base).
/// </summary>
void ExceptionRunTimeTypeInformation::Process64() const noexcept {
	auto parameters = reinterpret_cast<const EHParameters64*>(&m_Record->ExceptionInformation[0]);
	if (parameters == nullptr)
		return;

	// Fetch the module name of the code that threw the exception.
	auto exceptionModule = m_LoadedModules->GetModuleAtAddress(parameters->pThrowInfo);
	if (exceptionModule != nullptr) 
		m_ThrowImagePath = exceptionModule->Path;
	

	// Verify that we have the throw info address 
	if (parameters->pThrowInfo == nullptr)
		return;

	// Prepare the structs we will be reading from the process' memory space
	ThrowInfo64      throwInfo;
	CatchableType64  catchableType;
	TypeDescriptor64 typeDescriptor;

	// Read the throw info from the debugged process.
	if (!m_Process->Read(parameters->pThrowInfo, throwInfo))
		return;

	// Determine the address of the catchable type array
	auto typeArrayAddress = parameters->rva_to_va<const CatchableTypeArray64*>(throwInfo.pCatchableTypeArray);
	if (typeArrayAddress == nullptr)
		return;

	// Attempt to read the type array size, otherwise we cannot fetch each catchable type from the array.
	auto typeArraySize = 0;
	if (!m_Process->Read(reinterpret_cast<const void*>(typeArrayAddress), typeArraySize))
		return;

	// Calculate the length of the catchable type array, on x64 this is an array of ints (RVA offsets) + 1 int with the count.
	auto typeArrayBufferLength = static_cast<size_t>(typeArraySize * sizeof(int) + sizeof(int));
	auto typeArrayBuffer       = std::vector<char>(typeArrayBufferLength);                           // allocate space for it 
	auto typeArray             = reinterpret_cast<const CatchableTypeArray64*>(&typeArrayBuffer[0]); // reinterpret it.

	// Try to read the whole catchable type array 
	if (!m_Process->Read(static_cast<const void*>(typeArrayAddress), typeArrayBufferLength, static_cast<void*>(&typeArrayBuffer[0])))
		return;

	// Process each catchable type.
	auto containsStdException = false;
	for (auto i = 0; i < typeArraySize; ++i) {
		// Determine the address of the catchable type instance. 
		auto catchableTypeAddress = parameters->rva_to_va<const CatchableType64*>(typeArray->arrayOfCatchableTypes[i]);
		if (catchableTypeAddress == nullptr)
			return;

		// Try to read the catchable type.
		if (!m_Process->Read(reinterpret_cast<const void*>(catchableTypeAddress), catchableType))
			return;

		// Fetch the addresses for the type descriptor (and name member)
		auto typeDescriptorAddress = parameters->rva_to_va<const CatchableType64*>(catchableType.pType);
		auto typeNameAddress       = parameters->rva_to_va<const char*>(catchableType.pType + offsetof(TypeDescriptor64, name));

		// Stop when either is null
		if (typeDescriptorAddress == nullptr || typeNameAddress == nullptr)
			return;

		// Try to read the descriptor, excluding the name.
		if (!m_Process->Read(reinterpret_cast<const void*>(typeDescriptorAddress), typeDescriptor))
			return;

		// Try to read the decorated name.
		auto decoratedName = m_Process->ReadNulTerminatedString(typeNameAddress);
		if (decoratedName.empty())
			return;

		// Convert it to a type info pointer.
		auto typeInfoBuffer = std::vector<char>(sizeof(TypeDescriptor64) + decoratedName.size() + 1);
		memcpy(&typeInfoBuffer[0], &typeDescriptor, sizeof(TypeDescriptor64));
		memcpy(&typeInfoBuffer[sizeof(TypeDescriptor64)], decoratedName.c_str(), decoratedName.size() + 1);

		// Cast to type info
		auto typeInfo = reinterpret_cast<std::type_info*>(&typeInfoBuffer[0]);

		// Store full class name
		m_ExceptionTypeNames.push_back(std::string(typeInfo->name()));

		// Determine if this might contain a full exception message.
		if (!containsStdException)
			containsStdException = String::Contains(typeInfo->name(), std_exception);
	}

	// Try to fetch the full exception message if it is there, limited at 1024 characters.
	if (containsStdException) {
		const char* what = nullptr;
		if (m_Process->Read(static_cast<uint8_t*>(parameters->pExceptionObject) + 8, what)) {
			if (what != nullptr) {
				auto message = m_Process->ReadNulTerminatedString(what, 1024);
				if (!message.empty())
					m_ExceptionMessage = message;
			}
		}
	}
}

/// <summary>
/// Process the RTTI as a 32-bit structure using VA (non-relative, 32-bit address space).
/// </summary>
void ExceptionRunTimeTypeInformation::Process32() const noexcept {
	auto parameters = EHParameters32 {
		static_cast<unsigned long>(m_Record->ExceptionInformation[0]),
		reinterpret_cast<void*>(m_Record->ExceptionInformation[1]),
		reinterpret_cast<ThrowInfo32*>(m_Record->ExceptionInformation[2])
	};

	// Fetch the module name of the code that threw the exception.
	auto exceptionModule = m_LoadedModules->GetModuleAtAddress(parameters.pThrowInfo);
	if (exceptionModule != nullptr)
		m_ThrowImagePath = exceptionModule->Path;


	// Verify that we have the throw info address 
	if (parameters.pThrowInfo == nullptr)
		return;

	// Prepare the structs we will be reading from the process' memory space
	ThrowInfo32      throwInfo;
	CatchableType32  catchableType;
	TypeDescriptor32 typeDescriptor;

	// Read the throw info from the debugged process.
	if (!m_Process->Read(parameters.pThrowInfo, throwInfo))
		return;

	// Determine the address of the catchable type array
	auto typeArrayAddress = reinterpret_cast<const CatchableTypeArray32*>(static_cast<uintptr_t>(throwInfo.pCatchableTypeArray));
	if (typeArrayAddress == nullptr)
		return;

	// Attempt to read the type array size, otherwise we cannot fetch each catchable type from the array.
	auto typeArraySize = 0;
	if (!m_Process->Read(reinterpret_cast<const void*>(typeArrayAddress), typeArraySize))
		return;

	// Calculate the length of the catchable type array, on x86 this is an array of ints (32-bit VAs) + 1 int with the count.
	auto typeArrayBufferLength = static_cast<size_t>(typeArraySize * sizeof(int) + sizeof(int));
	auto typeArrayBuffer       = std::vector<char>(typeArrayBufferLength);                           // allocate space for it 
	auto typeArray             = reinterpret_cast<const CatchableTypeArray32*>(&typeArrayBuffer[0]); // reinterpret it.

	// Try to read the whole catchable type array 
	if (!m_Process->Read(static_cast<const void*>(typeArrayAddress), typeArrayBufferLength, static_cast<void*>(&typeArrayBuffer[0])))
		return;

	// Process each catchable type.
	auto containsStdException = false;
	for (auto i = 0; i < typeArraySize; ++i) {
		// Determine the address of the catchable type instance. 
		auto catchableTypeAddress = reinterpret_cast<const CatchableType32*>(static_cast<uintptr_t>(typeArray->arrayOfCatchableTypes[i]));
		if (catchableTypeAddress == nullptr)
			return;

		// Try to read the catchable type.
		if (!m_Process->Read(reinterpret_cast<const void*>(catchableTypeAddress), catchableType))
			return;

		// Fetch the addresses for the type descriptor (and name member)
		auto typeDescriptorAddress = reinterpret_cast<const CatchableType64*>(static_cast<uintptr_t>(catchableType.pType));
		auto typeNameAddress       = reinterpret_cast<const char*>(static_cast<uintptr_t>(catchableType.pType) + static_cast<uintptr_t>(offsetof(TypeDescriptor32, name)));

		// Stop when either is null
		if (typeDescriptorAddress == nullptr || typeNameAddress == nullptr)
			return;

		// Try to read the descriptor, excluding the name.
		if (!m_Process->Read(reinterpret_cast<const void*>(typeDescriptorAddress), typeDescriptor))
			return;

		// Try to read the decorated name.
		auto decoratedName = m_Process->ReadNulTerminatedString(typeNameAddress);
		if (decoratedName.empty())
			return;

		// Convert it to a type info pointer. 
		auto typeInfoBuffer = std::vector<char>(sizeof(std::type_info) + decoratedName.size() + 1);
#ifdef _WIN64
		memcpy(&typeInfoBuffer[sizeof(TypeDescriptor64)], decoratedName.c_str(), decoratedName.size() + 1);
#else 
		memcpy(&typeInfoBuffer[0], &typeDescriptor, sizeof(TypeDescriptor32));
		memcpy(&typeInfoBuffer[sizeof(TypeDescriptor32)], decoratedName.c_str(), decoratedName.size() + 1);
#endif 

		// Cast to type info
		auto typeInfo = reinterpret_cast<std::type_info*>(&typeInfoBuffer[0]);

		// Store full class name
		m_ExceptionTypeNames.push_back(std::string(typeInfo->name()));
		
		// Determine if this might contain a full exception message.
		if (!containsStdException)
			containsStdException = String::Contains(typeInfo->name(), std_exception);
	}
	
	// Try to fetch the full exception message if it is there, limited at 1024 characters.
	if (containsStdException) {
		int what = 0;
		if (m_Process->Read(static_cast<uint8_t*>(parameters.pExceptionObject) + 4, what)) {
			if (what != 0) {
				auto message = m_Process->ReadNulTerminatedString(reinterpret_cast<const void*>(what), 1024);
				if (!message.empty())
					m_ExceptionMessage = message;
			}
		}
	}
}

/// <summary>
/// Construct a new ExceptionRunTimeTypeInformation from a currently debugged process, an exception record and a module collection.
/// </summary>
/// <param name="process">The currently running (and suspended) process that is being debugged and is currently throwing an exception.</param>
/// <param name="record">The exception record that was obtained at the exact state <paramref name="process"/> is suspended in.</param>
/// <param name="loadedModules">The collection of loaded modules at the time of the exception.</param>
ExceptionRunTimeTypeInformation::ExceptionRunTimeTypeInformation(std::shared_ptr<Hindsight::Process::Process> process, const EXCEPTION_RECORD& record, const Hindsight::Debugger::ModuleCollection& loadedModules)
	: m_Process(process), m_Record(&record), m_LoadedModules(&loadedModules) {

	if (record.ExceptionCode != static_cast<DWORD>(EH_EXCEPTION_NUMBER))
		throw std::invalid_argument("provided exception record is not a MSVC++ EH Exception Code");

	if (record.ExceptionInformation[0] != static_cast<ULONG_PTR>(EH_MAGIC_NUMBER1))
		throw std::invalid_argument("provided exception record does not contain the EH_MAGIC_NUMBER1 magic number as its EHParameters magic field.");

	if (process->Is64()) {
		Process64();
	} else {
		Process32();
	}
}

/// <summary>
/// Construct a new ExceptionRunTimeTypeInformation from previously known RTTI (i.e. from a binary log file).
/// </summary>
/// <param name="exceptionTypeNames">The collection of demangled exception type names.</param>
/// <param name="message">The optional exception message.</param>
/// <param name="path">The optional module path to the module that constructed the ThrowInfo instance.</param>
ExceptionRunTimeTypeInformation::ExceptionRunTimeTypeInformation(const std::vector<std::string>& exceptionTypeNames, const std::optional<std::string>& message, const std::optional<std::wstring>& path) 
	: m_ExceptionTypeNames(exceptionTypeNames), m_ExceptionMessage(message), m_ThrowImagePath(path) {

}

/// <summary>
/// Fetch the collection of catchable type names which comprise this exception.
/// </summary>
/// <returns>A vector of strings with demangled type names.</returns>
const std::vector<std::string>& ExceptionRunTimeTypeInformation::exception_type_names() const noexcept {
	return m_ExceptionTypeNames;
}

/// <summary>
/// Fetch the optional exception message.
/// </summary>
/// <returns>The optional exception message.</returns>
const std::optional<std::string>& ExceptionRunTimeTypeInformation::exception_message() const noexcept {
	return m_ExceptionMessage;
}

/// <summary>
/// Fetch the optional full module path to the module that constructed the ThrowInfo instance.
/// </summary>
/// <returns>The optional module path.</returns>
const std::optional<std::wstring>& ExceptionRunTimeTypeInformation::exception_module_path() const noexcept {
	return m_ThrowImagePath;
}
