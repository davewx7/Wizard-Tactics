#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "filesystem.hpp"
#include "foreach.hpp"
#include "server.hpp"
#include "string_utils.hpp"
#include "web_server.hpp"

using boost::asio::ip::tcp;

web_server::web_server(boost::asio::io_service& io_service, server& serv)
  : acceptor_(io_service, tcp::endpoint(tcp::v4(), 17001)),
    server_(serv)
{
	start_accept();
}

void web_server::start_accept()
{
	socket_ptr socket(new tcp::socket(acceptor_.io_service()));
	acceptor_.async_accept(*socket, boost::bind(&web_server::handle_accept, this, socket, boost::asio::placeholders::error));

}

namespace {
int nconnections = 0;
}

void web_server::handle_accept(socket_ptr socket, const boost::system::error_code& error)
{
	if(error) {
		std::cerr << "ERROR IN ACCEPT\n";
		return;
	}

	std::cerr << "RECEIVED CONNECTION. Listening " << ++nconnections << "\n";

	start_receive(socket);

	start_accept();
}

void web_server::start_receive(socket_ptr socket)
{
	buffer_ptr buf(new boost::array<char, 1024>);
	socket->async_read_some(boost::asio::buffer(*buf), boost::bind(&web_server::handle_receive, this, socket, buf, _1, _2));
}

void web_server::handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes)
{
	if(e) {
		//TODO: handle error
		std::cerr << "SOCKET ERROR: " << e.message() << "\n";
		std::cerr << "CLOSESOCKA\n";
		disconnect(socket);
		return;
	}

	handle_incoming_data(socket, &(*buf)[0], &(*buf)[0] + nbytes);

//	start_receive(socket);
}

void web_server::handle_incoming_data(socket_ptr socket, const char* i1, const char* i2)
{
	handle_message(socket, std::string(i1, i2));
}

namespace {
struct Request {
	std::string path;
	std::map<std::string, std::string> args;
};

Request parse_request(const std::string& str) {
	std::string::const_iterator end_path = std::find(str.begin(), str.end(), '?');
	Request request;
	request.path.assign(str.begin(), end_path);

	std::cerr << "PATH: '" << request.path << "'\n";

	if(end_path != str.end()) {
		++end_path;

		std::vector<std::string> args = util::split(std::string(end_path, str.end()), '&');
		foreach(const std::string& a, args) {
			std::string::const_iterator equal_itor = std::find(a.begin(), a.end(), '=');
			if(equal_itor != a.end()) {
				const std::string name(a.begin(), equal_itor);
				const std::string value(equal_itor+1, a.end());
				request.args[name] = value;

				std::cerr << "ARG: " << name << " -> " << value << "\n";
			}
		}
	}

	return request;
}

std::map<std::string, std::string> parse_env(const std::string& str)
{
	std::map<std::string, std::string> env;
	std::vector<std::string> lines = util::split(str, '\n');
	foreach(const std::string& line, lines) {
		std::string::const_iterator colon = std::find(line.begin(), line.end(), ':');
		if(colon == line.end() || colon+1 == line.end()) {
			continue;
		}

		std::string key(line.begin(), colon);
		const std::string value(colon+2, line.end());

		std::transform(key.begin(), key.end(), key.begin(), tolower);
		env[key] = value;

		std::cerr << "PARM: " << key << " -> " << value << "\n";
	}

	return env;
}

}

