#include "../header/main.hpp"
#include "../header/Solver.hpp"
#include "../header/tools/Helper.hpp"
#include "../header/datamodel/Instance.hpp"
#include "../header/datamodel/Heuristics.hpp"
#include "../header/datamodel/Semantics.hpp"
#include "../header/Parsing/Parser.hpp"
#include "./parsing/ParserSimpleFormat.cpp"

#include <chrono>
#include <unistd.h>
#include <getopt.h>
#include <filesystem>
#include <fmt/core.h>
#include <optional>
#include <sys/resource.h>


using namespace std;

/**
 * The exit code to be returned when the program arguments are invalid
 */
const int EXIT_CODE_ARGUMENTS = 1;

/**
 * The exit code to be returned when the program was interrupted by some signal other then alarm
 */

const int EXIT_CODE_SIGNALS = 2;

/**
 * The exit code to be returned when a problem occurred during parsing
 */
const int EXIT_CODE_PARSING = 4;

/**
 * The exit code to be returned when the program was interrupted by the timeout
 */
const int EXIT_CODE_TIMEOUT = 9;

/**
 * the exit code to be returned when something unexpected went wrong
 */
const int EXIT_CODE_UNEXPECTED = 20;

chrono::time_point<chrono::high_resolution_clock> startTime;
chrono::time_point<chrono::high_resolution_clock> firstModelTime;
double percentageSolved = 0;

unsigned long modelCount = 0;

int PrintSummary()
{
	chrono::time_point<chrono::high_resolution_clock> endTime = chrono::high_resolution_clock::now();
	struct rusage usageValues;
	if (getrusage(RUSAGE_SELF, &usageValues))
	{
		cout << "Failed to get the time usage" << endl;
		return EXIT_CODE_UNEXPECTED;
	}	

	switch (Helper::getSignalType())
	{
		case Helper::SignalType::Alarm:
			cout << "Interrupted by timeout" << endl;			
		break;
		case Helper::SignalType::Interrupt:
		case Helper::SignalType::Terminate:
			cout << "Interrupted by signal" << endl;		
		break;
		default:
			//No special message
			break;			
	}

	cout << "Finished." << endl << "Models found: " << modelCount << endl;
	if (modelCount != 0)
		printf("Runtime (s): %.3f (user: %.3f, system: %.3f, first Model: %.3f)\n", ((chrono::duration<double>)(endTime - startTime)).count(), (double)usageValues.ru_utime.tv_sec + (double)usageValues.ru_utime.tv_usec / (double)1000000, (double)usageValues.ru_stime.tv_sec + (double)usageValues.ru_stime.tv_usec / (double)1000000, ((chrono::duration<double>)(firstModelTime - startTime)).count());
	else
		printf("Runtime (s): %.3f (user: %.3f, system: %.3f)\n", ((chrono::duration<double>)(endTime - startTime)).count(), (double)usageValues.ru_utime.tv_sec + (double)usageValues.ru_utime.tv_usec / (double)1000000, (double)usageValues.ru_stime.tv_sec + (double)usageValues.ru_stime.tv_usec / (double)1000000);
	printf("Percentage solved: %.9f", percentageSolved * 100);
	return 0;
}

