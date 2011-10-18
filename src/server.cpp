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
  : acceptor_(io_service, tcp::endpoint(tcp::v4(), 17000)),
    timer_(io_service), nheartbeat_(0)
{
	start_accept();

	heartbeat();
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
	handle_message_internal(socket, node);

	socket_info& info = connections_[socket];
	if(info.ajax_connection) {
		close_ajax(socket);
	}
}

void server::handle_message_internal(socket_ptr socket, const TiXmlElement& node)
{
	const std::string name = node.Value();
	std::cerr << "handle_message(" << name << ")\n";

	socket_info& info = connections_[socket];
	if(name == "close_ajax") {
		close_ajax(socket);
	} else if(name == "commands") {
		for(const TiXmlElement* t = node.FirstChildElement(); t != NULL; t = t->NextSiblingElement()) {
			handle_message_internal(socket, *t);
		}
	} else if(name == "login") {
		info.nick = node.Attribute("name");
	} else if(name == "enter_lobby") {
		foreach(game_info_ptr g, games_) {
			if(std::count(g->clients.begin(), g->clients.end(), info.nick)) {
				g->clients.erase(std::remove(g->clients.begin(), g->clients.end(), info.nick), g->clients.end());
			}
		}

		std::string msg;
		create_lobby_msg(msg);

		queue_msg(info.nick, msg);

	} else if(name == "create_game") {
		game_info_ptr new_game(new game_info);
		games_.push_back(new_game);
		const game_context context(new_game->game_state.get());
		new_game->clients.push_back(info.nick);
		new_game->game_state->add_player(info.nick);
		const int nbots = xml_int(node, "bots");
		for(int n = 0; n != nbots; ++n) {
			new_game->game_state->add_ai_player("bot");
		}

		client_info& cli_info = clients_[info.nick];
		
		cli_info.game = new_game;
		cli_info.nplayer = 0;

		send_lobby_msg();

		queue_msg(info.nick, "<game_created/>");
	} else if(name == "join_game") {
		std::cerr << "join_game...\n";
		client_info& cli_info = clients_[info.nick];
		for(std::map<std::string, client_info>::iterator i = clients_.begin();
		    i != clients_.end(); ++i) {
			if(i->second.game && i->second.game->game_state->started() == false) {
				const game_context context(i->second.game->game_state.get());
				i->second.game->clients.push_back(info.nick);
				i->second.game->game_state->add_player(info.nick);
				cli_info.nplayer = i->second.game->clients.size() - 1;
				cli_info.game = i->second.game;

				std::string join_game;
				join_game << node;
				queue_msg(i->second.game->clients.front(), join_game);
				std::cerr << "SENDING join_game TO " << i->second.game->clients.front() << "\n";
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
				if(msg.recipients.empty()) {
					foreach(const std::string& nick, cli_info.game->clients) {
						queue_msg(nick, msg.contents);
					}
				} else {
					foreach(int player, msg.recipients) {
						ASSERT_LT(player, cli_info.game->clients.size());
						queue_msg(cli_info.game->clients[player], msg.contents);
					}
				}
			}
		}
	}

}

void server::close_ajax(socket_ptr socket)
{
	socket_info& info = connections_[socket];

	client_info& cli_info = clients_[info.nick];

	if(cli_info.msg_queue.empty() == false) {

		for(std::map<socket_ptr,std::string>::iterator s = waiting_connections_.begin();
		    s != waiting_connections_.end(); ++s) {
			if(s->second == info.nick && s->first != socket) {
				send_msg(s->first, "<heartbeat/>");
				s->first->close();
				disconnect(s->first);
				break;
			}
		}

		send_msg(socket, cli_info.msg_queue.front());
		cli_info.msg_queue.pop_front();
		disconnect(socket);
		socket->close();
	} else {
		waiting_connections_[socket] = info.nick;
	}
}

void server::queue_msg(const std::string& nick, const std::string& msg)
{
	clients_[nick].msg_queue.push_back(msg);
}

void server::send_msg(socket_ptr socket, const std::string& msg)
{
	const socket_info& info = connections_[socket];
	std::string header;
	if(info.ajax_connection) {
		char buf[4096];
		sprintf(buf, "HTTP/1.1 200 OK\nDate: Tue, 20 Sep 2011 21:00:00 GMT\nConnection: close\nServer: Wizard/1.0\nAccept-Ranges: bytes\nContent-Type: text/xml\nContent-Length: %d\nLast-Modified: Tue, 20 Sep 2011 10:00:00 GMT\n\n", msg.size());
		header = buf;
	}
	boost::asio::async_write(*socket, boost::asio::buffer(header.empty() ? msg : (header + msg)),
			                         boost::bind(&server::handle_send, this, socket, _1, _2));
	std::cerr << "SEND MSG: " << (header + msg) << "\n";
}

void server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes)
{
	if(e) {
		disconnect(socket);
	}
}

void server::disconnect(socket_ptr socket)
{
	waiting_connections_.erase(socket);
	connections_.erase(socket);
}

void server::heartbeat()
{
	const bool send_heartbeat = nheartbeat_++%10 == 0;

	std::vector<socket_ptr> disconnect_sockets;

	for(std::map<socket_ptr, std::string>::iterator i = waiting_connections_.begin(); i != waiting_connections_.end(); ++i) {
		socket_ptr socket = i->first;

		socket_info& info = connections_[socket];
		client_info& cli_info = clients_[info.nick];
		if(cli_info.msg_queue.empty() == false) {
			send_msg(socket, cli_info.msg_queue.front());
			cli_info.msg_queue.pop_front();
			disconnect_sockets.push_back(socket);
		} else if(send_heartbeat) {
			send_msg(socket, "<heartbeat/>");
			disconnect_sockets.push_back(socket);
		}
	}

	foreach(const socket_ptr& socket, disconnect_sockets) {
		disconnect(socket);
		socket->close();
	}

	timer_.expires_from_now(boost::posix_time::seconds(1));
	timer_.async_wait(boost::bind(&server::heartbeat, this));
}

void server::adopt_ajax_socket(socket_ptr socket, const std::string& nick, const std::vector<char>& msg)
{
	socket_info& info = connections_[socket];
	info.ajax_connection = true;
	info.nick = nick;
	
	handle_message(socket, msg);
}

void server::create_lobby_msg(std::string& msg)
{
	msg += "<lobby>";
	foreach(game_info_ptr g, games_) {
		msg += "<game started=\"";
		msg += g->game_state->started() ? "yes" : "no";
		msg += "\" clients=\"";
		bool first_time = true;
		foreach(const std::string& client, g->clients) {
			if(!first_time) {
				msg += ",";
			} else {
				first_time = false;
			}
			msg += client;
		}

		msg += "\"/>";
	}
	msg += "</lobby>";
}

void server::send_lobby_msg()
{
	std::string msg;
	create_lobby_msg(msg);
	for(std::map<std::string, client_info>::const_iterator i = clients_.begin();
	    i != clients_.end(); ++i){
		queue_msg(i->first, msg);
	}
}
