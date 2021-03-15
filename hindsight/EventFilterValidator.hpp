#pragma once

#ifndef event_filter_validator_h
#define event_filter_validator_h
	#include "CLI11.hpp"

	#include <set>
	#include <vector>
	#include <string>

	namespace Hindsight {
		namespace Cli {
			namespace CliValidator {
				/// <summary>
				/// A simple validator for <see cref="::CLI::App"/> that validates the choice of filters 
				/// when event filtering must be applied in one of the hindsight output modes. This struct
				/// can be used to check if the user chose a valid filter name.
				/// </summary>
				struct EventFilterValidator : public CLI::Validator {
					/// <summary>
					/// A static collection of valid event names.
					/// </summary>
					static std::set<std::string> Valid;

					/// <summary>
					/// The static instance of the validator, for convenience.
					/// </summary>
					static EventFilterValidator Validator;

					/// <summary>
					/// Construct a new EventFilterValidator
					/// </summary>
					EventFilterValidator();

					/// <summary>
					/// Get a comma-separated list of valid options for this validator.
					/// </summary>
					/// <returns>A comma-separated list of event names.</returns>
					static std::string GetValid();
				};
			}
		}
	}

#endif 