#include "../../header/parsing/Parser.hpp"
#include "../../header/tools/Helper.hpp"
#include "../../header/datamodel/Misc.hpp"
#include "../../header/datamodel/Argument.hpp"
#include "../../header/datamodel/Clause.hpp"
#include "../../header/datamodel/Instance.hpp"
#include "../../header/tools/IDTrie.hpp"

#include <vector>
#include <stdexcept>
#include <iterator>
#include <fstream>
#include <sstream>
#include <optional>
#include <fmt/core.h>
#include <cassert>
#include <algorithm>
#include <tuple>


using namespace std;

class ParserSimpleFormat : public Parser
{
private:

	/**
	 * Used as base class for other simple format readers
	 */
	class SimpleFormatReader
	{
	private:
		//The file to read from
		ifstream file;

	protected:

		//The path of the file
		const string path;

		/**
		 * Creates a new instance and opens the specified file
		 */
		SimpleFormatReader(char const* const path)
			try : file(path, ifstream::in), path(path)
		{}
		catch (exception const& ex)
		{
			throw Parser::ParserException(fmt::format("Failed to open file {}: {}", path, ex.what()));
		}
		catch (...)
		{
			throw Parser::ParserException(fmt::format("Failed to open file {}", path));
		}

		/**
		 * {@return the next line or none if no further line exists}
		 */
		optional<string> getNextEntry()
		{
			string line;

			while (true)
			{
				try
				{
					if (!std::getline(file, line))
						return {};
				}
				catch (exception const& ex)
				{
					throw Parser::ParserException(fmt::format("Failed reading from file {}: {}", path, ex.what()));
				}
				catch (...)
				{
					throw Parser::ParserException(fmt::format("Failed reading from file {}", path));
				}

				if (line.empty())
					continue;

				if (line.back() == '\r')
				{
					line.pop_back();
					if (line.empty())
						continue;
				}

				if (line[0] == '#')
					continue;

				return line;
			}
		}
	};

	/**
	 * Encapsulates the parsing of the input file based on the simple format, i.e. integers separated by one empty space and ending with 0
	 */
	class SimpleFormatInstanceReader : SimpleFormatReader
	{
	public:
		/**
		* Creates a new instance and opens the specified file
		*/
		SimpleFormatInstanceReader(char const* const path) : SimpleFormatReader(path) {};

		/**
		* {@ the values of the next line (without the trailing 0) or none if no next line existed
		*/
		optional<vector<ID>> getNextEntry()
		{
			if (auto read = SimpleFormatReader::getNextEntry())
			{
				auto& line = *read;
				vector<ID> vec;
				stringstream stream(line);
				string word;
				while (stream >> word)
				{
					auto value = Helper::tryParseULong(word);

					if (!value)
						throw Parser::ParserException(fmt::format("The line '{}' in file '{}' is malformed.", line, path));

					vec.push_back(*value);
				}

				if (vec.size() < 2)
					throw Parser::ParserException(fmt::format("The line '{}' in file '{}' contains no values.", line, path));

				if (vec.back() != 0)
					throw Parser::ParserException(fmt::format("The line '{}' in file '{}' does not end with 0.", line, path));

				//Remove the trailing 0
				vec.pop_back();
				return vec;
			}
			return {};
		}
	};

	/**
	 * Encapsulates the parsing of the description file based on the simple format
	 */
	class SimpleFormatDescriptionReader : SimpleFormatReader
	{
	public:
		/**
		* Creates a new instance and opens the specified file
		*/
		SimpleFormatDescriptionReader(char const* const path) : SimpleFormatReader(path) {};

		/**
		* {@return a pair consisting of the ID of the argument and the rest of the line}
		*/
		optional<pair<ID, string>> getNextEntry()
		{
			if (auto read = SimpleFormatReader::getNextEntry())
			{
				auto& line = *read;
				auto firstBlankIndex = line.find(" ");
				if (firstBlankIndex == string::npos || firstBlankIndex == line.length())
					throw Parser::ParserException(fmt::format("The line '{}' in file '{}' does not contain a name", line, path));

				auto number = Helper::tryParseULong(line.substr(0, firstBlankIndex));
				if (!number)
					throw Parser::ParserException(fmt::format("The line '{}' in file '{}' does not contain a valid argument id", line, path));

				//Remove id part
				line.erase(0, firstBlankIndex + 1);
				return pair(*number, line);
			}
			return {};
		}
	};

