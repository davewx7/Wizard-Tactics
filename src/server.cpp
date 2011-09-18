#include <algorithm>
#include <iostream>
#include <string>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "foreach.hpp"
#include "server.hpp"

namespace
{
int xml_int(const TiXmlElement& el, const char* s)
{
	int res = 0;
	el.QueryIntAttribute(s, &res);
	return res;
}
}

using boost::asio::ip::tcp;

server::game_info::game_info() : game_state(new game)
{}

server::server(boost::asio::io_service& io_service)
  : acceptor_(io_service, tcp::endpoint(tcp::v4(), 17000))
{
	start_accept();
}

void server::start_accept()
{
	socket_ptr socket(new tcp::socket(acceptor_.io_service()));
	acceptor_.async_accept(*socket, boost::bind(&server::handle_accept, this, socket, boost::asio::placeholders::error));
}

void server::handle_accept(socket_ptr socket, const boost::system::error_code& error)
{
	if(error) {
		std::cerr << "ERROR IN ACCEPT\n";
		return;
	}

	std::cerr << "RECEIVED CONNECTION. Listening\n";

	start_receive(socket);

	start_accept();
}

void server::start_receive(socket_ptr socket)
{
	buffer_ptr buf(new boost::array<char, 1024>);
	socket->async_read_some(boost::asio::buffer(*buf), boost::bind(&server::handle_receive, this, socket, buf, _1, _2));
}

void server::handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes)
{
	if(e) {
		disconnect(socket);
		return;
	}

	handle_incoming_data(socket, &(*buf)[0], &(*buf)[0] + nbytes);

	start_receive(socket);
}

void server::handle_incoming_data(socket_ptr socket, const char* i1, const char* i2)
{
	socket_info& info = connections_[socket];
	const char* string_terminator = std::find(i1, i2, 0);
	info.partial_message.insert(info.partial_message.end(), i1, string_terminator);
	if(string_terminator != i2) {
		info.partial_message.push_back(0);
		handle_message(socket, info.partial_message);
		info.partial_message.clear();
		handle_incoming_data(socket, string_terminator+1, i2);
	}
}

void server::handle_message(socket_ptr socket, const std::vector<char>& msg)
{
	std::cerr << "MSG: " << &msg[0] << "\n";
	TiXmlDocument doc;
	doc.Parse(&msg[0]);
	if(doc.Error()) {
		std::cerr << "INVALID XML RECEIVED\n";
		return;
	}
	handle_message(socket, *doc.RootElement());
}

void server::handle_message(socket_ptr socket, const TiXmlElement& node)
{
	const std::string name = node.Value();

	socket_info& info = connections_[socket];
	if(name == "login") {
		info.nick = node.Attribute("name");
	} else if(name == "create_game") {
		game_info_ptr new_game(new game_info);
		const game_context context(new_game->game_state.get());
		new_game->clients.push_back(socket);
		new_game->game_state->add_player(info.nick);
		const int nbots = xml_int(node, "bots");
		for(int n = 0; n != nbots; ++n) {
			new_game->game_state->add_ai_player("bot");
		}

		client_info& cli_info = clients_[info.nick];
		
		cli_info.game = new_game;
		cli_info.nplayer = 0;
	} else if(name == "join_game") {
		std::cerr << "join_game...\n";
		client_info& cli_info = clients_[info.nick];
		for(std::map<std::string, client_info>::iterator i = clients_.begin();
		    i != clients_.end(); ++i) {
			if(i->second.game && i->second.game->game_state->started() == false) {
				const game_context context(i->second.game->game_state.get());
				i->second.game->clients.push_back(socket);
				i->second.game->game_state->add_player(info.nick);
				cli_info.nplayer = i->second.game->clients.size() - 1;
				cli_info.game = i->second.game;

				std::string join_game;
				join_game << node;
				join_game.resize(join_game.size()+1);
				join_game[join_game.size()-1] = 0;
				send_msg(i->second.game->clients.front(), join_game);
				break;
			}
		}
	} else {
		if(info.nick.empty()) {
			std::cerr << "USER WITH NO NICK SENT UNRECOGNIZED DATA\n";
			return;
		}

		client_info& cli_info = clients_[info.nick];
		if(cli_info.game) {
			std::cerr << "GOT MESSAGE FROM " << cli_info.nplayer << ": " << node << "\n";
			const game_context context(cli_info.game->game_state.get());
			cli_info.game->game_state->handle_message(cli_info.nplayer, node);

			std::vector<game::message> game_response;
			cli_info.game->game_state->swap_outgoing_messages(game_response);
			foreach(game::message& msg, game_response) {
				msg.contents.push_back(0);
				if(msg.recipients.empty()) {
					foreach(const socket_ptr& sock, cli_info.game->clients) {
						send_msg(sock, msg.contents);
					}
				} else {
					foreach(int player, msg.recipients) {
						ASSERT_LT(player, cli_info.game->clients.size());
						send_msg(cli_info.game->clients[player], msg.contents);
					}
				}
			}
		}
	}
}

void server::send_msg(socket_ptr socket, const std::string& msg)
{
	boost::asio::async_write(*socket, boost::asio::buffer(msg),
			                         boost::bind(&server::handle_send, this, socket, _1, _2));
	std::cerr << "SEND MSG: " << msg.c_str() << "\n";
}

void server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes)
{
	if(e) {
		disconnect(socket);
	}
}

void server::disconnect(socket_ptr socket)
{
	connections_.erase(socket);
}
