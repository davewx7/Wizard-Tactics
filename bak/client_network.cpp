#include <boost/asio.hpp>

#include "client_network.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_writer.hpp"

namespace network
{
using boost::asio::ip::tcp;

namespace {
tcp::socket* socket;
boost::asio::io_service* io_service;

bool tcp_packet_waiting(tcp::socket& tcp_socket)
{
	boost::asio::socket_base::bytes_readable command(true);
	tcp_socket.io_control(command);
	return command.get() != 0;
}
}

network::manager::manager()
{
	io_service = new boost::asio::io_service;
}

network::manager::~manager()
{
	delete socket;
	delete io_service;
	io_service = 0;
}

void connect(const std::string& hostname, const std::string& port)
{
	tcp::resolver resolver(*io_service);
	tcp::resolver::query query(hostname.c_str(), port.c_str());

	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	socket = new tcp::socket(*io_service);
	boost::system::error_code error = boost::asio::error::host_not_found;
	while(error && endpoint_iterator != end) {
		socket->close();
		socket->connect(*endpoint_iterator++, error);
	}

	if(error) {
		throw network_error();
	}
}

void send(const std::string& msg)
{
	std::vector<char> buf(msg.begin(), msg.end());
	buf.push_back(0);

	boost::system::error_code error = boost::asio::error::host_not_found;
	socket->write_some(boost::asio::buffer(buf), error);
	if(error) {
		throw network_error();
	}
}

void send(wml::const_node_ptr node)
{
	std::string str;
	wml::write(node, str);
	send(str);
}

namespace {
std::vector<char> current_message;
}

wml::const_node_ptr receive()
{
	size_t nbytes;
	while(tcp_packet_waiting(*socket)) {
		boost::array<char, 4096> buf;
		int nbytes = socket->read_some(boost::asio::buffer(buf));
		if(nbytes <= 0) {
			throw network_error();
		}

		current_message.insert(current_message.end(), buf.begin(), buf.begin() + nbytes);
	}

	std::vector<char>::iterator end = std::find(current_message.begin(), current_message.end(), 0);
	if(end != current_message.end()) {
		std::string msg(current_message.begin(), end);
		current_message.erase(current_message.begin(), end+1);
		return wml::parse_wml(msg);
	}

	return wml::const_node_ptr();
}

}

