#include "webserver.hh"
#include <boost/network/protocol/http/server.hpp>

namespace http = boost::network::http;

typedef http::server<WebServer::handler> http_server;

struct WebServer::handler {
	WebServer& m_server;
	void operator() (http_server::request const &request,
	http_server::response &response) {
	//destination describes the file to be loaded, by default it's "/"
		if(request.method == "GET") {
		response = m_server.GETresponse(request);
		} else if(request.method == "POST") {
		response = m_server.POSTresponse(request);
		} else {
		response = http_server::response::stock_reply(
			http_server::response::ok, "other request");
		}
	}

	void log(http_server::string_type const &info) {
		std::cerr << "ERROR: " << info << '\n';
	}
};


void WebServer::StartServer() {
	handler handler_{*this};
	http_server::options options(handler_);
	server_ = new http_server(options.address("0.0.0.0").port("8000"));
	server_->run();
}

WebServer::WebServer(Songs& songs):
m_songs(songs){
	serverthread.reset(new boost::thread(boost::bind(&WebServer::StartServer,boost::ref(*this))));
}

WebServer::~WebServer() {
	server_->stop();
	serverthread->join();
	delete server_;
}

http_server::response WebServer::GETresponse(const http_server::request &request) {
	if(request.destination == "/") { //default
		fs::ifstream f(findFile("index.html"), std::ios::binary);
		f.seekg(0, std::ios::end);
		size_t size = f.tellg();
		f.seekg(0);
		char responseBuffer[size + 2];
		f.read(responseBuffer, size);
		return http_server::response::stock_reply(
		http_server::response::ok, responseBuffer);
		} else if(request.destination == "/api/getDataBase.json") { //get database
		std:: stringstream JSONDB;
		JSONDB << "[\n";
		for(int i=0; i<m_songs.size(); i++) {
			JSONDB << "\n{\n\"Title\": \"" << m_songs[i].title << "\",\n\"Artist\": \"";
			JSONDB << m_songs[i].artist << "\",\nEdition\": \"" << m_songs[i].edition << "\",\n\"Language\": \"" << m_songs[i].language;
			JSONDB << "\",\n\"Creator\": \"" << m_songs[i].creator << "\"\n},";
		}
		std::string output = JSONDB.str(); //remove the last comma
		output.pop_back(); //remove the last comma
		output += "\n]";
		return http_server::response::stock_reply(
		http_server::response::ok, output);
	} else if(request.destination == "/api/getCurrentPlaylist.json") { //get playlist
	Game * gm = Game::getSingletonPtr();
	std:: stringstream JSONPlayList;
		JSONPlayList << "[\n";
		for(auto const& song : gm->getCurrentPlayList().getList()) {
			JSONPlayList << "\n{\n\"Title\": \"" << song->title << "\",\n\"Artist\": \"";
			JSONPlayList << song->artist << "\",\n\"Edition\": \"" << song->edition << "\",\n\"Language\": \"" << song->language;
			JSONPlayList << "\",\n\"Creator\": \"" << song->creator << "\"\n},";
		}
		std::string output = JSONPlayList.str();
		output.pop_back(); //remove the last comma
		output += "\n]";
		return http_server::response::stock_reply(
		http_server::response::ok,output);

	} else {
		//other text files
		try {
		std::string destination = request.destination;
		destination.erase(0,1);
		fs::ifstream f(findFile(destination), std::ios::binary);
		f.seekg(0, std::ios::end);
		size_t size = f.tellg();
		f.seekg(0);
		char responseBuffer[size + 2];
		f.read(responseBuffer, size);
		return http_server::response::stock_reply(
		http_server::response::ok, responseBuffer);
		}
		catch(std::exception e) {
		return http_server::response::stock_reply(
		http_server::response::ok, "not a text file");
		}
	}
}

http_server::response WebServer::POSTresponse(const http_server::request &request) {
	if(request.destination == "/api/add") {
		Game * gm = Game::getSingletonPtr();
		boost::shared_ptr<Song> pointer = GetSongFromJSON(request.body);
		if(pointer == NULL) {
			return http_server::response::stock_reply(
			http_server::response::ok, "failure");
		} else {
		std::cout << pointer->title << std::endl;
		gm->getCurrentPlayList().addSong(pointer);
		return http_server::response::stock_reply(
			http_server::response::ok, "success");
		}
	} else {
		return http_server::response::stock_reply(
		http_server::response::ok, "not yet implemented");
	}

}

boost::shared_ptr<Song> WebServer::GetSongFromJSON(std::string JsonDoc) {
std::cout << JsonDoc << std::endl;
	return m_songs.currentPtr(); //placeholder to see if mechanism works.
}