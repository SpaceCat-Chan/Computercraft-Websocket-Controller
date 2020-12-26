#include "Computer.hpp"

size_t hash(websocketpp::connection_hdl c)
{
	std::hash<websocketpp::connection_hdl> a;
	return a(c);
}
