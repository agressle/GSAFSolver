#include <cassert>
#include <iostream>
#include <cmath>

#include "../header/tools/Helper.hpp"
#include "../header/Solver.hpp"

Solver::Solver(Instance& instance, Semantics const& semantics, Heuristics const& heuristics, chrono::time_point<chrono::high_resolution_clock>& firstModelTime, unsigned long& modelCount, double& percentageSolved, unsigned long& numberOfModels, bool const& printModels, double const& clForgetPercentage, double const& clGrowthRate, std::unique_ptr<ofstream>& proofFile) :
	instance(instance),
	semantics(semantics),
	heuristics(heuristics),
	firstModelTime(firstModelTime),
	modelCount(modelCount),
	percentageSolved(percentageSolved),
	numberOfModels(numberOfModels),
	printModels(printModels),
	learnedClausesToForgetThreshold(instance.getNumberOfArguments()),
	clForgetPercentage(clForgetPercentage),
	clGrowthRate(clGrowthRate),
	proofFile(proofFile)
{
	assingedArguments.reserve(instance.getNumberOfArguments());
};

void Solver::solve()
{
	switch (semantics.getType())
	{
	case Semantics::SemanticsType::stable:
		percentageSolved = calculate_stable();
		break;
	}

	if (proofFile)
		if (modelCount == 0)
			*proofFile << "0";
}

bool Solver::printAssignment()
{
	//Record the first model time 
	if (modelCount == 0)
		firstModelTime = chrono::high_resolution_clock::now();

	modelCount++;

	//Print the model
	if (printModels)
	{
		printf("Model %lu\n", modelCount);
		auto [begin, end] = instance.getArgumentsIterator();

		//Print until the first argument which is in is found (thus the one without leading " ")
		for (; begin != end; begin++)
		{
			auto& argument = *begin;
			assert(argument.getValueFast() != 0);
			if (argument.getValueFast() == 1)
			{
				printf("%s", argument.getName().c_str());
				begin++;
				break;
			}
		}

		//Print the remaining arguments with leading " "
		for (; begin != end; begin++)
		{
			auto& argument = *begin;
			assert(argument.getValueFast() != 0);
			if (argument.getValueFast() == 1)
				printf(" %s", argument.getName().c_str());
		}

		printf("\n");
	}

	return numberOfModels == modelCount;
}

void Solver::checkAndForgetClauses()
{
	auto numberOfLearnedClauses = (double)instance.getNumberOfLearnedClauses();
	if (numberOfLearnedClauses > learnedClausesToForgetThreshold)
	{
		if (modelCount == 0)
			instance.forgetClauses(numberOfLearnedClauses * clForgetPercentage, proofFile, proofFile != nullptr);
		else
			instance.forgetClauses(numberOfLearnedClauses * clForgetPercentage, proofFile, false);
		learnedClausesToForgetThreshold *= clGrowthRate;
	}
}

void Solver::writeProofClause(Clause& clause, bool isImplicitClause)
{
	if (modelCount == 0)
	{
		if (isImplicitClause)
			*proofFile << "i ";

		for (auto [beginMember, endMember] = clause.getMembersIterator(); beginMember != endMember; beginMember++)
		{
			auto& [argument, sign] = *beginMember;
			if (sign == -1)
				*proofFile << "-";
			*proofFile << argument->getName();
			*proofFile << " ";
		}
		*proofFile << "0\n";
	}
}

