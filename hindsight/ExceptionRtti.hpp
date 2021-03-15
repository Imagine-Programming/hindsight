#pragma once

#ifndef cxx_exceptions_rtti_h
#define cxx_exceptions_rtti_h

#include "Process.hpp"
#include "ModuleCollection.hpp"

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <type_traits>
#include <typeinfo>

namespace Hindsight::Debugger::CxxExceptions {
	// Much of these definitions are from the MSVC++ CRT ehdata.h and ehdata_forceinclude.h 

	#define EH_EXCEPTION_NUMBER ('msc' | 0xE0000000) // The NT Exception # that we use, 0xe06d7363
	#define EH_EXCEPTION_THREAD_NAME 0x406D1388 // The thread-naming exception

	// As magic numbers increase, we have to keep track of the versions that we are
	// backwards compatible with.  The top 3 bits of the magic number DWORD are
	// used for other purposes, so while the magic number started as a YYYYMMDD
	// date, it can't represent the year 2000 or later.  Old CRTs also recognize
	// unknown magic numbers with a >= test.  Therefore, we just increment the
	// the magic number by one every time we need another.
	//
	// EH_MAGIC_NUMBER1     The original, still used in exception records for
	//                      native or mixed C++ thrown objects.
	// EH_MAGIC_NUMBER2     Used in the FuncInfo if exception specifications are
	//                      supported and FuncInfo::pESTypeList is valid.
	// EH_MAGIC_NUMBER3     Used in the FuncInfo if FuncInfo::EHFlags is valid, so
	//                      we can check if the function was compiled -EHs or -EHa.

	#define EH_MAGIC_NUMBER1        0x19930520
	#define EH_MAGIC_NUMBER2        0x19930521
	#define EH_MAGIC_NUMBER3        0x19930522

	#pragma pack(push, exception_handling_rtti, 4)

	//
	// PMD - Pointer to Member Data: generalized pointer-to-member descriptor
	//
	typedef struct PMD {
		int	mdisp;	// Offset of intended data within base
		int	pdisp;	// Displacement to virtual base pointer
		int	vdisp;	// Index within vbTable to offset of base
	} PMD;

	//
	// PMFN - Pointer to Member Function
	//
	typedef	int	PMFN64;						// Image relative offset of Member Function
	typedef	int	PMFN32;						// 32-bit VA
	//typedef void(__cdecl* PMFN32)(void*);

	//
	// TypeDescriptor - per-type record which uniquely identifies the type.
	//
	// Each type has a decorated name which uniquely identifies it, and a hash
	// value which is computed by the compiler.  The hash function used is not
	// important; the only thing which is essential is that it be the same for
	// all time.
	//
	// The special type '...' (ellipsis) is represented by a null name.
	//
	#pragma warning(push)
	#pragma warning(disable:4200)	// nonstandard extension used: array of runtime bound

	#pragma pack(push, TypeDescriptor64, 8)
	typedef struct TypeDescriptor64 {
		const void* pVFTable; // Field overloaded by RTTI
		void *      spare;    // reserved, possible for RTTI
		char        name[];   // The decorated name of the type; 0 terminated.
	} TypeDescriptor64;
	#pragma pack(pop, TypeDescriptor64)
	
	typedef struct TypeDescriptor32 {
		unsigned long hash;   // Hash value computed from type's decorated name
		int           spare;  // reserved, possible for RTTI
		char          name[]; // The decorated name of the type; 0 terminated.
	} TypeDescriptor32;

	#pragma warning(pop)

	/////////////////////////////////////////////////////////////////////////////
	//
	// Description of the thrown object.
	//
	// This information is broken down into three levels, to allow for maximum
	// comdat folding (at the cost of some extra pointers).
	//
	// ThrowInfo is the head of the description, and contains information about
	// 				the particular variant thrown.
	// CatchableTypeArray is an array of pointers to type descriptors.  It will
	//				be shared between objects thrown by reference but with varying
	//				qualifiers.
	// CatchableType is the description of an individual type, and how to effect
	//				the conversion from a given type.
	//
	//---------------------------------------------------------------------------


