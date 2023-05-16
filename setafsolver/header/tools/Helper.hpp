#pragma once
#include <optional>
#include <vector>
#include <cassert>
#include <algorithm>
#include <csignal>
#include <string>
#include <stdexcept>

using namespace std;

class Helper
{
	public:

		/**
		* The signal type received
		*/
		enum class SignalType { NotInitialized = 0, None = 1, Other = 2, Interrupt = 3, Terminate = 4, Alarm = 5 };


		/**
		 * Thrown if a signal has been received and throwExceptionIfReceivedSignal has been called
		 */
		class SignalReceivedException : public logic_error
		{
			public:
				SignalReceivedException(string const& message) : std::logic_error(message) {}
		};

	private:		

		/**
		 * The last signal received or another status
		 */
		volatile inline static sig_atomic_t signalStatus = (sig_atomic_t)SignalType::NotInitialized;		

		/**
		* The signal handler
		*/
		static void signal_handler(int signal);

	public:
		/**
		* Tries to parse a string to an unsigned short
		*/
		static optional<unsigned short> tryParseUShort(string const& s);

		/**
		* Tries to parse a string to an unsigned int
		*/
		static optional<unsigned int> tryParseUInt(string const& s);

		/**
		 * Tries to parse a string to an unsigned long
		 */
		static optional<unsigned long> tryParseULong(string const& s);
			
		/**
		 * Tries to parse a string to a double
		 */
		static optional<double> tryParseDouble(string const& s);			

		/**
		 * Removes an element with the given index from the given vector without retaining order
		 */
		template<typename T, typename A>
		static void swapRemove(vector<T, A>& vector, size_t const& index)
		{
			assert(index < vector.size());
			if (index != vector.size() - 1)
				vector[index] = std::move(vector.back());
			vector.pop_back();
		}
		
		/**
		 * Gets the current signal type
		 */
		static SignalType getSignalType();
		
		/**
		 * Registers the signal handler. Does nothing if already registered
		 */
		static void registerSignalHandlers();

		/**
		 * {@return True iff no signal has been received or the signal handler has not been initialized}
		 */
		static bool receivedSignal();

		/**
		 * Throws SignalReceivedException if a signal has been received or the signal handler has not been initialized
		 */
		static void throwExceptionIfReceivedSignal();
		

};