Clause& Solver::buildImplictClause(Argument& arg)
{
	auto& clause = instance.getNewLearnedClause(arg.getAttackedByCount());
	clause.addArgument(arg, 1);

	//Keep track of which arguments we have added so that we don't add them twice
	helperHashsetID.clear();

	//For each attacking clause, select an argument with the highest dl	
	for (auto [begin, end] = arg.getAttackedByIterator(); begin != end; begin++)
	{
		auto& attackingClause = *begin;
		Argument* selectedArgument = nullptr;
		if (!attackingClause->isSelfAttack())
		{
			//Ensure that always a literal at the maxDL has been chosen (to ensure that resolution is possible)
			auto [beginMember, endMember] = attackingClause->getMembersIterator();
			assert(beginMember != endMember);
			beginMember++; //Skip attacked argument (= argument variable)			
			for (; beginMember != endMember; beginMember++)
			{
				auto& [attackingArgument, sign] = *beginMember;
				if ((selectedArgument == nullptr || attackingArgument->getDl() > selectedArgument->getDl()) && attackingArgument->getValue(currentDl) == sign)
				{
					selectedArgument = attackingArgument;
					if (selectedArgument->getDl() == currentDl)
						break;
				}
			}

			assert(selectedArgument != nullptr);
			if (helperHashsetID.insert(selectedArgument->getId()).second)
				clause.addArgument(*selectedArgument, selectedArgument->getValueFast() * -1);
		}
	}

#ifdef TRACE
	cout << "\tImplicit: " << clause.getId() << " {";
	clause.printTrace();
	cout << "}" << endl;
#endif // TRACE

	if (proofFile)
		writeProofClause(clause, true);


	return clause;
}

Clause* Solver::recomputeWatchedAttack(Argument& arg, size_t* const forAttackIndex)
{
	//Note that we don't care if the arguments watched_attacked_index is still 0 due to initial assignment.
	//In this case, we will set it here anyway or backtrack
	//Otherwise, this attack will not be used again anyway as this method has been called exactly because this attack is no longer valid

	//Nothing to do if the argument it not currently out
	if (arg.getValue(currentDl) != -1)
		return nullptr;

	if (arg.getAttackedByCount() == 0)
		return &buildImplictClause(arg);

	auto index = arg.getWatchedAttackIndex();

	//Check all the attacks, starting with the old one (or +1 if it is the one we are called for) and wrap around
	if (forAttackIndex != nullptr)
	{
		//If the argument is no longer out or the watch is no longer relevant, we got nothing to do
		if (index != *forAttackIndex)
			return nullptr;

		//Otherwise, no point in checking the attack again, as this method has been called because it is blocked
		index += 1;
		if (index == arg.getAttackedByCount())
			index = 0;
	}

	do
	{
		auto& attack = arg.getAttackingClause(index);
		if (attack.isNotBlocked(currentDl))
		{
			arg.setWatchedAttackedIndex(attack, index);
			return nullptr;
		}

		index++;
		if (index == arg.getAttackedByCount())
			index = 0;
	} while (index != arg.getWatchedAttackIndex());

	//We did not find any valid witness -> build and return implicit clause
	return &buildImplictClause(arg);
}

Clause* Solver::checkClause(Clause& clause, Argument* argument, bool* wasRemoved)
{
	//Unit clause are self attacks and should have been handled at dl 0 an thus should never occur here
	assert(clause.getMemberCount() > 1 || !clause.isAttack());

	//If the clause has been forgotten, we remove it from the watched_in list and maybe recycle it if the used counter is 0
	if (clause.isForgotten())
	{
		assert(argument != nullptr);
		if (argument->removeWatchedIn(clause))
			instance.recycleClause(clause);
		return nullptr;
	}

	//See which watch we have to verify
	size_t watch = clause.getFirstWatch();
	size_t other = clause.getSecondWatch();
	if (argument != nullptr && argument != clause.get_member_element_at(watch)->first)
	{
		watch = clause.getSecondWatch();
		other = clause.getFirstWatch();
	}

	size_t start = watch;
	//Check and update watches		
	while (true)
	{
		if (watch != other) //Skip if we count at the other watch
		{
			auto member = clause.get_member_element_at(watch);
			if (member->first->getValueFast() != (member->second * -1))
				break; //We found our new argument to watch. It either already satisfies the clause or could do it later on					
		}

		//Increment watch and wrap around if the hit the end
		watch++;
		if (watch == clause.getMemberCount())
			watch = 0;

		if (watch == start)
		{
			//All members except for the other index are conflicting -> assert other index
			auto otherMember = clause.get_member_element_at(other);
			return setAndPropagate(*otherMember->first, otherMember->second, &clause);
		}
	}

	//Update the watches of the clause if necessary
	if (wasRemoved == nullptr)
		clause.setWatch(start == clause.getFirstWatch(), watch);
	else
		*wasRemoved = clause.setWatch(start == clause.getFirstWatch(), watch);

	//If we checked the clause for a specific argument, we can end. Otherwise we need to check the other watch too
	if (argument != nullptr)
		return nullptr;

	//Otherwise we need to check the other argument too
	return checkClause(clause, clause.get_member_element_at(other)->first, wasRemoved);
}