	/**
	 * Encapsulates the parsing of the required arguments file based on the simple format
	 */
	class SimpleFormatRequiredArgumentsReader : SimpleFormatReader
	{
	private:
		/**
		 * The instance from which arguments are fetched
		 */
		Instance& instance;
		/**
		 * The mapping from argument name to arguments
		 */
		unordered_map<string, optional<Argument*>> const& argumentNameToArgumentMapping;


	public:
		/**
		* Creates a new instance and opens the specified file
		*/
		SimpleFormatRequiredArgumentsReader(char const* const path, Instance& instance, unordered_map<string, optional<Argument*>> const& argumentNameToArgumentMapping)
			: SimpleFormatReader(path), instance(instance), argumentNameToArgumentMapping(argumentNameToArgumentMapping) {};

		/**
		* {@return a pair consisting of the Argument and the Sign}
		*
		*/
		optional<pair<Argument*, Sign>> getNextEntry()
		{
			if (auto read = SimpleFormatReader::getNextEntry())
			{
				auto& line = *read;

				bool isPositive;
				auto firstBlankIndex = line.find(" ");
				if (firstBlankIndex == string::npos)
				{
					//Only the argument. We know that line is not empty
					isPositive = line[0] != '-';

					if (line.length() - (isPositive ? 0 : 1) == 0)
						throw Parser::ParserException(fmt::format("The line '{}' in file '{}' does not reference an argument", line, path));

					if (auto value = Helper::tryParseULong(line.substr(isPositive ? 0 : 1)))
					{
						auto& number = *value;
						if (number == 0 || number > instance.getNumberOfArguments())
							throw Parser::ParserException(fmt::format("The argument {} referenced in file '{}' does not exist", line, path));
						return pair(&instance.getArgument(number - 1), isPositive ? 1 : -1);
					}
					else
						throw Parser::ParserException(fmt::format("The argument {} referenced in file '{}' is malformed", line, path));
				}
				else
				{
					//The name of the argument							
					if (line.substr(0, firstBlankIndex) != "s" || line.length() < 3)
						throw Parser::ParserException(fmt::format("The line '{}' in file '{}' is malformed", line, path));

					isPositive = line[2] != '-';
					auto argumentName = line.substr(isPositive ? 2 : 3);
					auto entry = argumentNameToArgumentMapping.find(argumentName);
					if (entry == argumentNameToArgumentMapping.end())
						throw Parser::ParserException(fmt::format("The argument {} referenced in file '{}' does not exist", argumentName, path));

					auto& mapping = *entry;
					if (!mapping.second.has_value())
						throw Parser::ParserException(fmt::format("The argument {} referenced in file '{}' is not unqiue", argumentName, path));

					return pair(*mapping.second, isPositive ? 1 : -1);
				}
			}
			return {};
		}
	};

	char const* const instancePath;
	char const* const descriptionPath;
	char const* const requiredArgumentsPath;

public:

	/**
	 * Create a new instance.
	 * @param instancePath The path to the instance. Must not be null.
	 */
	ParserSimpleFormat(char const* const instancePath, char const* const descriptionPath, char const* const requiredArgumentsPath)
		: instancePath(instancePath), descriptionPath(descriptionPath), requiredArgumentsPath(requiredArgumentsPath)
	{};

