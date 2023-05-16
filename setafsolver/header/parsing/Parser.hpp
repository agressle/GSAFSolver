#pragma once
#include "../datamodel/instance.hpp"
#include <stdexcept>

class Parser
{
	public:
		virtual ~Parser() = default;
		virtual Instance getInstance() = 0;

		class ParserException : public std::logic_error
		{
		public:
			ParserException(string const& message) : std::logic_error(message) {}
		};
};