Clause* Solver::setAndPropagate(Argument& argument, Sign value, Clause* reason)
{
	//If the argument already has a value set at a lower or equal decision level, we return the clause as conflicting
	if (argument.getDl() <= currentDl)
	{
		if (argument.getValueFast() == value)
			return nullptr;

		assert(reason != nullptr || currentDl == 0); //We would not have guessed a conflicting assignment
		return reason;
	}

	//Do the assignment
	assingedArguments.push_back(&argument);
	argument.setValue(value, currentDl, reason, instance);

	//We check all the clauses in which the argument in watched in
	//Cant use iterator as the list might change, as we might remove the current argument
	for (size_t i = 0; i < argument.getWatchedInCount();)
	{
		bool didMove = false;
		auto& clause = argument.getWatchedInElementAt(i);
		if (clause.watchesAreInvalidArgSet(currentDl))
		{
			auto result = checkClause(clause, &argument, &didMove);
			if (result != nullptr)
				return result;

			if (didMove)
				continue; //We removed the clause from the watchedIn list, thus we need to check index i again, as we swapped another clause in from further back
		}
		i++;
	}

	//If this argument is set in, we don't need to check stability
	if (value == 1)
		return nullptr;

	//If we are here, value must be -1, thus we need to check stability
	//Check all arguments for which this argument was used as guarantee for stability, as this argument is out and the resulting attack is blocked
	while (!argument.stabilityWatchIsEmpty())
	{
		auto [stabilityArgument, index] = argument.stabilityWatchPop();
		auto result = recomputeWatchedAttack(stabilityArgument, &index);
		if (result != nullptr)
		{
			argument.stabilityWatchPush(stabilityArgument, index);
			return result;
		}
	}

	//Check stability of this argument and return
	if (reason == nullptr || !reason->isAttack() || reason->getAttackedArgument().getId() != argument.getId())
		return recomputeWatchedAttack(argument, nullptr); //We either got no reason for the set of this argument or it is either not an attack or is not attacking the given argument (just asserting somewhere else within the clause), thus we need to check stability

	//The reason to set this argument was an attack directed at the argument, does stability is guaranteed
	return nullptr;
}

