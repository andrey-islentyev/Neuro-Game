#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include <SFML/System.hpp>

#define GAME_START_TIME 20.0
#define PLAYER_MAX 8

struct PlayerInfo;

void pause() {
	std::cout.flush();
#ifdef  _WIN32
	system("pause");
#endif
}

void mainThread(PlayerInfo* p) {

}

struct PlayerInfo {
	sf::TcpSocket authSocket;
	sf::UdpSocket udpSocket;
	sf::Thread playerThread;
	sf::Uint16 udpPort;
	std::string nickname;
	PlayerInfo() : playerThread(mainThread, this) {}
};

int main() {
	sf::IpAddress localAddress = sf::IpAddress::getLocalAddress();
	sf::IpAddress publicAddress = sf::IpAddress::getPublicAddress();
	std::cout << "Local address: " << localAddress.toString() << "\n";
	std::cout << "Public address: " << publicAddress.toString() << "\n";
	std::cout << "What port to use?: ";
	uint16_t port;
	std::cin >> port;
	sf::TcpListener listener;
	sf::Socket::Status status = listener.listen(port);
	if (status != sf::Socket::Done) {
		std::cout << "Error: can't listen on this port\n";
		pause();
		return EXIT_FAILURE;
	}
	std::cout << "Listening on the port: " << listener.getLocalPort() << "\n";
	uint64_t currentSlot = 0;
	PlayerInfo playersInfo[PLAYER_MAX];
	sf::Clock clock;
	listener.setBlocking(false);
	int prevSec = -1;
	while (currentSlot != PLAYER_MAX) {
		int timeLeft = (int)(GAME_START_TIME - clock.getElapsedTime().asSeconds());
		if (timeLeft <= 0 && currentSlot >= 2) {
			break;
		}
		if (listener.accept(playersInfo[currentSlot].authSocket) == sf::Socket::Done) {
			std::cout << "Player with id #" << currentSlot << " connected\n";
			currentSlot++;
			if (currentSlot >= 2) {
				clock.restart();
			}
		}
		if (timeLeft != prevSec) {
			prevSec = timeLeft;
			if ((prevSec % 60 == 0 || prevSec == 20 || prevSec == 30 || prevSec == 10 || prevSec <= 5) && currentSlot >= 2)
				std::cout << timeLeft << " seconds left" << std::endl;
		}
	}
	std::cout << "Game started\n";
	for (size_t i = 0; i < currentSlot; ++i) {
		if (playersInfo[i].udpSocket.bind(0) != sf::Socket::Done) {
			std::cout << "Can't open udp socket\n";
			pause();
			return EXIT_FAILURE;
		}
		sf::Uint16 port = playersInfo[i].udpSocket.getLocalPort();
		sf::Packet packet;
		packet << port;
		if (playersInfo[i].authSocket.send(packet) != sf::Socket::Done) {
			std::cout << "Can't send server UDP port data\n";
			pause();
			return EXIT_FAILURE;
		}
	}
	for (size_t i = 0; i < currentSlot; ++i) {
		sf::Packet packet;
		if (playersInfo[i].authSocket.receive(packet) != sf::Socket::Done) {
			std::cout << "Can't recieve client UDP port data\n";
			pause();
			return EXIT_FAILURE;
		}
		packet >> playersInfo[i].udpPort;
		packet >> playersInfo[i].nickname;
	}
	for (size_t i = 0; i < currentSlot; ++i) {
		std::cout << "#" << i << " nickname: " << playersInfo[i].nickname << "\n";
	}
	pause();
}