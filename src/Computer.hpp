#pragma once

#include <future>
#include <functional>

#include "Common_Networking.hpp"

#include "nlohmann/json.hpp"

template <>
struct std::hash<websocketpp::connection_hdl>
{
	std::size_t operator()(websocketpp::connection_hdl connection) const
	{
		return reinterpret_cast<size_t>(
		    static_cast<std::shared_ptr<void>>(connection).get());
	}
};

struct connection_equal
{
	bool operator()(
	    websocketpp::connection_hdl connection_a,
	    websocketpp::connection_hdl connection_b) const
	{
		std::hash<websocketpp::connection_hdl> hash;
		return hash(connection_a) == hash(connection_b);
	}
};

size_t hash(websocketpp::connection_hdl c);

template <typename T = nlohmann::json>
class CommandBuffer;

class ComputerInterface
{
	public:
	ComputerInterface(websocketpp::connection_hdl connection, server &endpoint)
	    : m_connection(connection), m_endpoint(endpoint)
	{
	}

	std::future<nlohmann::json> auth_message(std::string auth)
	{
		auto request = make_auth(auth);
		auto request_id = add_request_id(request);
		m_endpoint.send(
		    m_connection,
		    request.dump(),
		    websocketpp::frame::opcode::text);

		m_requests.emplace(
		    request_id,
		    std::make_shared<std::promise<nlohmann::json>>());
		return m_requests.at(request_id)->get_future();
	}

	std::future<nlohmann::json> remote_eval(std::string to_eval)
	{
		auto request = make_eval(to_eval);
		auto request_id = add_request_id(request);
		m_endpoint.send(
		    m_connection,
		    request.dump(),
		    websocketpp::frame::opcode::text);
		m_requests.emplace(
		    request_id,
		    std::make_shared<std::promise<nlohmann::json>>());
		return m_requests.at(request_id)->get_future();
	}

	template <typename T>
	std::future<T> execute_buffer(CommandBuffer<T> &buffer);

	void send_stop()
	{
		m_endpoint.send(
		    m_connection,
		    make_stop().dump(),
		    websocketpp::frame::opcode::text);
	}

	void recieve(server::message_ptr message)
	{
		auto json_response = nlohmann::json::parse(message->get_payload());
		std::cout << message->get_payload() << '\n';
		if (json_response.find("request_id") == json_response.end()
		    || m_requests.find(json_response["request_id"].get<size_t>())
		           == m_requests.end())
		{
			if (m_unexpected_message_handler)
			{
				m_unexpected_message_handler(*this, json_response);
			}
			else
			{
				std::cout << "recieved unexpected message from computer "
				          << hash(m_connection)
				          << " with no registered handler\n";
			}
			return;
		}
		m_requests.at(json_response["request_id"].get<size_t>())
		    ->set_value(json_response["response"]);
		m_requests.erase(json_response["request_id"].get<size_t>());
	}

	private:
	static nlohmann::json make_auth(std::string auth)
	{
		auto request = R"(
			{
				"request_type": "authentication",
				"token": "a"
			}
		)"_json;
		std::hash<websocketpp::connection_hdl> hash;
		request["token"] = auth;
		return request;
	}
	static nlohmann::json make_eval(std::string to_eval)
	{
		auto request = R"(
			{
				"request_type": "eval",
				"to_eval": ""
			}
		)"_json;
		request.at("to_eval") = to_eval;
		return request;
	}
	static nlohmann::json make_stop()
	{
		auto stop = R"(
			{
				"request_type": "close",
				"request_id": -1
			}
		)"_json;
		return stop;
	}
	template <typename T>
	static nlohmann::json make_command_buffer_json(CommandBuffer<T> &);

	size_t add_request_id(nlohmann::json &request)
	{
		request["request_id"] = get_id();
		return request.at("request_id");
	}

	size_t get_id() { return m_request_id_counter++; }
	size_t m_request_id_counter = 1;
	websocketpp::connection_hdl m_connection;
	server &m_endpoint;
	std::unordered_map<size_t, std::shared_ptr<std::promise<nlohmann::json>>>
	    m_requests;
	std::function<void(ComputerInterface &, nlohmann::json)>
	    m_unexpected_message_handler;

	template <typename T>
	friend class CommandBuffer;
};

template <typename T>
class CommandBuffer
{
	public:
	CommandBuffer()
	{
		m_commands = R"(
			{
				"commands": [],
				"shutdown": false
			}
		)"_json;
	}

	void auth(std::string m)
	{
		CheckShutdown();
		m_commands.at("commands").push_back(ComputerInterface::make_auth(m));
	}
	void eval(std::string m)
	{
		CheckShutdown();
		m_commands.at("commands").push_back(ComputerInterface::make_eval(m));
	}
	void buffer(CommandBuffer& m)
	{
		CheckShutdown();
		m_commands.at("commands").push_back(ComputerInterface::make_command_buffer_json(m));
	}
	void stop()
	{
		m_commands.at("commands").push_back(ComputerInterface::make_stop());
		m_commands["shutdown"] = true;
	}

	void SupplyOutputParser(std::function<T(nlohmann::json)> m)
	{
		m_output_parser = m;
	}
	void SetDefaultParser()
	{
		m_output_parser = [](nlohmann::json &m) { return m; };
	}

	private:
	std::future<T> m_parse(std::future<nlohmann::json> &&future)
	{
		return std::async(
		    [parser = this->m_output_parser](std::future<nlohmann::json> future) {
			    future.wait();
			    return parser(future.get());
		    },
		    std::move(future));
	}
	void CheckShutdown()
	{
		if (m_commands.at("shutdown") == true)
		{
			std::cout << "attempt to add commands after a shutown, they won't "
			             "be executed\n";
		}
	}
	std::function<T(nlohmann::json)> m_output_parser;
	nlohmann::json m_commands;

	friend class ComputerInterface;
};

template <typename T>
nlohmann::json
ComputerInterface::make_command_buffer_json(CommandBuffer<T> &buffer)
{
	auto copy = buffer.m_commands;
	copy["request_type"] = "command buffer";
	return copy;
}

template <typename T>
std::future<T> ComputerInterface::execute_buffer(CommandBuffer<T> &buffer)
{
	auto request = make_command_buffer_json(buffer);
	auto request_id = add_request_id(request);
	m_endpoint.send(
	    m_connection,
	    request.dump(),
	    websocketpp::frame::opcode::text);
	m_requests.emplace(
	    request_id,
	    std::make_shared<std::promise<nlohmann::json>>());
	return buffer.m_parse(m_requests.at(request_id)->get_future());
}

