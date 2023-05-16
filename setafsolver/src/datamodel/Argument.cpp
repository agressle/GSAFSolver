#include "../../header/datamodel/Argument.hpp"
#include "../../header/datamodel/Clause.hpp"
#include "../../header/datamodel/Misc.hpp"
#include "../../header/datamodel/Instance.hpp"
#include "../../header/tools/Helper.hpp"
#include <fmt/core.h>
#include <iostream>
#include <cassert>
#include <tuple>

using namespace std;

Argument::Argument(ID const& id, ID const& position, string const& name) : id(id), position(position), name(name) {};
Argument::Argument(ID const& id, ID const& position) : id(id), position(position), name(std::to_string(id + 1)) {};
Argument::Argument(ID const& id) : id(id), position(id), name(std::to_string(id + 1)) {};

void Argument::setName(string const& name)
{
	this->name = name;
}

string Argument::getName() const
{
	return name;
}

void Argument::setIdAndPosition(ID const& id)
{
	this->id = id;
	this->name = std::to_string(id + 1);
	this->setPosition(id);
}

ID Argument::getId() const
{
	return id;
}

DL Argument::getDl() const
{
	return dl;
}

void Argument::setPosition(ID const& position)
{	
	this->position = position;
}

ID Argument::getPosition() const
{
	return position;
}

void Argument::setValue(Sign const& value, DL const& dl, Clause* reason, Instance &instance)
{	
	assert(value >= -1);
	assert(value <= 1);

	//Debug trace output
	#ifdef TRACE
		cout << "\tSet ";
		if (value == -1)
			cout << "-";
		cout << getName() << "@" << dl << " <- ";		
		if (reason == nullptr)
		{
			if (dl == 0)
				cout << "forced" << endl;
			else
				cout << "guess" << endl;
		}
		else
		{
			cout << reason->getId() << " {";			
			reason->printTrace();
			cout << "}" << endl;
		}				
	#endif

	this->value = value;
	this->dl = dl;
	if (this->reason != nullptr && this->reason->decrementUseCounter())
		instance.recycleClause(*reason);
	
	this->reason = reason;
	if (reason != nullptr)
		reason->incrementUseCounter();
}

Sign Argument::getValueFast() const
{
	return value;
}

Sign Argument::getValue(DL const& dl) const
{
	return this->dl > dl ? 0 : value;
}

void Argument::addWatchedIn(Clause& clause)
{
	clause.incrementUseCounter();
	auto index = watchedIn.size();
	watchedIn.push_back(&clause);
	watchedInClauseIndex[clause.getId()] = index;
}

bool Argument::removeWatchedIn(Clause& clause)
{
	auto node = watchedInClauseIndex.extract(clause.getId());
	assert(!node.empty());
	
	auto& index = node.mapped();
	assert(index < watchedIn.size());

	auto isNotLast = index + 1 < watchedIn.size();
	Helper::swapRemove(watchedIn, index);
	
	if (isNotLast)
	{
		//Update the index of the clause we swapped in for the one we removed
		assert(index < watchedIn.size());
		auto otherClause = watchedIn[index];
		auto [_, wasInserted] = watchedInClauseIndex.insert_or_assign(otherClause->getId(), index);
		assert(!wasInserted);
	}

	return clause.decrementUseCounter();	
}

void Argument::setHeuristicsValue(double const& value)
{
	heuristicsValue = value;
}

double Argument::getHeuristicsValue() const
{
	return heuristicsValue;
}

void Argument::addAttackedBy(Clause& clause)
{
	this->attackedBy.push_back(&clause);
}

Clause& Argument::getAttackingClause(size_t const& index) const
{
	assert(index < attackedBy.size());
	return *attackedBy[index];
}

size_t Argument::getAttackedByCount() const
{
	return attackedBy.size();
}

pair<vector<Clause*>::const_iterator, vector<Clause*>::const_iterator> Argument::getAttackedByIterator() const
{
	return pair(attackedBy.cbegin(), attackedBy.cend());
}

void Argument::setWatchedAttackedIndex(Clause& clause, size_t const& index)
{
	assert(index < attackedBy.size());
	watchedInAttackIndex = index;

	auto [begin, end] = clause.getMembersIterator();	
	begin++; //Skip first (= attacked argument, i.e. this)
	for (auto& member = begin; begin != end; begin++)
	{		
		auto& [argument, sign] = *member;
		argument->stabilityWatchPush(*this, index);
	}
}

size_t Argument::getWatchedAttackIndex() const
{
	assert(value == -1);
	return watchedInAttackIndex;
}

size_t Argument::getWatchedInCount() const
{
	return watchedIn.size();
}

Clause& Argument::getWatchedInElementAt(size_t const& index) const
{
	assert(index < watchedIn.size());
	return *watchedIn[index];
}

bool Argument::stabilityWatchIsEmpty() const
{
	return stabilityWatch.empty();
}

pair<Argument&, size_t> Argument::stabilityWatchPop()
{
	assert(!stabilityWatch.empty());
	auto& [argument, index] = stabilityWatch.back();
	auto returnValue = pair<Argument&, size_t>(*argument, index);
	stabilityWatch.pop_back();
	return returnValue;
}

void Argument::stabilityWatchPush(Argument& argument, size_t const& index)
{
	stabilityWatch.emplace_back(&argument, index);
}

Clause* Argument::getReason()
{
	return reason;
}

void Argument::reset()
{
	#ifdef TRACE
		cout << "\tReset " << getName() << endl;
	#endif
	value = 0;
	dl = std::numeric_limits<ID>::max();
}

string Argument::toString()
{	
	if (value == 0)
		return fmt::format("?{}", getName());
	else
		return fmt::format("{}{}@{}<-{}",
			value == -1 ? "-" : "",
			getName(),
			getDl(),
			reason == nullptr ? "guess" : to_string(reason->getId()));
}
