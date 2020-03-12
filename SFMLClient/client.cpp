#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <ctime>
#include <random>
#include <cmath>

void pause() {
	std::cout.flush();
#ifdef  _WIN32
	system("pause");
#endif
}

const int width = 1000;
const int height = 500;
const int maxPlayerCount = 7;
uint16_t port = 5757;
std::string ip = "127.0.0.1";

sf::Sprite** textures;
sf::Texture texture;
sf::UdpSocket socket;
sf::Color revealed;
sf::Color unrevealed;
sf::UdpSocket udpSocket;
sf::Packet packet;
sf::Uint16 serverPort;
sf::Uint16 clientPort;

void initialize() {
	revealed = sf::Color(47, 129, 54);
	unrevealed = sf::Color(39, 109, 45);
	texture.loadFromFile("plants.png");
	if (socket.bind(5757) != sf::Socket::Done) {}
	textures = new sf::Sprite*[7];
	for (int i = 0; i < 7; i++) {
		textures[i] = new sf::Sprite[3];
		for (int j = 0; j < 3; j++) {
			textures[i][j].setTexture(texture, true);
			textures[i][j].setTextureRect(sf::IntRect(i * 32, (j + 2) * 64, 32, 64));
			textures[i][j].setScale(sf::Vector2f(2, 2));
		}
	}
}

int main() {
	setlocale(LC_ALL, "Russian");
	std::cout << "Nickname: ";
	std::string nickname;
	std::getline(std::cin, nickname);
	std::cout << "ip: ";
	std::getline(std::cin, ip);
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
	initialize();
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear();
		window.draw(textures[4][1]);
		window.display();
	}
	return 0;
}