#include "../../header/datamodel/Heuristics.hpp"
#include "../../header/tools/Helper.hpp"
#include <optional>
#include <algorithm>
#include <cmath>

using namespace std;

Heuristics::Heuristics(HeuristicTypes const& type) : type(type) {};
Heuristics::Heuristics() : Heuristics(HeuristicTypes::None) {};

optional<Heuristics> Heuristics::tryParse(char const* const s)
{	
	//String representations
	static string heuristicTypeNoneString("None");	
	static string heuristicTypeMaxOutDegreeString("MaxOutDegree");
	static string heuristicTypeMaxInDegreeString("MinInDegree");
	static string heuristicTypePathLengtPrefixString("PathLength");
	static string heuristicTypePathLengtModifiedPrefixString("PathLengthModified");

	if (!heuristicTypeNoneString.compare(s))
	{
		return Heuristics(HeuristicTypes::None);
	}
	
	if (!heuristicTypeMaxOutDegreeString.compare(s))
	{
		return Heuristics(HeuristicTypes::MaxOutDegree);
	}		

	if (!heuristicTypeMaxInDegreeString.compare(s))
	{
		return Heuristics(HeuristicTypes::MinInDegree);
	}			
	
	if (heuristicTypePathLengtPrefixString.compare(0, heuristicTypePathLengtPrefixString.length(), s))
	{		
		optional<unsigned short> param = Helper::tryParseUShort(s + heuristicTypePathLengtPrefixString.size());
		if (param)
		{
			Heuristics h(HeuristicTypes::PathLength);
			h.heuristicsParameter1 = *param;
			return h;
		}
	}

	if (heuristicTypePathLengtModifiedPrefixString.compare(0, heuristicTypePathLengtModifiedPrefixString.length(), s))
	{
		optional<unsigned short> param = Helper::tryParseUShort(s + heuristicTypePathLengtModifiedPrefixString.size());
		if (param)
		{
			Heuristics h(HeuristicTypes::PathLengthModified);
			h.heuristicsParameter1 = *param;
			return h;
		}
	}
	
	return {};
}

/**
 * {@return a vector that, for each argument, returns all arguments that are involved in some attack directed at the original argument}
 */
vector<vector<Argument*>> computeAttackedBy(Instance& instance)
{
	vector<vector<Argument*>> returnValue(instance.getNumberOfArguments());	
	for (auto [begin, end] = instance.getAttackIterator(); begin != end; begin++)
	{
		auto [memberBegin, memberEnd] = (*begin).getMembersIterator();
		
		auto [attackedArgument, _] = *memberBegin;
		memberBegin++;

		for (; memberBegin != memberEnd; memberBegin++)
		{
			auto& [attackingArgument, _] = *memberBegin;
			returnValue[attackedArgument->getId()].push_back(attackingArgument);
		}
	}
	return returnValue;
}


/**
 * Computes the path length heuristics value for each argument a, i.e. sum_{i=1}^n d_i^+(a) / 2^i
 */
vector<double> computePathLength(vector<Argument*> const& arguments, unsigned short const& requestedPathLength, vector<vector<Argument*>> const& attackedBy)
{
	vector<double> values(arguments.size(), 0.0);	

	//Idea: one contains the number of paths of length n, the other is used to compute the sum of paths of length n of all attacked arguments and thus the paths of length n + 1 from the current argument
	vector<vector<unsigned long>> helper(2, vector<unsigned long>(arguments.size()));
	
	//Path length 1
	for (ID i = 0; i < arguments.size(); i++)
	{
		helper[0][i] = arguments[i]->getHeuristicsValue();
		values[i] = arguments[i]->getHeuristicsValue() / 2.0;
	}

	//Paths length 2...n
	size_t workingIndex = 1, prevResultIndex = 0;
	for (unsigned short pathLength = 2; pathLength <= requestedPathLength; pathLength++)
	{		
		for (ID i = 0; i < arguments.size(); i++)
		{			
			helper[workingIndex][i] = 0;

			//For each argument, add the number of paths with the previous length for each argument that attacks it
			for (auto& attacker : attackedBy[i])		
				helper[workingIndex][i] += helper[prevResultIndex][attacker->getId()];

			//Update the heuristics value
			values[i] += ((double)helper[workingIndex][i]) / (double)pow(2.0, pathLength);			
		}

		//Swap workingIndex and prevResultIndex
		workingIndex = (workingIndex + 1) % 2;
		prevResultIndex = (prevResultIndex + 1) % 2;
	}		

	return values;
}

