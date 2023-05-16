#pragma once
#include <optional>
#include <variant>
#include <vector>
#include "Misc.hpp"
#include "Argument.hpp"
#include "Instance.hpp"

using namespace std;

/**
 * Represents a heuristics to be used by the solver
 */
class Heuristics
{
	public: 
		/**
		 * The heuristics supported by the solver
		 */
		enum class HeuristicTypes {None, MaxOutDegree, MinInDegree, PathLength, PathLengthModified };		

		/**
		* @{return A object of Heuristics that represents the provided string or none if the string could not be parsed}
		*/
		static optional<Heuristics> tryParse(char const* const s);

	private:
		/**
		 * The type of heuristics to be used
		 */
		HeuristicTypes type;

		/**
		 * The first heuristics parameter
		 */
		variant<unsigned short> heuristicsParameter1;			

	public:

		/**
		 * Creates a new instance of the Heuristics class with the provided type
		 */
		Heuristics(HeuristicTypes const& type);

		Heuristics(const Heuristics& other) = default;
		Heuristics(Heuristics&& other) = default;
		Heuristics& operator=(const Heuristics& other) = default;
		Heuristics& operator=(Heuristics&& other) = default;

		/**
		 * Creates a new instance of the Heuristics class with type None
		 */
		Heuristics();

		/**
		 * Applies the heuristics to a given instance
		 * @return returns a vector of all arguments that have not been assigned yet at or below the given DL in the order they should be guessed and a vector indicating the first guess that should be made
		 */
		pair<vector<Argument*>, vector<Sign>> apply(Instance& instance, DL const& dl) const;
};