	Instance getInstance() override
	{
		assert(instancePath != nullptr);

		//Read instance			
		ID numArguments, numAttacks;
		SimpleFormatInstanceReader instanceReader(instancePath);
		if (auto read = instanceReader.getNextEntry())
		{
			auto& preamble = *read;
			if (preamble.size() != 2)
				throw Parser::ParserException("The preamble is malformed");

			numArguments = preamble[0];
			numAttacks = preamble[1];
		}
		else
			throw Parser::ParserException("The supplied instance contains no preamble");

		//Buffer the attacks of the instance to be able to sort them first for efficient subsumption test. The bool indicates whether this attack is a superset of another
		vector<tuple<ID, vector<ID>, bool>> attackBuffer;

		ID attackCount = 0;
		for (; auto read = instanceReader.getNextEntry(); attackCount++)
		{
			Helper::throwExceptionIfReceivedSignal();

			if (attackCount == numAttacks)
				throw Parser::ParserException("The instance contains more attacks than specified in the preamble");

			auto& line = *read;

			if (line.size() < 2) //Attack contains no support
				throw Parser::ParserException(fmt::format("The attack {} is malformed", attackCount + 1));

			//Read the attacked argument
			if (line[0] == 0 || line[0] > numArguments)
				throw Parser::ParserException(fmt::format("The attack {} attacks argument {} that does not exist", attackCount + 1, line[0]));

			//Validate the attack members
			for (auto it = line.begin() + 1; it != line.end(); it++)
				if (*it == 0 || *it > numArguments)
					throw Parser::ParserException(fmt::format("The attack {} refereces argument {} that does not exist", attackCount + 1, *it));

			attackBuffer.emplace_back(line[0], vector<ID>{ line.begin() + 1, line.end() }, false);
		}
		if (attackCount != numAttacks)
			throw Parser::ParserException("The instance contains less attacks than specified in the preamble");
				
		//Sort the attacks by member count by. This ensures that a given attack cannot be a proper subset of a previous one
		sort(attackBuffer.begin(), attackBuffer.end(), [](auto a, auto b) {return get<1>(a).size() < get<1>(b).size(); });

		//Eliminate subsumed attacks
		IDTrie trie;
		size_t subsumedCount = 0;
		for (auto& [attackedArgument, members, isSubsumed] : attackBuffer)
		{			
			Helper::throwExceptionIfReceivedSignal();

			sort(members.begin(), members.end());

			if (trie.containsSubsetOf(attackedArgument, members))
			{
				isSubsumed = true;
				subsumedCount++;
			}
			else
				trie.insert(attackedArgument, members);
		}		

		//Create the instance
		Instance instance(numArguments, numAttacks - subsumedCount);
		vector<ID> argumentOccurenceWatch(numArguments, 0); //Used to make sure that every argument is only contained once in every clause			

		attackCount = 0;
		for (auto& [attackedArgumentNumber, members, isSubsumed] : attackBuffer)
		{
			Helper::throwExceptionIfReceivedSignal();

			if (isSubsumed)
				continue;

			auto& attack = instance.getAttack(attackCount);
			auto& attackedArgument = instance.getArgument(attackedArgumentNumber - 1);
			attack.setAttacked(attackedArgument, -1);

			for (auto member : members)
			{
				if (attackedArgumentNumber == member)
					attack.markAsSelfAttack();
				else
				{
					auto attackMemberID = member - 1;
					auto& argOccurence = argumentOccurenceWatch[attackMemberID];
					if (argOccurence < attackCount + 1) //First time this argument appears in this attack
					{
						auto& attackMember = instance.getArgument(attackMemberID);
						attack.addArgument(attackMember, -1);

						//Initialize the heuristics value of each argument to the number of attacks in occurs in to later be used for heuristics
						attackMember.setHeuristicsValue(attackMember.getHeuristicsValue() + 1.0);
						argOccurence++;
					}
				}
			}
			attackCount++;
		}

		unordered_map<string, optional<Argument*>> argumentNameToArgumentMapping; //A mapping from names to argument or none if the name occurs multiple times. This is relevant in the that the required argument file references this name

		//Read Description file
		if (descriptionPath != nullptr)
		{
			Helper::throwExceptionIfReceivedSignal();

			SimpleFormatDescriptionReader descriptionReader(descriptionPath);
			while (auto read = descriptionReader.getNextEntry())
			{
				auto& [id, name] = *read;
				if (id == 0 || id > numArguments)
					throw Parser::ParserException(fmt::format("The description file references argument {} that does not exist", id));

				auto& argument = instance.getArgument(id - 1);
				auto entry = argumentNameToArgumentMapping.find(name);
				if (entry == argumentNameToArgumentMapping.end())
					argumentNameToArgumentMapping[name] = &argument;
				else
					(*entry).second.reset();
				argument.setName(name);
			}
		}

		//Read required arguments file
		if (requiredArgumentsPath != nullptr)
		{
			Helper::throwExceptionIfReceivedSignal();

			SimpleFormatRequiredArgumentsReader requiredArgumentsReader(requiredArgumentsPath, instance, argumentNameToArgumentMapping);
			while (auto read = requiredArgumentsReader.getNextEntry())
				instance.addRequiredArgument(*read->first, read->second);
		}


		return instance;
	}
};