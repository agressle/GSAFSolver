#pragma once
#include <optional>

using namespace std;

/**
 * Represents a semantics
 */
class Semantics
{
	public:
		/**
		 * The semantics supported by the solver
		 */
		enum SemanticsType { stable };

		/**
		 * @{return A object of Semantics that represents the provided string if none if the string could not be parsed}		 
		 */
		static optional<Semantics> tryParse(char* const& s);

	private:
		/**
		 * The type of semantics to be used
		 */
		SemanticsType type;
	
	public:	
		/**
		 * Create a new instance of Semantics with the provided type
		 */
		Semantics(SemanticsType const& type);

		/**
		 * Create a new instance of Semantics with the type stable
		 */
		Semantics();

		Semantics(const Semantics& other) = default;
		Semantics(Semantics&& other) = default;
		Semantics& operator=(const Semantics& other) = default;
		Semantics& operator=(Semantics&& other) = default;

		/**
		 * Returns the type of the semantics
		 */
		SemanticsType const& getType() const;
};