void web_server::handle_message(socket_ptr socket, const std::string& msg)
{
	if(msg.size() < 16) {
		std::cerr << "CLOSESOCKB\n";
		disconnect(socket);
		return;
	}

	std::cerr << "MESSAGE RECEIVED: " << msg << "\n";

	if(std::equal(msg.begin(), msg.begin()+4, "GET ")) {
		std::string::const_iterator end_url = std::find(msg.begin()+4, msg.end(), ' ');
		if(end_url == msg.end()) {
		std::cerr << "CLOSESOCKC\n";
			disconnect(socket);
			return;
		}

		const std::string path(msg.begin()+4, end_url);
		Request request = parse_request(path);

		boost::algorithm::replace_first(request.path, "/dave/wizard-data/", "data/");
		boost::algorithm::replace_first(request.path, "/dave/wizard-images/", "alpha-images/");
		boost::algorithm::replace_first(request.path, "/dave/", "www/");

		std::cerr << "MESSAGE: (((" << msg.c_str() << ")))\n";

		std::string::const_iterator begin_env = std::find(msg.begin(), msg.end(), '\n');
		if(begin_env == msg.end()) {
			std::cerr << "COULD NOT FIND ENV\n";
			return;
		}

		++begin_env;

		std::map<std::string, std::string> env = parse_env(std::string(begin_env, msg.end()));

		if(request.path == "/" && request.args.count("user")) {
			std::string contents = sys::read_file("www/main_template.html");
			boost::algorithm::replace_first(contents, "__HOSTNAME__", env["host"]);
			boost::algorithm::replace_first(contents, "__USERID__", request.args["user"]);
			send_msg(socket, "text/html; charset=UTF-8", contents);

		} else if(!request.path.empty() && request.path[0] != '/') {
			std::string content_type = "text/html";
			std::string::iterator i = request.path.end();
			--i;
			while(i != request.path.begin() && *i != '.') {
				--i;
			}

			std::string end(i, request.path.end());
			if(end == ".js") {
				content_type = "application/javascript";
			} else if(end == ".png") {
				content_type = "image/png";
			} else if(end == ".xml") {
				content_type = "text/xml";
			}

			const std::string content = sys::read_file(request.path);
			if(!content.empty()) {
				send_msg(socket, content_type, content);
			} else {
				send_404(socket);
			}
		} else {
			send_404(socket);
		}
	} else if(std::equal(msg.begin(), msg.begin()+5, "POST ")) {
		const char* begin_msg = msg.c_str();
		const char* begin_xml = strstr(begin_msg, "\n<");
		
		std::vector<char> buf;
		if(!begin_xml) {
			//the actual post contents will be received with more data
			//we want to read the content length and work out how much data
			//to try to read.
			std::map<std::string, std::string> env = parse_env(msg);
			const int content_length = atoi(env["content-length"].c_str());
			buf.resize(content_length);
			const size_t nbytes = socket->read_some(boost::asio::buffer(buf));

			std::cerr << "POST READ " << nbytes << "/" << buf.size() << "\n";
			if(nbytes == buf.size() && !buf.empty()) {
				buf.push_back(0);
				std::cerr << "(((" << &buf[0] << ")))\n";
				begin_msg = &buf[0];
				begin_xml = strstr(begin_msg+1, "\n<");
			}

			if(!begin_xml) {
				return;
			}
		}

		const char* begin_user = begin_xml-1;
		while(begin_user != begin_msg && *begin_user != '\n') {
			--begin_user;
		}

		if(*begin_user == '\n') {
			++begin_user;
		}

		std::string user(begin_user, begin_xml);

		std::cerr << "USER: (" << user << ")\n";

		++begin_xml;

		std::vector<char> xml_msg(begin_xml, begin_xml + strlen(begin_xml));

		server_.adopt_ajax_socket(socket, user, xml_msg);
	} else {
		std::cerr << "CLOSESOCKE\n";
		disconnect(socket);
	}
}

void web_server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, size_t max_bytes)
{
	std::cerr << "SENT: " << nbytes << " / " << max_bytes << "\n";
		std::cerr << "CLOSESOCKF\n";
	if(nbytes == max_bytes) {
		disconnect(socket);
	}
}

void web_server::disconnect(socket_ptr socket)
{
	socket->close();
	--nconnections;
}

void web_server::send_msg(socket_ptr socket, const std::string& type, const std::string& msg)
{
	char buf[4096];
	sprintf(buf, "HTTP/1.1 200 OK\nDate: Tue, 20 Sep 2011 21:00:00 GMT\nConnection: close\nServer: Wizard/1.0\nAccept-Ranges: bytes\nContent-Type: %s\nContent-Length: %d\nLast-Modified: Tue, 20 Sep 2011 10:00:00 GMT\n\n", type.c_str(), msg.size());

	std::string str(buf);
	str += msg;

	boost::asio::async_write(*socket, boost::asio::buffer(str),
	                         boost::bind(&web_server::handle_send, this, socket, _1, _2, str.size()));
}

void web_server::send_404(socket_ptr socket)
{
	char buf[4096];
	sprintf(buf, "HTTP/1.1 404 NOT FOUND\nDate: Tue, 20 Sep 2011 21:00:00 GMT\nConnection: close\nServer: Wizard/1.0\nAccept-Ranges: bytes\n\n");
	std::string str(buf);
	std::cerr << "SENDING 404:" << str << "\n";
	boost::asio::async_write(*socket, boost::asio::buffer(str),
                boost::bind(&web_server::handle_send, this, socket, _1, _2, str.size()));

}