Clause* Solver::resolveConflictAndUpdateDL(Clause& conflictingClause, Argument** uipArgument)
{
	//Flipping caused conflict -> we dont analyse
	if (currentDl == backjumpingBound)
	{
		if (currentDl == 0)
			return nullptr; //No further backtracking possible

		currentDl -= 1;
		backjumpingBound = currentDl;
		*uipArgument = nullptr;
		return &conflictingClause;
	}

	//Find the second highest of the members of the clause		
	auto [begin, end] = conflictingClause.getMembersIterator();
	*uipArgument = begin->first;
	DL highestDl = (*uipArgument)->getDl();
	DL secondHighestDl = numeric_limits<DL>::max();
	ID atMaxDL = 1;
	begin++;
	for (; begin != end; begin++)
	{
		auto& argument = begin->first;
		auto dl = argument->getDl();
		if (dl > highestDl)
		{
			*uipArgument = argument;
			secondHighestDl = highestDl;
			highestDl = dl;
			atMaxDL = 1;
		}
		else
		{
			if (dl == highestDl)
			{
				atMaxDL++;
				if (argument->getReason() != nullptr)
					*uipArgument = argument;
			}
			else
			{
				if (dl > secondHighestDl)
					secondHighestDl = dl;
			}
		}
	}

	//The highest dl in the clause is 0, thus we cant backtrack any further
	if (highestDl == 0)
		return nullptr;

	//Only one DL, thus backtrack 1
	if (secondHighestDl > highestDl)
	{
		//currentDl -= 1;
		//backjumpingBound = currentDl;
		//*uipArgument = nullptr;
		//return &conflictingClause;
		secondHighestDl = 0;
	}

	//The conflicting clause is already asserting
	if (atMaxDL == 1)
	{
		currentDl = max(backjumpingBound, secondHighestDl);
		return &conflictingClause;
	}

	//Build the learned clause
	auto& learnedClause = instance.getNewLearnedClause(1);
	helperVectorArgumentSign.clear();
	helperHashsetID.clear(); //Used to keep track of which arguments we have already added to avoid duplicates

	//We copy all argument that are not at the highest dl to the resolvent clause and remember the argument at the highest dl to resolve based on them later
	for (auto [memberCount, i] = tuple{ conflictingClause.getMemberCount(), (size_t)0 }; i < memberCount; i++)
	{
		pair<Argument*, Sign> const* member = conflictingClause.get_member_element_at(i);
		auto& [argument, sign] = *member;
#ifndef NDEBUG
		auto result = helperHashsetID.insert(argument->getId());
		assert(result.second);	//No duplicate arguments should be possible here
#else
		helperHashsetID.insert(argument->getId());
#endif // !NDEBUG				
		if (argument->getDl() < highestDl)
			learnedClause.addArgument(*argument, sign);
		else
			helperVectorArgumentSign.push_back(member);
	}
	assert(!helperVectorArgumentSign.empty());

	//Next we go trough the helperVectorArgumentSign until there is only one left
	while (helperVectorArgumentSign.size() > 1)
	{
		auto entry = helperVectorArgumentSign.back();

		//We need an argument that is not guessed, i.e. where the reason is not none
		if (entry->first->getReason() != nullptr)
			helperVectorArgumentSign.pop_back();
		else
		{
			//The last value in the vector has no reason, thus we use the first one instead and move the previous last to the front
			entry = helperVectorArgumentSign[0];
			Helper::swapRemove(helperVectorArgumentSign, 0);
		}

		auto& argument = entry->first;
		assert(argument->getReason() != nullptr);
		//TODO: careful for shared clauses, they might have 2 arguments with only guesses. Return conflicting clause?

		//We now replace the arg with the members of its reason		
		helperHashsetID.erase(argument->getId());
		auto reason = argument->getReason();
		for (auto [memberCount, i] = pair(reason->getMemberCount(), (size_t)0); i < memberCount; i++)
		{
			auto reasonMember = reason->get_member_element_at(i);
			auto& [reasonArgument, reasonSign] = *reasonMember;
			if (reasonArgument->getId() != argument->getId() && helperHashsetID.insert(reasonArgument->getId()).second)
			{
				if (reasonArgument->getDl() == highestDl)
					helperVectorArgumentSign.push_back(reasonMember);
				else
				{
					assert(reasonArgument->getDl() < highestDl);
					learnedClause.addArgument(*reasonArgument, reasonSign);
				}
			}
		}
	}

	//Add the last reaming max dl arg into the learned clause
	auto& [lastArgument, lastSign] = *helperVectorArgumentSign.back();
	learnedClause.addArgument(*lastArgument, lastSign);
	*uipArgument = lastArgument;

	//Backtrack to the second highest dl in the resulting clause or 0 if we only have 1 argument
	highestDl = 0;
	secondHighestDl = 0;
	for (auto [begin, end] = learnedClause.getMembersIterator(); begin != end; begin++)
	{
		auto dl = begin->first->getDl();
		if (dl > highestDl)
		{
			secondHighestDl = highestDl;
			highestDl = dl;
		}
	}
	currentDl = max(backjumpingBound, secondHighestDl);

#ifdef TRACE
	cout << "\tLearned: " << learnedClause.getId() << " {";
	learnedClause.printTrace();
	cout << "}" << endl;
#endif // TRACE

	if (proofFile)
		writeProofClause(learnedClause, false);

	return &learnedClause;
}


