#ifndef SERVER_HPP_INCLUDED
#define SERVER_HPP_INCLUDED

#include "game.hpp"
#include "simple_wml.hpp"

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
private:
	void start_accept();
	void handle_accept(socket_ptr socket, const boost::system::error_code& error);
	void start_receive(socket_ptr socket);
	void handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes);
	void handle_incoming_data(socket_ptr socket, const char* i1, const char* i2);
	void handle_message(socket_ptr socket, const std::vector<char>& msg);
	void handle_message(socket_ptr socket, simple_wml::document& msg);

	void send_msg(socket_ptr socket, const std::string& msg);
	void handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes);

	void disconnect(socket_ptr socket);

	boost::asio::ip::tcp::acceptor acceptor_;

	struct game_info {
		game_info();
		boost::intrusive_ptr<game> game_state;
		std::vector<socket_ptr> clients;
	};

	typedef boost::shared_ptr<game_info> game_info_ptr;

	struct client_info {
		std::vector<char> partial_message;
		std::string nick;
		game_info_ptr game;
		int nplayer;
	};

	std::map<socket_ptr, client_info> clients_;
	std::vector<game_info_ptr> games_;
};

#endif
