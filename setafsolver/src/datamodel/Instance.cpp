#include "../../header/datamodel/Instance.hpp"
#include "../../header/tools/Helper.hpp"
#include <cassert>
#include <iostream>
#include <fmt/core.h>
#include <algorithm>

using namespace std;

Instance::Instance(ID const& numArguments, ID const& numAttacks)
{	
	arguments.reserve(numArguments);
	attacks.reserve(numAttacks);

	for (ID i = 0; i < numArguments; i++)
		arguments.emplace_back(i);

	for (ID i = 0; i < numAttacks; i++)
		attacks.emplace_back(i, Clause::ClauseType::Attack);	

	nextClauseID = attacks.size();
}

Clause& Instance::getAttack(ID const& id)
{
	assert(id < this->attacks.size());
	return attacks[id];
}

Argument& Instance::getArgument(ID const& id)
{
	assert(id < this->arguments.size());
	return arguments[id];
}

size_t Instance::getNumberOfArguments() const
{
	return arguments.size();
}

void Instance::addRequiredArgument(Argument& argument, Sign const& sign)
{
	requiredArguments.push_back(pair(&argument, sign));
}

void Instance::print()
{
	cout << fmt::format("Arguments: ({}):", arguments.size()) << endl;
	for (auto& argument: arguments)
		cout << argument.toString() << endl;
	cout << endl;
	cout << "-------------------------" << endl;
	cout << endl;
	cout << fmt::format("Attacks: ({}):", attacks.size()) << endl;
	for (auto& attack : attacks)
		cout << attack.toString() << endl;
	cout << endl;
	cout << "-------------------------" << endl;
	cout << endl;
	cout << fmt::format("Required: ({}):", requiredArguments.size()) << endl;
	for (auto& [argument, sign] : requiredArguments)
	{
		if (sign == -1) cout << "-";
		cout << argument->toString() << endl;
	}
}

vector<Argument*> Instance::getArgumentsCopy()
{		
	vector<Argument*> target;
	target.reserve(arguments.size());	
	for (auto& arg : arguments)		
		target.emplace_back(&arg);
	return target;
}

pair<vector<Clause>::iterator, vector<Clause>::iterator> Instance::getAttackIterator()
{	
	return pair(attacks.begin(), attacks.end());
}

pair<vector<Argument>::iterator, vector<Argument>::iterator> Instance::getArgumentsIterator()
{
	return pair(arguments.begin(), arguments.end());
}

pair<vector<pair<Argument*, Sign>>::iterator, vector<pair<Argument*, Sign>>::iterator> Instance::getRequiredArgumentsIterator()
{
	return pair(requiredArguments.begin(), requiredArguments.end());
}

size_t Instance::getNumberOfAttacks() const
{
	return attacks.size();
}

Clause& Instance::getNewClause(size_t const& capacity, Clause::ClauseType const& clauseType)
{	
	if (availableClauses.empty())
	{
		//No recycled clauses available -> need to get a new one
		return addedClauses.emplace_back(nextClauseID++, capacity, clauseType);
	}
	else
	{
		//We can recycle a clause
		auto clause = availableClauses.back();
		availableClauses.pop_back();
		clause->reset(clauseType);
		clause->setId(nextClauseID++);		
		clause->reserveMemberSize(capacity);		
		return *clause;
	}		
}

/*
Clause& Instance::getNewOutputClause(size_t const& capacity)
{
	auto& clause = getNewClause(capacity, Clause::ClauseType::Output);	
	return clause;
}
*/

Clause& Instance::getNewLearnedClause(const size_t& capacity)
{
	auto& clause = getNewClause(capacity, Clause::ClauseType::Learned);	
	learnedClauses.push_back(&clause);
	return clause;
}

void Instance::recycleClause(Clause& clause)
{
	if (clause.isForgotten()) //Only clauses that are marked as forgotten can be recycled
	{
		assert(!forgottenClauses.empty());
		assert(forgottenClauses.size() > clause.getForgottenIndex());

		//Remove the clause from the forgotten clauses list
		if (clause.getForgottenIndex() == forgottenClauses.size() - 1)
			forgottenClauses.pop_back(); //The current clause is the last one in the list -> thus just remove
		else
		{
			//The current clause is not the last one in the list -> swap remove and update index of the other clause
			Helper::swapRemove(forgottenClauses, clause.getForgottenIndex());
			forgottenClauses[clause.getForgottenIndex()]->setForgottenIndex(clause.getForgottenIndex());
		}
		availableClauses.push_back(&clause);
	}
}

size_t Instance::getNumberOfLearnedClauses() const
{
	return learnedClauses.size();
}

void Instance::forgetClauses(size_t amountOfClausesToForget, std::unique_ptr<ofstream>& proofFile, bool generateProof)
{
	assert(amountOfClausesToForget <= learnedClauses.size());
	while (amountOfClausesToForget-- > 0)
	{
		Clause* clause = learnedClauses.front();
		if (generateProof)
			writeProofClause(clause, proofFile);

		if (clause->isNotUsed())
			availableClauses.push_back(clause); //Clause is not used anywhere and thus can be made available again
		else
		{
			//Clause is still used somewhere and thus we cant make it available again yet
			clause->markAsForgotten(forgottenClauses.size());
			forgottenClauses.push_back(clause);
		}

		learnedClauses.pop_front();
	}
}