bool Solver::backtrackForClause(Clause& conflictingClause)
{
	auto clause = &conflictingClause;
	Argument* resultingArgument = nullptr;
	Sign oldSign = 1;
	DL prevDL;
	while (clause != nullptr)
	{
		prevDL = currentDl;
		clause = resolveConflictAndUpdateDL(*clause, &resultingArgument);
		if (clause == nullptr)
			return false; //Resolving the conflict would go beyond dl 0, thus we are finished			

		if (prevDL != currentDl)
		{
			if (resultingArgument == nullptr)
			{
				//Need to flip decision literal
				resultingArgument = backtrackToCurrentDL(oldSign);
				if (!doAssignment(*resultingArgument, oldSign * -1, nullptr))
					return false;
			}
			else
			{
				//Need to flip UIP literal
				oldSign = resultingArgument->getValueFast();
				Sign tmpSign; //Throw away				
				backtrackToCurrentDL(tmpSign);
				if (!doAssignment(*resultingArgument, oldSign * -1, clause))
					return false;
			}
		}

		//Check if we need another iteration
		clause = checkClause(*clause, nullptr, nullptr);
	}

	return true;
}

Argument* Solver::backtrackToCurrentDL(Sign& oldSign)
{
#ifdef TRACE		
	cout << "\tBacktrack to dl " << currentDl << endl;
#endif // TRACE

	assert(!assingedArguments.empty());

	//Undo all guesses until we reach the decision literal for the destination dl		
	Argument* argument = nullptr;
	while (!assingedArguments.empty() && assingedArguments.back()->getDl() > currentDl)
	{
		argument = assingedArguments.back();
		oldSign = argument->getValueFast();
		argument->reset();
		nextGuessPosition = min(nextGuessPosition, argument->getPosition());
		assingedArguments.pop_back();
	}

	return argument;
}


bool Solver::computeGrounded()
{
	//Contains, for each attack, either Nothing if the attack is blocked (= some supporting argument is set to -1) or the number of supporting arguments that are not yet set to 1 and the attack argument
	vector<pair<optional<size_t>, Argument*>> attacks;
	attacks.reserve(instance.getNumberOfAttacks());
	//For each argument by ID, contains all attacks in which this argument is in the support
	vector<vector<Clause*>> containedInAsAttacker(instance.getNumberOfArguments());
	//For each argument, contains the number of not-blocked attacks directed at it
	vector<size_t> incomingAttacksCount(instance.getNumberOfArguments(), 0);

	//Fill the attacks and containedInAsAttacker vectors
	for (auto [begin, end] = instance.getAttackIterator(); begin != end; begin++)
	{
		auto attack = &*begin;
		size_t count = 0;
		bool isBlocked = false;

		auto [beginMember, endMember] = attack->getMembersIterator();
		beginMember++; //We don't want the attacked argument

		for (; beginMember != endMember; beginMember++)
		{
			auto& argument = beginMember->first;
			containedInAsAttacker[argument->getId()].push_back(attack);
			if (!isBlocked)
			{
				switch (argument->getValueFast())
				{
				case -1:
					isBlocked = true;
					break;
				case 0:
					count++;
					break;
				default:
					//Nothing
					break;
				}
			}
		}

		if (isBlocked)
			attacks.emplace_back(optional<size_t>(), &attack->getAttackedArgument());
		else
		{
			incomingAttacksCount[attack->getAttackedArgument().getId()]++;
			attacks.emplace_back(optional(count), &attack->getAttackedArgument());
		}
	}

	//The arguments that we need to assigned with either the reason why we must set them -1 or nullptr if we need to set them 1
	vector<tuple<Argument*, Clause*>> argsToDo;

	//All arguments that have no incoming attack must be in
	for (size_t i = 0; i < incomingAttacksCount.size(); i++)
		if (incomingAttacksCount[i] == 0)
			argsToDo.emplace_back(&instance.getArgument(i), nullptr);

	//All arguments with an incoming attack with all attackers in must be out
	for (size_t i = 0; i < attacks.size(); i++)
	{
		auto& [count, argument] = attacks[i];
		if (count.has_value() && *count == 0)
			argsToDo.emplace_back(argument, &instance.getAttack(i));
	}

	//Do the assignments
	while (!argsToDo.empty())
	{
		auto [argument, reason] = argsToDo.back();
		argsToDo.pop_back();

		Sign sign = reason == nullptr ? 1 : -1;

		if (proofFile)
			if (argument->getValue(0) == 0)
				buildImplictClause(*argument);

		if (setAndPropagate(*argument, sign, reason) != nullptr)
			return false; //Assignment caused a conflict

		//Check all attacks that contain the argument
		for (auto& attack : containedInAsAttacker[argument->getId()])
		{
			auto& [count, attackedArgument] = attacks[attack->getId()];
			//if the attack is already blocked, we are done
			if (count.has_value())
			{
				if (sign == -1)
				{
					//If we assigned the arg to -1, we need to block the attack
					count.reset();
					if (--incomingAttacksCount[attackedArgument->getId()] == 0) //Decrement as the current attack is now blocked									
						argsToDo.emplace_back(attackedArgument, nullptr); 	//This was the last not blocked attack on the attackedArgument, it must be in					
				}
				else
				{
					//If we assign the arg to 1 and this causes the attack not set counter to go to 0, the attacked argument is out
					if (--(*count) == 0)
						argsToDo.emplace_back(attackedArgument, attack);
				}
			}
		}
	}

	return true;
}

