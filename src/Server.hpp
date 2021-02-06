#pragma once

#include <memory>
#include <unordered_map>

#include "Common_Networking.hpp"

#include "Computer.hpp"

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

		m_endpoint.set_close_handler(std::bind(
		    &server_manager::close_handler,
		    this,
		    std::placeholders::_1));

		m_endpoint.set_reuse_addr(true);
	}

	void run()
	{
		m_endpoint.listen(8080);
		m_endpoint.start_accept();
		m_endpoint.run();
	}

	void send_stops()
	{
		for (auto m_connection = m_computers.begin();
		     m_connection != m_computers.end();)
		{
			auto interface = *m_connection;
			m_connection = m_computers.erase(m_connection);
			interface.second->send_stop();
		}
	}

	bool scheduler_stop = false;
	void scheduler()
	{
		auto start = std::chrono::steady_clock::now();
		while (!scheduler_stop)
		{
			std::this_thread::sleep_for(std::chrono::seconds{0});
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> dt = now - start;
			for (auto &computer : m_computers)
			{
				computer.second->scheduler_internal(dt);
			}
			start = now;
		}
	}

	void stop()
	{
		m_endpoint.stop();
		m_computers.clear();
	}

	void register_new_handler(
	    std::function<void(std::shared_ptr<ComputerInterface>)> new_handler)
	{
		m_new_handler = new_handler;
	}

	std::unordered_map<
	    websocketpp::connection_hdl,
	    std::shared_ptr<ComputerInterface>,
	    std::hash<websocketpp::connection_hdl>,
	    connection_equal>
	    m_computers;

	server m_endpoint;

	private:
	void echo_handler(
	    websocketpp::connection_hdl connection,
	    server::message_ptr msg)
	{
		if (m_computers.find(connection) != m_computers.end())
		{
			m_computers.at(connection)->recieve(msg);
		}
		else
		{
			// was probably in the middle of closing
		}
	}

	void new_handler(websocketpp::connection_hdl connection)
	{
		m_computers.emplace(
		    connection,
		    std::make_shared<ComputerInterface>(connection, m_endpoint));
		std::cout << "computer connected\n";
		if (m_new_handler)
		{
			m_new_handler(m_computers.at(connection));
		}
	}

	void close_handler(websocketpp::connection_hdl connection)
	{
		m_computers.erase(connection);
	}

	std::function<void(std::shared_ptr<ComputerInterface>)> m_new_handler;
};
