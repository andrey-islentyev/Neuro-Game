#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <set>
#include <map>

void pause() {
	std::cout.flush();
#ifdef  _WIN32
	system("pause");
#endif
}

const int width = 1000;
const int height = 500;
const int maxPlayerCount = 7;

bool comp(const std::pair<int, sf::Sprite>& lhs, const std::pair<int, sf::Sprite>& rhs) {
	return lhs.first < rhs.first;
}

int main() {
	std::set<std::pair<int, sf::Sprite>, bool(*)(const std::pair<int, sf::Sprite>& lhs, const std::pair<int, sf::Sprite>& rhs)> entity(comp);
	std::map<int, sf::Sprite> idTable;
	uint16_t port = 5757;
	std::string ipString = "127.0.0.1";
	sf::Sprite** textures;
	sf::Texture texture;
	sf::Color revealed;
	sf::Color unrevealed;
	sf::UdpSocket udpSocket;
	sf::Packet packet;
	sf::Uint16 serverPort;
	sf::Uint16 clientPort;
	setlocale(LC_ALL, "Russian");
	std::cout << "Nickname: ";
	std::string nickname;
	std::getline(std::cin, nickname);
	std::cout << "ip: ";
	std::getline(std::cin, ipString);
	sf::IpAddress ip = ipString;
	std::cout << "port: ";
	std::cin >> port;
	sf::TcpSocket socket;
	if (socket.connect(ip, port) != sf::Socket::Done) {
		std::cout << "Server not found\n";
		system("pause");
		exit(EXIT_FAILURE);
	}
	std::cout << "Connected\n";
	pause();
	if (socket.receive(packet) != sf::Socket::Done) {
		std::cout << "Can't recieve UDP server socket port\n";
		pause();
		return EXIT_FAILURE;
	}
	if (packet.getDataSize() != 2) {
		std::cout << "Incorrect UDP port packet\n";
		pause();
		return EXIT_FAILURE;
	}
	packet >> serverPort;
	std::cout << serverPort << std::endl;
	udpSocket.bind(0);
	clientPort = udpSocket.getLocalPort();
	packet = sf::Packet();
	packet << clientPort;
	packet << nickname;
	socket.send(packet);
	sf::RenderWindow window(sf::VideoMode(width, height), L"Client");
	window.setVerticalSyncEnabled(true);
	revealed = sf::Color(47, 129, 54);
	unrevealed = sf::Color(39, 109, 45);
	texture.loadFromFile("./res/plants.png");
	textures = new sf::Sprite*[7];
	for (int i = 0; i < 7; i++) {
		textures[i] = new sf::Sprite[3];
		for (int j = 0; j < 3; j++) {
			textures[i][j].setTexture(texture, true);
			textures[i][j].setTextureRect(sf::IntRect(i * 32, (j + 2) * 64, 32, 64));
			textures[i][j].setScale(sf::Vector2f(2, 2));
		}
	}
	udpSocket.setBlocking(false);
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear();
		sf::Packet* pac = new sf::Packet();
		while (udpSocket.receive(*pac, ip, serverPort) == sf::Socket::Done) {
			std::cout << "received\n";
			sf::Uint8 type, species;
			sf::Int8 fruit;
			sf::Uint64 id;
			double x, y;
			*pac >> type;
			if (type == 1) {
				*pac >> id >> species >> fruit >> x >> y;
				if (entity.size() > 0 && entity.find({ id, idTable[id] }) != entity.end()) 
					entity.erase({ id, idTable[id] });
				if (fruit != -1) {
					sf::Sprite s = textures[species][fruit];
					s.setPosition(sf::Vector2f(x, y));
					entity.insert({ id, s });
					idTable[id] = s;
				}
			}
		}
		delete pac;
		for (auto now : entity)
			window.draw(now.second);
		window.display();
	}
	return 0;
}