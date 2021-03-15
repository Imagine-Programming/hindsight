#pragma once

/*
The versioning scheme used in this project is quite basic and does not follow some international standard,
but is used to keep track of what changes happend and reflect that in the version number. The major and minor
version numbers are used for major and minor functional changes or improvements. The revision is exclusively 
used for changes in the core code that fix bugs and undefined or unexpected behaviours and the build number is 
exclusively used when dependencies change.

All elements in the version number are unsigned 8-bit values, hence why the revision and build will be reset to 
1 on each minor version update. 

A major version 0 indicates a pre-release version.

	major version:	only increments with major changes, such as a full new subcommand or feature overhaul;
	minor version:	only increments with minor changes, such as a change in internal behaviour or improvement of functionality;
					relative to: major version
	revision:		only increments on bugfixes/corrections due to undefined or unexpected behaviour;
					relative to: minor version
	build:			only increments when a dependency is updated, such as linking with distorm or DbgHelp.
					relative to: minor version
	appendix:		indicate alpha, beta, work versions.

examples:
	0.4.1.1alpha	pre-release version 0.4, revision 1, build 1 - alpha
	1.0.0.0			released version 1.0, revision 0, build 0 (base release version)
	1.0.1.0			released version 1.0, revision 1 (bugfix), build 0 (no deps change)
	1.1.0.1beta		beta version 1.1, revision 0, build 1 (deps changed) - beta
*/

#ifndef version_h
#define version_h
	#include <string>
	#include <vector>

	#define __wchar_unfold(s) L ## s
	#define __wchar_literal(s) __wchar_unfold(s)
	#define __str_unfold(s) #s
	#define __str_convert(s) __str_unfold(s)

	#define hindsight_version_major			0
	#define hindsight_version_minor			6
	#define hindsight_version_revision		2
	#define hindsight_version_build			0
	#define hindsight_version_year_s		"2021"
	#define hindsight_version_appendix		"alpha"
	#define hindsight_author				"Bas Groothedde"

	#define hindsight_version_major_s		__str_convert(hindsight_version_major)
	#define hindsight_version_minor_s		__str_convert(hindsight_version_minor)
	#define hindsight_version_revision_s	__str_convert(hindsight_version_revision)
	#define hindsight_version_build_s		__str_convert(hindsight_version_build)
	#define hindsight_version				hindsight_version_major_s "." hindsight_version_minor_s hindsight_version_appendix
	#define hindsight_version_full			hindsight_version_major_s "." hindsight_version_minor_s "." hindsight_version_revision_s "." hindsight_version_build_s hindsight_version_appendix
	#define hindsight_version_int			((hindsight_version_major << 24) | (hindsight_version_minor << 16) | (hindsight_version_revision << 8) | (hindsight_version_build))

	namespace Hindsight {
		/// <summary>
		/// In the case that someone wants to contribute and requests a pull, that contributor should add their 
		/// name to this vector if they want to be included in the list of contributors in the form L"Display Name".
		/// </summary>
		static const std::vector<std::wstring> _contributors = {
			__wchar_literal(hindsight_author),
			L"Lisa Marie"
		};
	}
#endif 