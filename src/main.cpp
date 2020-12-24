#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "nlohmann/json.hpp"

#include <functional>
#include <future>
#include <unordered_map>
#include <memory>

using server = websocketpp::server<websocketpp::config::asio>;

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

size_t hash(websocketpp::connection_hdl c)
{
	std::hash<websocketpp::connection_hdl> a;
	return a(c);
}

class ComputerInterface
{
	public:
	ComputerInterface(websocketpp::connection_hdl connection, server &endpoint)
	    : m_connection(connection), m_endpoint(endpoint)
	{
	}

	std::future<nlohmann::json> auth_message(std::string auth)
	{
		auto boot_json = R"(
			{
				"computer-id": 1,
				"request-id": 1,
				"request-type": "authentication",
				"token": "a"
			}
		)"_json;
		std::hash<websocketpp::connection_hdl> hash;
		boot_json["computer-id"] = hash(m_connection);
		boot_json["request-id"] = 1;
		boot_json["token"] = auth;
		m_endpoint.send(
		    m_connection,
		    boot_json.dump(),
		    websocketpp::frame::opcode::text);

		m_requests.emplace(1, std::make_shared<std::promise<nlohmann::json>>());
		return m_requests.at(1)->get_future();
	}

	void send_stop()
	{
		auto stop = R"(
			{
				"request-type": "graceful close",
				"request-id": -1
			}
		)"_json;
		m_endpoint.send(m_connection, stop.dump(), websocketpp::frame::opcode::text);
		m_endpoint.close(m_connection, websocketpp::close::status::normal, "requested by server");
	}

	void recieve(server::message_ptr message)
	{
		auto json_response = nlohmann::json::parse(message->get_payload());
		if(json_response.find("request-id") == json_response.end() || m_requests.find(json_response["request-id"].get<size_t>()) == m_requests.end())
		{
			if(m_unexpected_message_handler)
			{
				m_unexpected_message_handler(*this, json_response);
			}
			else
			{
				std::cout << "revieved unexpected message from computer " << hash(m_connection) << " with no registered handler\n";
			}
			return;
		}
		m_requests.at(json_response["request-id"].get<size_t>())
		    ->set_value(json_response["response"]);
		m_requests.erase(json_response["request-id"].get<size_t>());
	}

	private:
	websocketpp::connection_hdl m_connection;
	server &m_endpoint;
	std::unordered_map<size_t, std::shared_ptr<std::promise<nlohmann::json>>> m_requests;
	std::function<void(ComputerInterface&, nlohmann::json)> m_unexpected_message_handler;
};


class server_manager
{
	public:
	server_manager()
	{
		m_endpoint.set_error_channels(websocketpp::log::elevel::all);
		m_endpoint.set_access_channels(
		    websocketpp::log::alevel::all
		    /*^ websocketpp::log::alevel::frame_payload*/);

		m_endpoint.init_asio();

		m_endpoint.set_message_handler(std::bind(
		    &server_manager::echo_handler,
		    this,
		    std::placeholders::_1,
		    std::placeholders::_2));

		m_endpoint.set_open_handler(std::bind(
		    &server_manager::new_handler,
		    this,
		    std::placeholders::_1));
	}

	void run()
	{
		m_endpoint.listen(80);
		m_endpoint.start_accept();
		m_endpoint.run();
	}

	void stop()
	{
		for(auto m_connection = m_computers.begin(); m_connection != m_computers.end();)
		{
			auto interface = *m_connection;
			m_computers.erase(m_connection);
			interface.second.send_stop();
		}
		m_endpoint.stop();
	}

	private:
	void echo_handler(
	    websocketpp::connection_hdl connection,
	    server::message_ptr msg)
	{
		if(m_computers.find(connection) != m_computers.end())
		{
			m_computers.at(connection).recieve(msg);
		}
		else
		{
			//was probably in the middle of closing
		}
	}

	void new_handler(websocketpp::connection_hdl connection)
	{
		m_computers.emplace(
		    std::piecewise_construct,
		    std::tuple<websocketpp::connection_hdl>(connection),
		    std::tuple<websocketpp::connection_hdl, server &>{
		        connection,
		        m_endpoint});
		
		m_computers.at(connection).auth_message("am cat, aaa");
	}

	void close_handler(websocketpp::connection_hdl connection)
	{
		m_computers.erase(connection);
	}

	server m_endpoint;
	std::unordered_map<
	    websocketpp::connection_hdl,
	    ComputerInterface,
	    std::hash<websocketpp::connection_hdl>,
	    connection_equal>
	    m_computers;
};

int main()
{
	server_manager s;

	auto run_result = std::async(std::bind(&server_manager::run, &s));

	bool stop = false;
	while(!stop)
	{
		std::string line;
		std::getline(std::cin, line);
		if(line == "stop")
		{
			stop = true;
		}
	}
	s.stop();
}