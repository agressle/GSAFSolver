#pragma once

#include <vector>
#include <unordered_set>
#include <utility>
#include <chrono>

#include "./datamodel/Misc.hpp"
#include "./datamodel/Instance.hpp"
#include "./datamodel/Argument.hpp"
#include "./datamodel/Clause.hpp"
#include "../header/datamodel/Heuristics.hpp"
#include "../header/datamodel/Semantics.hpp"

using namespace std;

class Solver
{
	private:
		/**
		 * The instance to solver
		 */
		Instance& instance;

		/**
		 * The semantics to use
		 */
		Semantics const& semantics;

		/**
		 * The heuristics to use
		 */
		Heuristics const& heuristics;

		/**
		 * The time at which time first model has been found
		 */
		chrono::time_point<chrono::high_resolution_clock>& firstModelTime;

		/**
		 * The number of models found
		 */
		unsigned long& modelCount;

		/**
		 * The percentage of the search space that has been checked
		 */
		double& percentageSolved;

		/**
		 * The number of models to find or 0 for unlimited
		 */
		unsigned long& numberOfModels;

		/**
		 * Indicates whether models should be printed to stdout
		 */
		bool const& printModels;

		/**
		 * The number of learned clauses that, when reached, causes a new forget cycle
		 */
		double learnedClausesToForgetThreshold;

		/**
		 * The number of learned clauses that, when reached, causes a new forget cycle
		 */
		double const& clForgetPercentage;

		/**
		 * The factor that the forget threshold increases per forget cycle
		 */
		double const& clGrowthRate;

		/**
		 * The current decision level
		 */
		DL currentDl = 0;

		/**
		 * The decision level to which a back jump can occur at most to ensure that no solutions are repeated
		 */
		DL backjumpingBound = 0;

		/**
		 * The next index to guess at
		 */
		ID nextGuessPosition = 0;

		/**
		 * Holds the assigned arguments
		 */
		vector<Argument*> assingedArguments;

		/**
		 * A helper hash set that can be used for keeping track of IDs. Member to avoid reallocation.
		 */
		unordered_set<ID> helperHashsetID;

		/**
		 * A helper vector that can be used to keep track of arguments with associated signs. Member to avoid reallocation.
		 */
		vector<pair<Argument*, Sign> const*> helperVectorArgumentSign;		

		/**
		 * The file to which the proof is written to.
		 */
		std::unique_ptr<ofstream>& proofFile;

	public:
		/**
		 * Create a new solver with the given instance
		 */
		Solver(Instance& instance, Semantics const& semantics, Heuristics const& heuristics, chrono::time_point<chrono::high_resolution_clock>& firstModelTime, unsigned long& modelCount, double& percentageSolved, unsigned long& numberOfModels, bool const& printModels, double const& clForgetPercentage, double const& clGrowthRate, std::unique_ptr<ofstream>& proofFile);
		Solver(const Solver& other) = default;
		Solver(Solver&& other) = default;
		Solver& operator=(const Solver& other) = default;
		Solver& operator=(Solver&& other) = default;

		
		/**
		 * Starts with solving process
		 */		
		void solve();	

	private:

		/**
		 * Prints the current assignment if printModels is true and increments the model counter
		 *
		 * @return True if the required number of models have been found
		 */
		bool printAssignment();

		/**
		* Checks whether the threshold has been reached and if yes, marks the the appropriate number of clauses as forgotten
		*/
		void checkAndForgetClauses();

		/**
		 * Write the clause to the proof file.
		 */		
		void writeProofClause(Clause& clause, bool isImplicitClause);

		/**
		 * Builds the implicit clause that represents the conflict that occurred when assigning the given argument
		 */
		Clause& buildImplictClause(Argument& arg);

		/**
		 * Recomputes the watched attack for an argument.
		 * Every argument assigned -1 needs an attack of the original instance that attacks it. Here, we try to find such a clause and set the stability watches.
		 */
		Clause* recomputeWatchedAttack(Argument& arg, size_t* const forAttackIndex);

		/**
		 * Checks a given clause by updating watches
		 * @param wasRemoved will be set to true if the clause was removed from an arguments watchedIn vector
		 * @return a clause if this clause was asserting and thee subsequent call to setAndPropagate returned a clause and nullptr otherwise
		 */
		Clause* checkClause(Clause& clause, Argument* argument, bool* wasRemoved);

		/**
		 * Sets the given value for the given argument
		 * @return A clause representing the conflict if one occurred as consequence of the assignment
		 */
		Clause* setAndPropagate(Argument& argument, Sign value, Clause* reason);

		/**
		 * Resolves a conflicting clause by generating an asserting resolvent clause and updates the current DL to backtrack to
		 * @param uipArgument Will be set to the the UIP argument if we backtrack to it
		 * @return a resolvent clause or nullptr, if no further backtracking is possible and the solver is done
		 */
		Clause* resolveConflictAndUpdateDL(Clause& conflictingClause, Argument** uipArgument);

		/**
		 * Backtracks based on a given clause		 
		 * @return false if no further backtracking is possible and the solver is finished
		 */
		bool backtrackForClause(Clause& conflictingClause);

		/**
		 * Undoes all assignments done at decision level higher than the given one
		 * @return the position of the next argument to guess after backtracking
		 */
		Argument* backtrackToCurrentDL(Sign& oldSign);

		/**
		 * Computes the grounded extension
		 * @return false if an assignment caused a conflict
		 */
		bool computeGrounded();

		/**
		 * Does an assignment and handles backtracking
		 */
		bool doAssignment(Argument& argument, Sign sign, Clause* reason);

		/**
		 * Calculates the stable extensions
		 * @return the percentage of the search space that has been exhausted
		 */
		double calculate_stable();

		/**
		 * Calculates how much of the search space has been exhausted
		 * @return the percentage of the search space that has been exhausted
		 */
		double calculatePercentageSolved(vector<Argument*>& sortedArguments, vector<Sign>& guessOrder);
};