int parseAndSolve(int argc, char** argv)
{
	Semantics semantics;
	Heuristics heuristics;
	bool printModels = true;
	unsigned long numberOfModels = 0;
	double clauseLearningForgetPercentage = 0.5;
	double clauseLearningGrowthRate = 2;
	char* instancePath = nullptr, *descriptionPath = nullptr, *requiredArgumentsPath = nullptr, *proofPath = nullptr;;

	int c;	
	while ((c = getopt(argc, argv, "i:d:r:s:n:t:p:g:h:q:c:")) != -1)
	{
		Helper::throwExceptionIfReceivedSignal();

		switch (c)
		{
			case 'i':
				instancePath = optarg;
				if (!filesystem::exists(instancePath))
				{
					cout << "The supplied instance does not exist" << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;

			case 'd':
				descriptionPath = optarg;
				if (!filesystem::exists(descriptionPath))
				{
					cout << "The supplied description does not exist" << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;

			case 'r':
				requiredArgumentsPath = optarg;
				if (!filesystem::exists(requiredArgumentsPath))
				{
					cout << "The supplied required arguments does not exist" << endl; 
					return EXIT_CODE_ARGUMENTS;
				}
				break;

			case 's':
				if (auto parsedSemantics = Semantics::tryParse(optarg))
					semantics = *parsedSemantics;
				else
				{
					cout << "The supplied semantics is not valid" << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;

			case 'n':
				if (optional<unsigned long> parsedNumberOfModels = Helper::tryParseULong(string(optarg)))
					numberOfModels = *parsedNumberOfModels;
				else
				{
					cout << "The supplied number of models is invalid" << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;

			case 't':
			{
				auto timeout = Helper::tryParseUInt(optarg);
				if (timeout.has_value() && *timeout > 0)
					alarm(*timeout);
				else
				{
					cout << "The supplied timeout is invalid" << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;
			}

			case 'p':
			{
				auto percentage = Helper::tryParseDouble(string(optarg));
				if (*percentage >= 0 && *percentage <= 1)
					clauseLearningForgetPercentage = *percentage;
				else
				{
					cout << "The supplied clause learning forget percentage is invalid" << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;
			}

			case 'g':
			{
				auto rate = Helper::tryParseDouble(string(optarg));
				if (*rate >= 0)
					clauseLearningGrowthRate = *rate;
				else
				{
					cout << "The supplied clause learning growth rate is invalid" << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;
			}

			case 'q':
				printModels = false;
				break;

			case 'h':
				if (auto parsedHeuristics = Heuristics::tryParse(optarg))
					heuristics = *parsedHeuristics;
				else
				{
					cout << fmt::format("Unkown heuristics: {}", optarg) << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;
			case 'c':
				proofPath = optarg;
				if (filesystem::exists(proofPath))
				{
					cout << "The supplied proof file already exist" << endl;
					return EXIT_CODE_ARGUMENTS;
				}
				break;

			case '?':
				return EXIT_CODE_ARGUMENTS;
				
			default:
			{
				cout << "An unexpected error occurred while parsing the arguments" << endl;
				return EXIT_CODE_ARGUMENTS;
			}
		}
	}

	if (instancePath == nullptr)
	{
		cout << "No instance was provided" << endl;
		return EXIT_CODE_ARGUMENTS;
	}

	auto proofFile = std::make_unique<ofstream>(nullptr);
	if (proofPath != nullptr)
	{
		proofFile = std::make_unique<ofstream>(proofPath, ios::out | ios::binary);
		if (proofFile->fail())
		{
			cout << "Failed to open proof file" << endl;
			return EXIT_CODE_ARGUMENTS;
		}
	}

	auto parser = std::unique_ptr<Parser>{ nullptr };
	parser = std::make_unique<ParserSimpleFormat>(instancePath, descriptionPath, requiredArgumentsPath);	
	Instance instance = parser->getInstance();		
	
	Solver solver(instance, semantics, heuristics, firstModelTime, modelCount, percentageSolved, numberOfModels, printModels, clauseLearningForgetPercentage, clauseLearningGrowthRate, proofFile);
	solver.solve();

	if (proofPath != nullptr)
	{
		proofFile->close();
		if (modelCount != 0)
			std::remove(proofPath);
	}	

	return 0;
}

int main(int argc, char** argv)
{		
	int returnValue = 0;

	try
	{
		startTime = chrono::high_resolution_clock::now();
		Helper::registerSignalHandlers();
		returnValue = parseAndSolve(argc, argv);
	}
	catch (Parser::ParserException const& ex)
	{
		cout << ex.what() << endl;
		returnValue = EXIT_CODE_PARSING;		
	}
	catch (Helper::SignalReceivedException const&)
	{
		switch (Helper::getSignalType())
		{
			case Helper::SignalType::Alarm:				
				returnValue = EXIT_CODE_TIMEOUT;
				break;
			default:				
				returnValue = EXIT_CODE_SIGNALS;
				break;
		}	
	}
	catch (exception const& ex)
	{
		cout << fmt::format("An unexpected error occurred: {}", ex.what()) << endl;		
		returnValue = EXIT_CODE_UNEXPECTED;
	}	
	catch (...)
	{
		cout << "An unexpected error occurred" << endl;
		returnValue = EXIT_CODE_UNEXPECTED;
	}	

	PrintSummary();

	return returnValue;
}
