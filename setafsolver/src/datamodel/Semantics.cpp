#include <string>
#include "../../header/datamodel/Semantics.hpp"
using namespace std;


Semantics::Semantics(SemanticsType const& type) : type(type) {};
Semantics::Semantics() : Semantics(stable) {};


Semantics::SemanticsType const& Semantics::getType() const
{
	return type;
}

optional<Semantics> Semantics::tryParse(char* const& s)
{
	static string semanticsTypeStableString = "Stable";

	if (!semanticsTypeStableString.compare(s))
		return Semantics(stable);

	return {};
}

