#include "../../header/tools/Helper.hpp"
#include <string>

using namespace std;

optional<unsigned short> Helper::tryParseUShort(string const& s)
{
	unsigned short value;
	int len;
	if (sscanf(s.c_str(), "%hu%n", &value, &len) != 1 || (len != (int)s.length()))
		return {};
	else
		return value;
}

optional<unsigned int> Helper::tryParseUInt(string const& s)
{
	unsigned int value;
	int len;
	if (sscanf(s.c_str(), "%u%n", &value, &len) != 1 || (len != (int)s.length()))
		return {};
	else
		return value;
}

optional<unsigned long> Helper::tryParseULong(string const& s)
{
	unsigned long value;
	int len;
	if (sscanf(s.c_str(), "%lu%n", &value, &len) != 1 || (len != (int)s.length()))
		return {};
	else
		return value;
}

optional<double> Helper::tryParseDouble(string const& s)
{
	double value;
	int len;
	if (sscanf(s.c_str(), "%lf%n", &value, &len) != 1 || (len != (int)s.length()))
		return {};
	else
		return value;
}

void Helper::signal_handler(int signal)
{
	switch (signal)
	{
		case SIGINT:
			signalStatus = (sig_atomic_t)SignalType::Interrupt;
			break;
		case SIGTERM:
			signalStatus = (sig_atomic_t)SignalType::Terminate;
			break;
		case SIGALRM:
			signalStatus = (sig_atomic_t)SignalType::Alarm;
			break;
		default:
			signalStatus = (sig_atomic_t)SignalType::Other;
			break;
	}
}

Helper::SignalType Helper::getSignalType()
{
	return (SignalType)signalStatus;
}

void Helper::registerSignalHandlers()
{
	if (getSignalType() == SignalType::NotInitialized)
	{
		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);
		signal(SIGALRM, signal_handler);
		signalStatus = (sig_atomic_t)SignalType::None;
	}
}

bool Helper::receivedSignal()
{
	return getSignalType() != SignalType::None;
}

void Helper::throwExceptionIfReceivedSignal()
{
	if (receivedSignal())
		throw SignalReceivedException("Signal has been received");
}
