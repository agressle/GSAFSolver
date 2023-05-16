#pragma once

#include <vector>
#include <deque>
#include <cassert>
#include <memory>
#include <fstream>

#include "Misc.hpp"
#include "Argument.hpp"
#include "Clause.hpp"


using namespace std;

/**
 * Represents a SETAF instance
 */
class Instance
{							
	private:

		/**
		* The arguments of the instance
		*/
		vector<Argument> arguments;

		/**
		 * The required arguments with signs
		 */
		vector<pair<Argument*, Sign>> requiredArguments;

		/**
		 * The attacks of the instance
		 */
		vector<Clause> attacks;

		/**
		 * All added clauses
		 * Not a vector as the reallocations would cause the references in arguments and clauses to go invalid
		 */
		deque<Clause> addedClauses;

		/**
		 * All learned clauses
		 */
		deque<Clause*> learnedClauses;

		/**
		 * /All clauses that have been marked as forgotten but are still used somewhere
		 */
		vector<Clause*> forgottenClauses;

		/**
		 * All clauses that have been forgotten and can be used again
		 */
		vector<Clause*> availableClauses;

		/**
		 * The next clause id to use
		 */
		ID nextClauseID;		

		/**
		 * {@return a new clause, either recycle a used clause that has been forgotten one or a new one with a given initial capacity}
		 */
		Clause& getNewClause(size_t const& capacity, Clause::ClauseType const& clauseType);

		/**
		 * Write the clause to the proof file.
		 */		
		void writeProofClause(Clause* clause, std::unique_ptr<ofstream>& proofFile)
		{
			*proofFile << "d "; 

			for (auto [beginMember, endMember] = clause->getMembersIterator(); beginMember != endMember; beginMember++)
			{
				auto& [argument, sign] = *beginMember;
				if (sign == -1)
					*proofFile << "-";
				*proofFile << argument->getName();
				*proofFile << " ";
			}
			*proofFile << "0\n";
		}

	public:
		
		/**
		 * Create a new instance object
		 */
		Instance(ID const& numArguments, ID const& numAttacks);

		Instance(const Instance& other) = default;
		Instance(Instance&& other) = default;
		Instance& operator=(const Instance& other) = default;
		Instance& operator=(Instance&& other) = default;
		
		/**
		 * {@return the attack with the given id}
		 */
		Clause& getAttack(ID const& id);

		/**
		 * {@return the argument with the given id}
		 */
		Argument& getArgument(ID const& id);

		/**
		 * {@return the number of arguments}
		 */
		size_t getNumberOfArguments() const;

		/**
		 * Adds the given value to the list of required arguments
		 */
		void addRequiredArgument(Argument& argument, Sign const& sign);

		/**
		 * Prints a representation of this instance to cout. Used for debugging.
		 */
		void print();

		/**
		 * {@return a copy of the vector of all arguments}
		 */
		vector<Argument*> getArgumentsCopy();

		/**
		 * {@return a pair consisting of the begin and end iterator of the attack clause}
		 */
		pair<vector<Clause>::iterator, vector<Clause>::iterator> getAttackIterator();		

		/**
		 * {@return a pair consisting of the begin and and iterator of the arguments}
		 */
		pair<vector<Argument>::iterator, vector<Argument>::iterator> getArgumentsIterator();

		/**
		 * {@return a pair consisting of the begin and and iterator of the required arguments}
		 */
		pair<vector<pair<Argument*, Sign>>::iterator, vector<pair<Argument*, Sign>>::iterator> getRequiredArgumentsIterator();

		/**
		 * {@return the number of attacks}
		 */
		size_t getNumberOfAttacks() const;
		
		/**
		 * {@return a new clause flagged as output clause, either recycle a used clause that has been forgotten one or a new one with a given initial capacity}
		 */
		//Clause& getNewOutputClause(size_t const& capacity);
		

		/**
		 * {@return a new clause flagged as leanred clause, either recycle a used clause that has been forgotten one or a new one with a given initial capacity}
		 */
		Clause& getNewLearnedClause(const size_t& capacity);

		/**
		 * Removes the given clause from the list of forgot clauses and adds it to the list of available clauses, if the clause is marked as forgotten
		 */
		void recycleClause(Clause& clause);

		/**
		 * Gets the number of learned clauses that have not yet been forgotten
		 */
		size_t getNumberOfLearnedClauses() const;

		/**
		 * Marks the given number of clauses as forgotten
		 */		
		void forgetClauses(size_t amountOfClausesToForget, std::unique_ptr<ofstream>& proofFile, bool generateProof);
};