/**
 * Modifies the path length heuristics value for each argument a by adding sum_{i=1}^n d_i^-(a) / (-2)^i - |{b | (b, a) \in R} / 2
 */
void computeModifiedPathLength(vector<Argument*> const& arguments, unsigned short const& requestedPathLength, vector<double>& pathLenghtValues)
{	
	//Idea: one contains the number of paths of length n, the other is used to compute the sum of paths of length n of attacking arguments and thus the paths of length n + 1 from the current argument
	vector<vector<unsigned long>> helper(2, vector<unsigned long>(arguments.size()));

	//Path length 1
	for (ID i = 0; i < arguments.size(); i++)
	{
		helper[0][i] = arguments[i]->getAttackedByCount();
		pathLenghtValues[i] += arguments[i]->getAttackedByCount() / -2.0;
	}

	//Paths length 2...n
	size_t workingIndex = 1, prevResultIndex = 0;
	for (unsigned short pathLength = 2; pathLength <= requestedPathLength; pathLength++)
	{
		for (ID i = 0; i < arguments.size(); i++)
		{
			helper[workingIndex][i] = 0;

			//For each argument, add the number of paths with the previous length for each argument that attacks it			
			for (auto [begin, end] = arguments[i]->getAttackedByIterator(); begin != end; begin++)
			{
				auto [beginAttackers, endAttackers] = (*begin)->getMembersIterator();
				beginAttackers++; //Skip attacked argument				
				for(; beginAttackers != endAttackers; beginAttackers++)
					helper[workingIndex][i] += helper[prevResultIndex][beginAttackers->first->getId()];
			}

			//Update the heuristics value
			pathLenghtValues[i] += ((double)helper[workingIndex][i]) / (double)pow(-2.0, pathLength);
		}

		//Swap workingIndex and prevResultIndex
		workingIndex = (workingIndex + 1) % 2;
		prevResultIndex = (prevResultIndex + 1) % 2;
	}

	//Add the last term
	for (ID i = 0; i < arguments.size(); i++)
		pathLenghtValues[i] -= arguments[i]->getAttackedByCount() / 2;	
}

pair<vector<Argument*>, vector<Sign>> Heuristics::apply(Instance& instance, DL const& dl) const
{
	auto arguments = instance.getArgumentsCopy();		

	//For the pathlength and modified pathlength heuristics
	if (type == HeuristicTypes::PathLength || type == HeuristicTypes::PathLengthModified)
	{
		auto pathLength = std::get<unsigned short>(heuristicsParameter1);

		//Compute the heuristics
		auto attackedBy = computeAttackedBy(instance);
		auto pathLengthValues = computePathLength(arguments, pathLength, attackedBy);
		if (type == HeuristicTypes::PathLengthModified)
			computeModifiedPathLength(arguments, pathLength, pathLengthValues);

		//Update the heuristics value
		for (ID i = 0; i < arguments.size(); i++)
			arguments[i]->setHeuristicsValue(pathLengthValues[i]);
	}	

	//project to arguments without a set value
	for(size_t i = 0; i < arguments.size(); i++)
		if (arguments[i]->getValue(dl) != 0)
		{
			Helper::swapRemove(arguments, i);
			i--;
		}		

	//Sort the arguments
	switch (type)
	{
		case HeuristicTypes::None:
			//Nothing to do
			break;		
		case HeuristicTypes::MinInDegree:
			//Sort ascending
			std::sort(arguments.begin(), arguments.end(),
				[&](Argument* const& first, Argument* const& second) { return first->getAttackedByCount() < second->getAttackedByCount(); });
			break;
		case HeuristicTypes::MaxOutDegree:
		case HeuristicTypes::PathLength:
		case HeuristicTypes::PathLengthModified:
			//Sort descending
			std::sort(arguments.begin(), arguments.end(),
				[&](Argument* const& first, Argument* const& second) { return first->getHeuristicsValue() > second->getHeuristicsValue(); });
			break;					
	}	

	//Update the positions
	for (ID i = 0; i < arguments.size(); i++)
		arguments[i]->setPosition(i);

	//Define the guess order
	vector<Sign> guessOrder(arguments.size(), 1);

	return pair(arguments, guessOrder);
}

