#pragma once

#include <chrono>
#include <deque>
#include <functional>

#include <boost/thread/future.hpp>

#include "Common_Networking.hpp"

#include "nlohmann/json.hpp"


template <>
struct std::hash<websocketpp::connection_hdl>
{
	std::size_t operator()(websocketpp::connection_hdl connection) const
	{
		return reinterpret_cast<size_t>(connection.lock().get());
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
	~ComputerInterface()
	{
		std::scoped_lock a{request_mutex};
		for (auto &expected_response : m_request_queue)
		{
			auto promise = std::get<2>(expected_response);
			promise->set_value(R"(
				{
					"error": "interface deconstructed"
				}
			)"_json);
		}
	}

	void send_stop()
	{
		constexpr size_t stop_send_count = 3;
		for (size_t i = 0; i < stop_send_count; i++)
		{
			m_endpoint.send(
			    m_connection,
			    make_stop().dump(),
			    websocketpp::frame::opcode::text);
		}
	}

	void close(
	    const websocketpp::close::status::value status,
	    const std::string &why)
	{
		m_endpoint.close(m_connection, status, why);
	}

	void recieve(server::message_ptr message)
	{
		auto json_response = nlohmann::json::parse(message->get_payload());
		std::cout << message->get_payload() << '\n';
		auto response_id = json_response.at("request_id").get<int32_t>();
		if (m_requests.find(response_id) == m_requests.end())
		{
			if (m_unexpected_message_handler[response_id])
			{
				m_unexpected_message_handler[response_id](
				    *this,
				    json_response["response"]);
			}
			else
			{
				std::cout << "recieved unexpected message from computer "
				          << hash(m_connection)
				          << " with no registered handler\n";
			}
			return;
		}
		m_requests.at(response_id)->set_value(json_response["response"]);
		m_requests.erase(response_id);
	}

	void set_unexpected_message_handler(
	    std::function<void(ComputerInterface &, nlohmann::json)> function,
	    int32_t id)
	{
		m_unexpected_message_handler[id] = function;
	}

	void scheduler_internal(std::chrono::duration<double> dt)
	{
		std::scoped_lock a{request_mutex};
		constexpr std::chrono::duration<double> threshold{0.1};
		if (!m_request_queue.empty() && m_time_since_last_request > threshold)
		{
			m_endpoint.send(
			    m_connection,
			    std::get<1>(m_request_queue.front()).dump(),
			    websocketpp::frame::opcode::text);
			m_requests.emplace(
			    std::get<0>(m_request_queue.front()),
			    std::get<2>(m_request_queue.front()));
			m_request_queue.pop_front();
			m_time_since_last_request = std::chrono::duration<double>{0};
		}
		m_time_since_last_request += dt;
	}

	void auth_message(std::string auth)
	{
		std::scoped_lock a{request_mutex};
		auth_message_impl(auth);
	}
	boost::future<nlohmann::json> auth_message_future(std::string auth)
	{
		std::scoped_lock a{request_mutex};
		auth_message_impl(auth);
		return std::get<2>(m_request_queue.back())->get_future();
	}
	void remote_eval(std::string to_eval)
	{
		std::scoped_lock a{request_mutex};
		remote_eval_impl(to_eval);
	}
	boost::future<nlohmann::json> remote_eval_future(std::string to_eval)
	{
		std::scoped_lock a{request_mutex};
		remote_eval_impl(to_eval);
		return std::get<2>(m_request_queue.back())->get_future();
	}
	template <typename T>
	void execute_buffer(CommandBuffer<T> &buffer)
	{
		std::scoped_lock a{request_mutex};
		execute_buffer_impl<T>(buffer);
	}
	template <typename T>
	boost::future<T> execute_buffer_future(CommandBuffer<T> &buffer)
	{
		std::scoped_lock a{request_mutex};
		execute_buffer_impl<T>(buffer);
		return buffer.m_parse(
		    std::get<2>(m_request_queue.back())->get_future());
	}
	void inspect(std::string direction)
	{
		std::scoped_lock a{request_mutex};
		inspect_impl(direction);
	}
	boost::future<nlohmann::json> inspect_future(std::string direction)
	{
		std::scoped_lock a{request_mutex};
		inspect_impl(direction);
		return std::get<2>(m_request_queue.back())->get_future();
	}
	void rotate(std::string direction)
	{
		std::scoped_lock a{request_mutex};
		rotate_impl(direction);
	}
	boost::future<nlohmann::json> rotate_future(std::string direction)
	{
		std::scoped_lock a{request_mutex};
		rotate_impl(direction);
		return std::get<2>(m_request_queue.back())->get_future();
	}
	void move(std::string direction)
	{
		std::scoped_lock a{request_mutex};
		move_impl(direction);
	}
	boost::future<nlohmann::json> move_future(std::string direction)
	{
		std::scoped_lock a{request_mutex};
		move_impl(direction);
		return std::get<2>(m_request_queue.back())->get_future();
	}
	auto inventory(bool detailed = true)
	{
		std::scoped_lock a{request_mutex};
		inventory_impl(detailed);
	}
	auto inventory_future(bool detailed = true)
	{
		std::scoped_lock a{request_mutex};
		inventory_impl(detailed);
		return std::get<2>(m_request_queue.back())->get_future();
	}
	auto inventory_slot(int slot, bool detailed = true)
	{
		std::scoped_lock a{request_mutex};
		inventory_slot_impl(slot, detailed);
	}
	auto inventory_slot_future(int slot, bool detailed = true)
	{
		std::scoped_lock a{request_mutex};
		inventory_slot_impl(slot, detailed);
		return std::get<2>(m_request_queue.back())->get_future();
	}
	auto
	inventory_move(int from, int to, std::optional<int> amount = std::nullopt)
	{
		std::scoped_lock a{request_mutex};
		inventory_move_impl(from, to, amount);
	}
	auto inventory_move_future(
	    int from,
	    int to,
	    std::optional<int> amount = std::nullopt)
	{
		std::scoped_lock a{request_mutex};
		inventory_move_impl(from, to, amount);
		return std::get<2>(m_request_queue.back())->get_future();
	}
	auto drop_item(
	    int slot,
	    std::string direction,
	    std::optional<int> amount = std::nullopt)
	{
		std::scoped_lock a{request_mutex};
		drop_item_impl(slot, direction, amount);
	}
	auto drop_item_future(
	    int slot,
	    std::string direction,
	    std::optional<int> amount = std::nullopt)
	{
		std::scoped_lock a{request_mutex};
		drop_item_impl(slot, direction, amount);
		return std::get<2>(m_request_queue.back())->get_future();
	}

	private:
	void auth_message_impl(std::string auth)
	{
		auto request = make_auth(auth);
		submit_request(request);
	}

	void remote_eval_impl(std::string to_eval)
	{
		auto request = make_eval(to_eval);
		submit_request(request);
	}

	template <typename T>
	void execute_buffer_impl(CommandBuffer<T> &buffer);

	void inspect_impl(std::string direction)
	{
		auto request = make_inspect(direction);
		submit_request(request);
	}
	void rotate_impl(std::string direction)
	{
		auto request = make_rotate(direction);
		submit_request(request);
	}
	void move_impl(std::string direction)
	{
		auto request = make_move(direction);
		submit_request(request);
	}
	void inventory_impl(bool detailed = true)
	{
		auto request = make_inventory(detailed);
		submit_request(request);
	}
	void inventory_slot_impl(int slot, bool detailed = true)
	{
		auto request = make_inventory_slot(slot, detailed);
		submit_request(request);
	}
	void inventory_move_impl(
	    int from,
	    int to,
	    std::optional<int> amount = std::nullopt)
	{
		auto request = make_inventory_move(from, to, amount);
		submit_request(request);
	}
	void drop_item_impl(
	    int slot,
	    std::string direction,
	    std::optional<int> amount = std::nullopt)
	{
		auto request = make_drop_item(slot, direction, amount);
		submit_request(request);
	}

	void submit_request(nlohmann::json request)
	{
		auto request_id = add_request_id(request);
		m_request_queue.emplace_back(
		    request_id,
		    request,
		    std::make_shared<boost::promise<nlohmann::json>>());
	}

	static nlohmann::json make_auth(std::string auth)
	{
		auto request = R"(
			{
				"request_type": "authentication",
				"token": "a"
			}
		)"_json;
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
	static nlohmann::json make_inspect(std::string direction)
	{
		auto inspect = R"(
			{
				"request_type": "inspect",
				"request_id": "-1",
				"direction": ""
			}
		)"_json;
		inspect.at("direction") = direction;
		return inspect;
	}
	static nlohmann::json make_rotate(std::string direction)
	{
		auto rotate = R"(
			{
				"request_type": "rotate",
				"request_id": -1,
				"direction": ""
			}
		)"_json;
		rotate.at("direction") = direction;
		return rotate;
	}
	static nlohmann::json make_move(std::string direction)
	{
		auto move = R"(
			{
				"request_type": "move",
				"request_id": -1,
				"direction": ""
			}
		)"_json;
		move.at("direction") = direction;
		return move;
	}
	static nlohmann::json make_inventory(bool detailed = true)
	{
		auto inventory = R"(
			{
				"request_type": "inventory",
				"request_id": -1,
				"detailed": true
			}
		)"_json;
		inventory.at("detailed") = detailed;
		return inventory;
	}
	static nlohmann::json make_inventory_slot(int slot, bool detailed = true)
	{
		auto inventory_slot = R"(
			{
				"request_type": "inventory_slot",
				"request_id": -1,
				"slot": 0,
				"detailed": true
			}
		)"_json;
		inventory_slot.at("slot") = slot;
		inventory_slot.at("detailed") = detailed;
		return inventory_slot;
	}
	static nlohmann::json make_inventory_move(
	    int from,
	    int to,
	    std::optional<int> amount = std::nullopt)
	{
		auto inventory_move = R"(
			{
				"request_type": "inventory_move",
				"request_id": -1,
				"from": 0,
				"to": 0
			}
		)"_json;
		inventory_move.at("from") = from;
		inventory_move.at("to") = to;
		if (amount)
		{
			inventory_move["amount"] = *amount;
		}
		return inventory_move;
	}
	static nlohmann::json make_drop_item(
	    int slot,
	    std::string direction,
	    std::optional<int> amount = std::nullopt)
	{
		auto drop_item = R"(
			{
				"request_type": "drop_item",
				"request_id": -1,
				"slot": 0,
				"direction": "forwards"
			}
		)"_json;
		drop_item.at("slot") = slot;
		drop_item.at("direction") = direction;
		if (amount)
		{
			drop_item["amount"] = *amount;
		}
		return drop_item;
	}

	size_t add_request_id(nlohmann::json &request)
	{
		request["request_id"] = get_id();
		return request.at("request_id");
	}

	size_t get_id() { return m_request_id_counter++; }
	size_t m_request_id_counter = 1;
	websocketpp::connection_hdl m_connection;
	server &m_endpoint;
	std::deque<std::tuple<
	    size_t,
	    nlohmann::json,
	    std::shared_ptr<boost::promise<nlohmann::json>>>>
	    m_request_queue;
	std::unordered_map<size_t, std::shared_ptr<boost::promise<nlohmann::json>>>
	    m_requests;
	std::map<int32_t, std::function<void(ComputerInterface &, nlohmann::json)>>
	    m_unexpected_message_handler;

	std::chrono::duration<double> m_time_since_last_request{1e100};
	std::mutex request_mutex;

	template <typename T>
	friend class CommandBuffer;
};

