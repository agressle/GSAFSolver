#include "../../header/datamodel/Clause.hpp"
#include "../../header/datamodel/Argument.hpp"
#include <fmt/core.h>
#include <cassert>
#include <iostream>

using namespace std;

Clause::Clause(ID const& id, size_t const& capacity, ClauseType const& clauseType) : id(id), clauseType(clauseType) { members.reserve(capacity); };
Clause::Clause(ID const& id, ClauseType const& clauseType) : Clause(id, 0, clauseType) {};

void Clause::reset(ClauseType const& clauseType)
{
	firstWatch = 0;
	secondWatch = 0;
	this->clauseType = clauseType;
	members.clear();
}

void Clause::reserveMemberSize(size_t const& size)
{
	members.reserve(size);
}

void Clause::setId(ID const& id)
{
	this->id = id;
}

ID Clause::getId() const
{
	return id;
}

void Clause::setAttacked(Argument& argument, Sign const& sign)
{
	assert(members.size() == 0);	
	addArgument(argument, sign);
	argument.addAttackedBy(*this);
}

Argument& Clause::getAttackedArgument() const
{
	assert(members.size() > 0); //Should never be the case
	assert(isAttack() || isSelfAttack()); //Only makes sense for attacks, as learned clauses have no real 'attacked' argument
	return *members[0].first;
}

void Clause::addArgument(Argument& argument, Sign const& sign)
{
	//Update watches
	if (members.size() < 2)
	{
		argument.addWatchedIn(*this);
		if (members.size() == 1)
			secondWatch = 1;
	}
	members.emplace_back(&argument, sign);
}

pair<vector<pair<Argument*, Sign>>::iterator, vector<pair<Argument*, Sign>>::iterator> Clause::getMembersIterator()
{
	assert(members.size() > 0);
	return pair(members.begin(), members.end());
}

pair<Argument*, Sign> const* Clause::get_member_element_at(size_t const& index) const
{
	assert(index < members.size());	
	return &(members[index]);	
}

size_t Clause::getMemberCount() const
{
	return members.size();
}

void Clause::incrementUseCounter()
{
	usageCounter++;
}

bool Clause::decrementUseCounter()
{
	assert(usageCounter > 0);
	usageCounter--;
	return isNotUsed();
}

bool Clause::isNotUsed()
{
	return usageCounter == 0;
}

void Clause::markAsSelfAttack()
{
	assert(clauseType == ClauseType::Attack);
	clauseType = ClauseType::SelfAttack;
}

void Clause::markAsLearned()
{
	clauseType = ClauseType::Learned;
}

/*
void Clause::markAsOutput()
{
	clauseType == ClauseType::Output;
}
*/

void Clause::markAsForgotten(size_t const& index)
{
	assert(clauseType == ClauseType::Learned);	
	clauseType = ClauseType::Forgotten;
	setForgottenIndex(index);	
}

void Clause::setForgottenIndex(size_t const& index)
{
	assert(clauseType == ClauseType::Forgotten);
	indexInForgottenClausesVector = index;
}

size_t Clause::getForgottenIndex() const
{
	assert(clauseType == ClauseType::Forgotten);
	return indexInForgottenClausesVector;
}

bool Clause::isSelfAttack() const
{
	return clauseType == ClauseType::SelfAttack;
}

bool Clause::isForgotten() const
{	
	return clauseType == ClauseType::Forgotten;
}

bool Clause::isAttack() const
{
	return clauseType == ClauseType::Attack;
}

bool Clause::isNotBlocked(DL const& dl) const
{
	if (isSelfAttack()) return false;
	assert(clauseType == ClauseType::Attack);	
	auto begin = members.begin();
	auto end = members.end();	
	begin++; //Skip attacked argument
	for (; begin != end; begin++)
		if(begin->first->getValue(dl) == -1)
			return false;
	return true;	
}

pair<size_t, size_t> Clause::getWatches() const
{
	return pair(firstWatch, secondWatch);
}

size_t Clause::getFirstWatch()
{
	return firstWatch;
}


size_t Clause::getSecondWatch()
{
	return secondWatch;
}

bool Clause::setWatch(bool const& isFirst, size_t const& index)
{
	size_t* oldWatch = isFirst ? &firstWatch : &secondWatch;
	if (*oldWatch == index)
		return false; //Nothing to do

	//Update watches
	members[*oldWatch].first->removeWatchedIn(*this); //We don't care for the bool returned as we will increment the use counter again next line
	members[index].first->addWatchedIn(*this);
	*oldWatch = index;
	return true;
}

bool Clause::watchesAreInvalidArgSet(DL const& dl) const
{
	auto& [firstWatchedArgument, firstSign] = members[firstWatch];
	auto& [secondWatchedArgument, secondSign] = members[secondWatch];

	//OK as we assume that at least on argument is not 0
	return firstWatchedArgument->getValue(dl) != firstSign && secondWatchedArgument->getValue(dl) != secondSign;
}

void Clause::printTrace()
{
	bool isFirst = true;
	for (auto& [argument, sign] : members)
	{
		if (isFirst)					
			isFirst = false;
		else
			cout << " ";
		
		if (sign == -1)
			cout << "-";
		cout << argument->getName();
	}
}

string Clause::toString()
{
	string returnValue = to_string(id);
	returnValue += ": ";
	ID count = 0;
	for (auto& [argument, sign] : members)
	{
		if (count == firstWatch)
			returnValue += "(";
		if (count == secondWatch)
			returnValue += "[";
		if (sign == -1)
			returnValue += "-";
		returnValue += argument->getName() + string("=") + to_string(argument->getValueFast()) + string("@") + to_string(argument->getDl());
		if (count == secondWatch)
			returnValue += "]";
		if (count == firstWatch)
			returnValue += ")";
		returnValue += " ";
		count++;
	}
	return returnValue;
}

