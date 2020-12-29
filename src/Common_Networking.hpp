#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using server = websocketpp::server<websocketpp::config::asio>;
