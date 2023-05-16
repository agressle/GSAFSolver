#include "../../header/tools/IDTrie.hpp"


void IDTrie::insert(ID attackedArgument, vector<ID> const& members)
{
	//Find or create the root node for the attacked argument
	auto node = &rootNodes[attackedArgument];
	if (*node == nullptr)
		*node = &nodeStash.emplace_back();
		
	//Follow or create the path to the leaf node for the members	
	for(auto member : members)
	{
		node = &(*node)->children[member];
		if(*node == nullptr)
			*node = &nodeStash.emplace_back();				
	}

	//Set the leaf node to contain
	(*node)->isContained = true;
}


bool IDTrie::containsSubsetOf(ID attackedArgument, vector<ID> const& members)
{
	//Find the root node for the attacked argument	
	auto rootNode = rootNodes[attackedArgument];
	if (rootNode == nullptr)
		return false;
	
	//Initialize the helper
	nodesToProcess.clear();
	nodesToProcess.emplace_back(rootNode, 0);
		
	while (!nodesToProcess.empty())
	{
		auto [node, index] = nodesToProcess.back();
		nodesToProcess.pop_back();

		//We have found a subset
		if (node->isContained)
			return true;
		
		if(index == members.size())
			continue;;

		//Add the current node with incremented index. Case where the current element is not contained but there might still be a subset with the current element.
		nodesToProcess.emplace_back(node, index + 1);		
		node = node->children[members[index]];
		if (node != nullptr)		
			nodesToProcess.emplace_back(node, index + 1);  //The current element is contained. Thus we add it second so that we go depth first
	}

	return false;

}