	//
	// CatchableType - description of a type that can be caught.
	//
	// Note:  although isSimpleType can be part of ThrowInfo, it is more
	//		  convenient for the run-time to have it here.
	//
	typedef struct _s_CatchableType64 {
		unsigned int		properties;         // Catchable Type properties (Bit field)
		int					pType;              // Image relative offset of TypeDescriptor
		PMD 				thisDisplacement;   // Pointer to instance of catch type within thrown object.
		int					sizeOrOffset;       // Size of simple-type object or offset into
												//  buffer of 'this' pointer for catch object
		PMFN64				copyFunction;		// Copy constructor or CC-closure
	} CatchableType64;

	typedef struct _s_CatchableType32 {
		unsigned int		properties;         // Catchable Type properties (Bit field)
		int					pType;              // 32-bit VA to the type descriptor for this type
		PMD 				thisDisplacement;   // Pointer to instance of catch type within thrown object.
		int					sizeOrOffset;       // Size of simple-type object or offset into
												//  buffer of 'this' pointer for catch object
		PMFN32				copyFunction;       // Copy constructor or CC-closure
	} CatchableType32;

	//
	// CatchableTypeArray - array of pointers to catchable types, with length
	//
	#pragma warning (push)
	#pragma warning (disable:4200)	// nonstandard extension used: array of runtime bound
	typedef struct _s_CatchableTypeArray64 {
		int	nCatchableTypes;
		int arrayOfCatchableTypes[]; // Image relative offset of CatchableType64 instances 
		} CatchableTypeArray64;

	typedef struct _s_CatchableTypeArray32 {
		int	nCatchableTypes;
		int arrayOfCatchableTypes[]; // 32-bit VA of CatchableType64 instances 
		} CatchableTypeArray32;
	#pragma warning (pop)

	//
	// ThrowInfo - information describing the thrown object, statically built
	// at the throw site.
	//
	// pExceptionObject (the dynamic part of the throw; see below) is always a
	// reference, whether or not it is logically one.  If 'isSimpleType' is true,
	// it is a reference to the simple type, which is 'size' bytes long.  If
	// 'isReference' and 'isSimpleType' are both false, then it's a UDT or
	// a pointer to any type (i.e. pExceptionObject points to a pointer).  If it's
	// a pointer, copyFunction is NULL, otherwise it is a pointer to a copy
	// constructor or copy constructor closure.
	//
	// The pForwardCompat function pointer is intended to be filled in by future
	// versions, so that if say a DLL built with a newer version (say C10) throws,
	// and a C9 frame attempts a catch, the frame handler attempting the catch (C9)
	// can let the version that knows all the latest stuff do the work.
	//
	typedef struct _s_ThrowInfo64 {
		unsigned int	attributes;							// Throw Info attributes (Bit field)
		PMFN64			pmfnUnwind;							// Destructor to call when exception has been handled or aborted
		int				pForwardCompat;						// Image relative offset of Forward compatibility frame handler
		int				pCatchableTypeArray;				// Image relative offset of CatchableTypeArray
	} ThrowInfo64;

	typedef struct _s_ThrowInfo32 {
		unsigned int	attributes;							// Throw Info attributes (Bit field)
		PMFN32			pmfnUnwind;							// Destructor to call when exception has been handled or aborted
		int				pForwardCompat;						// 32-bit VA of Forward compatibility frame handler
		int				pCatchableTypeArray;				// 32-bit VA of CatchableTypeArray
	} ThrowInfo32;

	/////////////////////////////////////////////////////////////////////////////
	//
	// The NT Exception record that we use to pass information from the throw to
	// the possible catches.
	//
	// The constants in the comments are the values we expect.
	// This is based on the definition of EXCEPTION_RECORD in winnt.h.
	//
	#pragma pack(push, EHExceptionRecord64, 8)
	struct EHParameters64 {
		unsigned long magicNumber;      // = EH_MAGIC_NUMBER1
		void *        pExceptionObject; // Pointer to the actual object thrown
		ThrowInfo64*  pThrowInfo;       // Description of thrown object
		void*         pThrowImageBase;  // Image base of thrown object