bool Solver::doAssignment(Argument& argument, Sign sign, Clause* reason)
{
	auto result = setAndPropagate(argument, sign, reason);
	if (result == nullptr)
	{
		if (nextGuessPosition == argument.getPosition())
			nextGuessPosition += 1;
		return true;
	}

	return backtrackForClause(*result);
}


double Solver::calculate_stable()
{
	//If we have 0 arguments, there is only the empty set
	if (instance.getNumberOfArguments() == 0)
	{
		printAssignment();
		return 1.0;
	}

	//We do at dl 0 all signed that are forced
	//Argument that have incoming attacks that are only contain themselves are out
	for (auto [begin, end] = instance.getAttackIterator(); begin != end; begin++)
		if (begin->getMemberCount() == 1)//If only 1 member, than it is only the argument itself
			if (setAndPropagate(begin->getAttackedArgument(), -1, nullptr) != nullptr)
				return 1.0;

	//Now we do all required assignments provided by the caller
	for (auto [begin, end] = instance.getRequiredArgumentsIterator(); begin != end; begin++)
	{
		auto& [argument, sign] = *begin;
		if (argument->getValueFast() * -1 == sign || setAndPropagate(*argument, sign, nullptr) != nullptr)
			return 1.0; //Assignment causes conflict
	}

	//And finally, we compute the grounded extension as base			
	if (!computeGrounded())
		return 1.0;

	//Apply heuristics
	auto [sortedArguments, guessOrder] = heuristics.apply(instance, currentDl);

	//Start guessing
	while (true)
	{
		if (Helper::receivedSignal())
			return calculatePercentageSolved(sortedArguments, guessOrder); //Solver interrupted by signal

		checkAndForgetClauses(); //Forget clauses if necessary		

		if (nextGuessPosition == sortedArguments.size())
		{
			//We have a full assignment
			if (printAssignment())
				calculatePercentageSolved(sortedArguments, guessOrder); //Required number of models found

			if (currentDl == 0)
				return 1.0; //No further backtracking possible			

			//Flip last decision literal
			currentDl--;
			backjumpingBound = currentDl;
			Sign oldSign = 1;
			Argument* nextGuessArgument = backtrackToCurrentDL(oldSign); //Cant be null as we just decreased from a DL > 0
			if (!doAssignment(*nextGuessArgument, oldSign * -1, nullptr))
				return 1.0;
			continue;
		}

		auto& argument = sortedArguments[nextGuessPosition];
		//If the argument was already assigned, we can skip t		
		if (argument->getValueFast() != 0)
		{
			nextGuessPosition++;
			continue;
		}

		//Guess for the current argument
		currentDl++;
		if (!doAssignment(*argument, guessOrder[nextGuessPosition], nullptr))
			return 1.0;
	}
	return 1.0;
}


double Solver::calculatePercentageSolved(vector<Argument*>& sortedArguments, vector<Sign>& guessOrder)
{
	double percentageSolved = 0;
	for (ID i = 0; i < sortedArguments.size(); i++)
	{
		if (sortedArguments[i]->getValue(currentDl) == guessOrder[i] * -1)
			percentageSolved += pow(0.5, i + 1);
	}
	return percentageSolved;
}