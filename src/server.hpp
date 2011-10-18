#ifndef SERVER_HPP_INCLUDED
#define SERVER_HPP_INCLUDED

#include "game.hpp"
#include "tinyxml.h"

#include <deque>
#include <map>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<boost::array<char, 1024> > buffer_ptr;

class server
{
public:
	explicit server(boost::asio::io_service& io_service);
	 
	void run();

	void adopt_ajax_socket(socket_ptr socket, const std::string& nick, const std::vector<char>& msg);
private:
	void start_accept();
	void handle_accept(socket_ptr socket, const boost::system::error_code& error);
	void start_receive(socket_ptr socket);
	void handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes);
	void handle_incoming_data(socket_ptr socket, const char* i1, const char* i2);
	void handle_message(socket_ptr socket, const std::vector<char>& msg);
	void handle_message(socket_ptr socket, const TiXmlElement& msg);
	void handle_message_internal(socket_ptr socket, const TiXmlElement& msg);

	void close_ajax(socket_ptr socket);

	void send_msg(socket_ptr socket, const std::string& msg);
	void handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes);

	void disconnect(socket_ptr socket);

	void heartbeat();

	boost::asio::ip::tcp::acceptor acceptor_;

	boost::asio::deadline_timer timer_;

	struct game_info {
		game_info();
		boost::intrusive_ptr<game> game_state;
		std::vector<std::string> clients;
	};

	typedef boost::shared_ptr<game_info> game_info_ptr;

	struct socket_info {
		socket_info() : ajax_connection(false) {}
		std::vector<char> partial_message;
		std::string nick;
		bool ajax_connection;
	};

	struct client_info {
		game_info_ptr game;
		int nplayer;

		std::deque<std::string> msg_queue;
	};

	void queue_msg(const std::string& nick, const std::string& msg);

	void create_lobby_msg(std::string& str);
	void send_lobby_msg();

	std::map<socket_ptr, std::string> waiting_connections_;

	std::map<socket_ptr, socket_info> connections_;
	std::map<std::string, client_info> clients_;
	std::vector<game_info_ptr> games_;

	int nheartbeat_;
};

#endif