		template <typename _TPointer, typename _TOffset, 
			std::enable_if_t<
				std::conjunction<
					std::is_pointer<_TPointer>, 
					std::is_integral<_TOffset>
				>::value, 
			int> = 0>
		_TPointer rva_to_va(_TOffset offset) const {
			return reinterpret_cast<_TPointer>(reinterpret_cast<uintptr_t>(pThrowImageBase) + static_cast<uintptr_t>(offset));
		}
	};
	#pragma pack(pop, EHExceptionRecord64)

	struct EHParameters32 {
		unsigned long magicNumber;      // = EH_MAGIC_NUMBER1
		void *        pExceptionObject;	// Pointer to the actual object thrown
		ThrowInfo32*  pThrowInfo;       // Description of thrown object
	};

	#pragma pack(pop, exception_handling_rtti)

	/// <summary>
	/// The ExceptionRunTimeTypeInformation contains type name chains for a VC++ EH Exception. It might 
	/// also contain the full module path of the module that constructed the ThrowInfo instance and the 
	/// exception message if it was a catchable based on std::exception.
	/// </summary>
	class ExceptionRunTimeTypeInformation {
		private:
			/// <summary>
			/// Demangled name of std::exception to look for.
			/// </summary>
			static constexpr const char std_exception[] = "std::exception";

			std::shared_ptr<Hindsight::Process::Process> m_Process       = nullptr; /* process instance, optional (for binary playback) */
			const EXCEPTION_RECORD*                      m_Record        = nullptr; /* exception record, optional (for binary playback) */
			const Hindsight::Debugger::ModuleCollection* m_LoadedModules = nullptr; /* loaded binaries,  optional (for binary playback) */

			mutable std::vector<std::string>    m_ExceptionTypeNames;               /* a list of catchable type names, demangled */
			mutable std::optional<std::string>  m_ExceptionMessage;                 /* an optional exception message, if it could be determined */
			mutable std::optional<std::wstring> m_ThrowImagePath;                   /* the full module path to the module that constructed the ThrowInfo */

			/// <summary>
			/// Process the RTTI as a 64-bit structure using RVA (relative to module base).
			/// </summary>
			void Process64() const noexcept;

			/// <summary>
			/// Process the RTTI as a 32-bit structure using VA (non-relative, 32-bit address space).
			/// </summary>
			void Process32() const noexcept;

		public:
			/// <summary>
			/// Construct a new ExceptionRunTimeTypeInformation from a currently debugged process, an exception record and a module collection.
			/// </summary>
			/// <param name="process">The currently running (and suspended) process that is being debugged and is currently throwing an exception.</param>
			/// <param name="record">The exception record that was obtained at the exact state <paramref name="process"/> is suspended in.</param>
			/// <param name="loadedModules">The collection of loaded modules at the time of the exception.</param>
			ExceptionRunTimeTypeInformation(std::shared_ptr<Hindsight::Process::Process> process, const EXCEPTION_RECORD& record, const Hindsight::Debugger::ModuleCollection& loadedModules);

			/// <summary>
			/// Construct a new ExceptionRunTimeTypeInformation from previously known RTTI (i.e. from a binary log file).
			/// </summary>
			/// <param name="exceptionTypeNames">The collection of demangled exception type names.</param>
			/// <param name="message">The optional exception message.</param>
			/// <param name="path">The optional module path to the module that constructed the ThrowInfo instance.</param>
			ExceptionRunTimeTypeInformation(const std::vector<std::string>& exceptionTypeNames, const std::optional<std::string>& message, const std::optional<std::wstring>& path);

			/// <summary>
			/// Fetch the collection of catchable type names which comprise this exception.
			/// </summary>
			/// <returns>A vector of strings with demangled type names.</returns>
			const std::vector<std::string>& exception_type_names() const noexcept;

			/// <summary>
			/// Fetch the optional exception message.
			/// </summary>
			/// <returns>The optional exception message.</returns>
			const std::optional<std::string>& exception_message() const noexcept;

			/// <summary>
			/// Fetch the optional full module path to the module that constructed the ThrowInfo instance.
			/// </summary>
			/// <returns>The optional module path.</returns>
			const std::optional<std::wstring>& exception_module_path() const noexcept;
	};

}

#endif 