#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <limits>

#include "Misc.hpp"

using namespace std;

class Clause;
class Instance;

/**
 * Represents an Argument
 */
class Argument
{
	private:
		///The ID of the argument. The same that is used in the input file
		ID id;

		///The position of this argument in the list of arguments of the accompanying instance
		ID position;

		///The name of the argument. Either the number or the name
		string name;

		///The decision level that which this argument has been assigned a value
		DL dl = std::numeric_limits<ID>::max();

		///The value that this argument has been assigned, either 0 for unassigned, -1 for out or 1 for in
		Sign value = 0;

		///The reason that this argument has been assigned the current value. Either NULL if it is unassigned or is a guess and the asserting Clause otherwise
		Clause* reason = nullptr;

		///A value used to compute heuristics. Will be initialized to the number of attacks this argument occurs in by the parser but can be changed later
		double heuristicsValue = 0.0;

		///The attacks of the original instance that attack this argument
		vector<Clause*> attackedBy;

		///The Clauses, either original attacks of the instance or learned, that this argument is watched in
		vector<Clause*> watchedIn;

		///A hash map to map from Clause id to the index of that clause in watchedIn
		unordered_map<ID, size_t> watchedInClauseIndex;	

		///The index of the attack in attackedBy that is used as witness for the stability of this argument if its value set to -1
		size_t watchedInAttackIndex = 0;

		///A list of all arguments that use this argument as witness for stability. The second pair element is the index of the attack for which this argument is one of the witnesses
		vector<pair<Argument*, size_t>> stabilityWatch;

	public: 		

		/**
		 * Creates a new argument with given id, position and name
		 */
		Argument(ID const& id, ID const& position, string const& name);

		/**
		 * Creates a new argument with given id and position and name equal the id
		 */
		Argument(ID const& id, ID const& position);

		/**
		 * Creates a new argument with given id. The position and name equal the id
		 */
		Argument(ID const& id);

		Argument(const Argument& other) = default;
		Argument(Argument&& other) = default;
		Argument& operator=(const Argument& other) = default;
		Argument& operator=(Argument&& other) = default;

		/**
		 * Sets the name of this argument
		 */
		void setName(string const& name);

		/**
		 * Gets the name of the argument, either the number as string or the name if supplied
		 */
		string getName() const;

		/**
		 * Sets the id and position of this argument
		 */
		void setIdAndPosition(ID const& id);

		/**
		 * Gets the id of this argument
		 */
		ID getId() const;

		/**
		 * Gets the decision level at which this argument has been assigned or -1 if no assignment has occurred yet
		 */
		DL getDl() const;

		/**
		 * Sets the position of this argument
		 */
		void setPosition(ID const& position);

		/**
		 * Gets the position of this argument
		 */
		ID getPosition() const;

		/**
		 * Sets the value of this argument
		 */
		void setValue(Sign const& value, DL const& dl, Clause* reason, Instance &instance);

		/**
		 * Gets the value of this argument without checking for decision level
		 */
		Sign getValueFast() const;

		/**
		 * Gets the value that this argument has been assigned or 0 if no assignment has occurred yet, taking the decision level into account
		 */
		Sign getValue(DL const& dl) const;

		/**
		 * Add the given clause to the list of clauses that watch this argument
		 * Increments the usage counter of the clause
		 */
		void addWatchedIn(Clause& clause);

		/**
		 * Removes the given clause from the list of clauses that watch this argument
		 * Returns true if the usage counter of the clause hit 0 after removal
		 */
		bool removeWatchedIn(Clause& clause);

		/**
		 * Sets the heuristics value of this argument to the given value
		 */
		void setHeuristicsValue(double const& value);

		/**
		 * Gets the heuristics value of this argument
		 */
		double getHeuristicsValue() const;

		/**
		 * Add the given clause to the list of clauses attacking this argument
		 */
		void addAttackedBy(Clause& clause);

		/**
		 * Gets the attacking clause with a given index
		 */
		Clause& getAttackingClause(size_t const& index) const;

		/**
		 * Gets the number of attacks of the original instance attacking this argument
		 */
		size_t getAttackedByCount() const;

		/**
		 * Gets an a pair of the cbegin and cstart iterators over the attacking clauses of this argument
		 */
		pair<vector<Clause*>::const_iterator, vector<Clause*>::const_iterator> getAttackedByIterator() const;

		/**
		 * Sets the watched attack index to the given value and adds this argument to the watched argument of all the attackers of this attack
		 * @param clause The clause in which this argument is watched in
		 * @param index The index of clause in the attackedBy vector
		 */
		void setWatchedAttackedIndex(Clause& clause, size_t const& index);

		/**
		 * Gets the watched attack index
		 */
		size_t getWatchedAttackIndex() const;

		/**
		 * Gets the number of clauses in the watched in vector
		 */
		size_t getWatchedInCount() const;

		/**
		 * Gets the clause at the given index in the watched_in vector
		 */
		Clause& getWatchedInElementAt(size_t const& index) const;

		/**
		 * {@return True if the stability watch of this argument is empty}
		 */
		bool stabilityWatchIsEmpty() const;

		/**
		 * Removes the last element of the stability watch
		 */
		pair<Argument&, size_t> stabilityWatchPop();

		/**
		 * Add the given argument and index to the stability watch of this argument
		 */
		void stabilityWatchPush(Argument& argument, size_t const& index);

		/**
		 * Return the reason this argument has been assigned the assigned value
		 */
		Clause* getReason();

		/**
		 * Resets the dl and value of this argument
		 */
		void reset();

		/**
		 * Returns a string representation of this argument
		 */
		string toString();
};