template <typename T>
class CommandBuffer
{
	public:
	constexpr CommandBuffer()
	{
		m_commands = R"(
			{
				"commands": [],
				"shutdown": false
			}
		)"_json;
		if constexpr (std::is_same_v<T, nlohmann::json>)
		{
			SetDefaultParser();
		}
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
	template <typename Q>
	void buffer(CommandBuffer<Q> &m)
	{
		CheckShutdown();
		m_commands.at("commands")
		    .push_back(ComputerInterface::make_command_buffer_json(m));
	}
	void inspect(std::string direction)
	{
		CheckShutdown();
		m_commands.at("commands")
		    .push_back(ComputerInterface::make_inspect(direction));
	}
	void rotate(std::string direction)
	{
		CheckShutdown();
		m_commands.at("commands")
		    .push_back(ComputerInterface::make_rotate(direction));
	}
	void move(std::string direction)
	{
		CheckShutdown();
		m_commands.at("commands")
		    .push_back(ComputerInterface::make_move(direction));
	}
	void inventory(bool detailed = true)
	{
		CheckShutdown();
		m_commands.at("commands")
		    .push_back(ComputerInterface::make_inventory(detailed));
	}
	void inventory_slot(int slot, bool detailed = true)
	{
		CheckShutdown();
		m_commands.at("commands")
		    .push_back(ComputerInterface::make_inventory_slot(slot, detailed));
	}
	void
	inventory_move(int from, int to, std::optional<int> amount = std::nullopt)
	{
		CheckShutdown();
		m_commands.at("commands")
		    .push_back(
		        ComputerInterface::make_inventory_move(from, to, amount));
	}
	void drop_item(
	    int slot,
	    std::string direction,
	    std::optional<int> amount = std::nullopt)
	{
		CheckShutdown();
		m_commands.at("commands")
		    .push_back(
		        ComputerInterface::make_drop_item(slot, direction, amount));
	}
	constexpr void stop()
	{
		m_commands.at("commands").push_back(ComputerInterface::make_stop());
		m_commands["shutdown"] = true;
	}

	constexpr void SupplyOutputParser(std::function<T(nlohmann::json)> m)
	{
		m_output_parser = m;
	}
	constexpr void SetDefaultParser()
	{
		m_output_parser = [](nlohmann::json m) -> nlohmann::json { return m; };
	}

	private:
	boost::future<T> m_parse(boost::future<nlohmann::json> &&future)
	{
		return future.then([this](auto future)
		{
			return m_output_parser(future.get());
		});
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
void ComputerInterface::execute_buffer_impl(CommandBuffer<T> &buffer)
{
	auto request = make_command_buffer_json(buffer);
	auto request_id = add_request_id(request);
	m_request_queue.emplace_back(
	    request_id,
	    request,
	    std::make_shared<boost::promise<nlohmann::json>>());
}
