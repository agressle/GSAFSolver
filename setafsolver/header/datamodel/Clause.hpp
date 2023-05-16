#pragma once

#include <variant>
#include <vector>
#include <string>
#include "Misc.hpp"
using namespace std;

class Argument;

/**
 * Represents a Clause
 */
class Clause
{
	public:
		/**
		* The types of clauses
		*/
		enum class ClauseType { Attack, SelfAttack, Learned, /*Output,*/ Forgotten };

	private:
		/**
		 * The ID of the clause
		 */
		ID id = 0;
		/**
		 * The index in the members vector of the argument that is the first watched argument
		 */
		size_t firstWatch = 0;
		/**
		 * The index in the members vector of the argument that is the second watched argument
		 */
		size_t secondWatch = 0;
		/**
		 * The members of this clause
		 */
		vector<pair<Argument*, Sign>> members;
		/**
		 * The type of the clause
		 */
		ClauseType clauseType;
		/**
		 * The usage counter of the clause. Only relevant for learned and forgotten clauses
		 */
		ID usageCounter = 0;
		/**
		 * The index in the instance's forgotten clauses vector. Only relevant for forgotten clauses
		 */
		size_t indexInForgottenClausesVector = 0;

	public:

		/**
		 * Creates a new clause with a given id, initial capacity and type
		 */
		Clause(ID const& id, size_t const& capacity, ClauseType const& clauseType);

		/**
		 * Creates a new clause with a given id and and type with initial capacity 0
		 */
		Clause(ID const& id, ClauseType const& clauseType);		

		Clause(const Clause& other) = default;
		Clause(Clause&& other) = default;
		Clause& operator=(const Clause & other) = default;
		Clause& operator=(Clause&& other) = default;

		/**
		 * Resets the clause by setting the watches to 0, setting the clause type and clearing the members so that it is in a known state and can be reused
		 */
		void reset(ClauseType const& clauseType);

		/**
		 * Allocates a given number of members.
		 */
		void reserveMemberSize(size_t const& size);

		/**
		 * Sets the id of the clause
		 */
		void setId(ID const& id);

		/**
		 * {@return Returns the id of this clause}
		 */
		ID getId() const;

		/**
		 * Sets the attacked argument of the clause and adds it to the attackers of the argument. Updates the watches if appropriate
		 * Only relevant for the original attacks of the instance, not for learned clauses
		 */
		void setAttacked(Argument& argument, Sign const& sign);

		/**
		 * {@return Returns the attacked argument. Only allowed for attacks or self attacks}
		 */
		Argument& getAttackedArgument() const;

		/**
		 * Adds an argument to the members of this attack and updates the watches if appropriate
		 */
		void addArgument(Argument& argument, Sign const& sign);

		/**
		 * {@return Returns a pair of the start and end iterators over the members of this clause}
		 */
		pair<vector<pair<Argument*, Sign>>::iterator, vector<pair<Argument*, Sign>>::iterator> getMembersIterator();

		/**
		 * {@return Returns the member clause at a given index}
		 */
		pair<Argument*, Sign> const* get_member_element_at(size_t const& index) const;

		/**
		 * {@return Returns the count of members of this attack}
		 */
		size_t getMemberCount() const;

		/**
		 * Increments the use counter of this clause by 1
		 */
		void incrementUseCounter();

		/**
		 * Decrements the use counter of this clause by 1
		 *
		 * @return True iff the usage counter is 0 after decrement
		 */
		bool decrementUseCounter();

		/**
		 * {@return True iff the clause is not used, i.e. the usage counter == 0}
		 */
		bool isNotUsed();

		/**
		 * /Marks the clause as an attack that contains the attacked argument as member
		 */
		void markAsSelfAttack();

		/**
		 * Marks the clause as learned clause, setting the usage counter to 0
		 */
		void markAsLearned();

		/**
		 * Marks the clause as output clause
		 */
		void markAsOutput();

		/**
		 * Marks the clause as forgotten with the given index. Only allowed for clauses that are flagged as learned		 
		 */
		void markAsForgotten(size_t const& index);

		/**
		 * Updates the forgotten list index of this clause. Only allowed for clauses that are marked as forgotten
		 */
		void setForgottenIndex(size_t const& index);

		/**
		 * {@return Returns the forgotten index of the clause. Only valid for clauses that are marked as forgotten}
		 */
		size_t getForgottenIndex() const;

		/**
		 * {@return Returns true iff this clause is an attack of the original instance that contained a self attack}
		 */
		bool isSelfAttack() const;

		/**
		 * {@return Returns true iff this clause is marked as forgotten}
		 */
		bool isForgotten() const;

		/**
		 * {@return Returns true iff this clause is marked as attack}
		 */
		bool isAttack() const;

		/**
		 * {@return Returns true if the attack is not blocked, i.e. every attacking argument is either in or not assigned}
		 */
		bool isNotBlocked(DL const& dl) const;

		/**
		 * {@return Returns the watches of this clause}
		 */
		pair<size_t, size_t> getWatches() const;

		/**
		 * {@return The index of the first watched argument }
		 */
		size_t getFirstWatch();

		/**
		 * {@return The index of the second watched argument }
		 */
		size_t getSecondWatch();

		/**
		 * {@return Sets the provided watch of this clause. Will do nothing if the watch did not change. Returns true if the argument has been moved in its watchedIn vector}
		 */
		bool setWatch(bool const& isFirst, size_t const& index);

		/**
		 * Checks if the watched are valid, assuming that at least one argument does not have value 0
		 */
		bool watchesAreInvalidArgSet(DL const& dl) const;

		/**
		 * Prints the trace of the current clause to stdout (for debug proposes)
		 */
		void printTrace();

		/**
		 * Returns a string representation of this clause
		 */
		string toString();
};

