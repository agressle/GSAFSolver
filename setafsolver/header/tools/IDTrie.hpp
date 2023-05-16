#pragma once

#include <unordered_map>
#include <vector>
#include <deque>

#include "../datamodel/Misc.hpp"

using namespace std;

class IDTrie
{
	private:
		class Node
		{
			public:
				
				/**
				 * A flag indicating whether the path ending at this node forms a contained set or not
				 */
				bool isContained = false;

				/**
				 * The paths starting from this node
				 */
				unordered_map<ID, Node*> children;							
		};

		/**
		 * The root nodes for each attacked arguments
		 */
		unordered_map<ID, Node*> rootNodes;

		/**
		 * The added nodes
		 */
		deque<Node> nodeStash;

		/**
		 * Helper for the conainsSubsetOf function. Stores nodes that need to be process next with the respective index in the size.
		 */
		deque<pair<Node*, size_t>> nodesToProcess;		
		
	public:

		/**
		 * Inserts the provided attack into the trie
		 */
		void insert(ID attackedArgument, vector<ID> const& members);

		/**
		 * Returns true iff a subset of members for the given attacked argument exists
		 */
		bool containsSubsetOf(ID attackedArgument, vector<ID> const& members);	
};