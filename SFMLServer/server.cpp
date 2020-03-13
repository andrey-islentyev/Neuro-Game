#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <vector>

#define GAME_START_TIME 10.0
#define PLAYER_MAX 8
//10 milliseconds for handling messages from players
#define RECV_PHASE_TIMEOUT 10.0
//hard limit of messages per one game loop
#define RECV_MSG_HARD_LIMIT 10

struct PlayerInfo;

void pause() {
    std::cout.flush();
#ifdef _WIN32
    system("pause");
#endif
}

enum RequestToClientType {
    ChangePlantState = 1
};

struct PlayerInfo {
    sf::TcpSocket authSocket;
    sf::UdpSocket sendSocket, recvSocket;
	sf::Uint16 sendPort, recvPort;
    std::string nickname;
    sf::IpAddress ipAddress;
    std::queue<sf::Packet> messages;
    sf::Socket::Status sendToUdp(sf::Packet &packet) {
        sf::Socket::Status status = sendSocket.send(packet, ipAddress, sendPort);
		return status;
    }
    sf::Socket::Status receiveFromUdp(sf::Packet &packet) {
        return recvSocket.receive(packet, ipAddress, recvPort);
    }
};

struct Plant {
    double x, y;
    sf::Int8 state;
    sf::Uint8 type;
    sf::Uint64 id;
    sf::Clock clock;
    float deadline;
    static std::uniform_real_distribution<float> timeDistribution;
    static std::default_random_engine *engine;
    bool deadlinePassed() {
        if(state == 1) return false;
        return clock.getElapsedTime().asSeconds() > deadline;
    }
    void generateNewDeadline() {
        deadline = timeDistribution(*engine);
    }
    void changeState() {
        if (state == 0)
            state = 1;
        if (state == 2)
            state = 0;
		std::cout << "state = " << (int)(int8_t)state << "\n";
    }
    void grow(PlayerInfo *players, size_t playersCount) {
        if (state != 1) {
            changeState();
            notify(players, playersCount);
        }
    }
    void notify(PlayerInfo *players, size_t playersCount) {
        for (size_t i = 0; i < playersCount; ++i) {
            PlayerInfo &info = players[i];
			sf::Packet packet;
       		packet << sf::Uint8{ChangePlantState};
        	packet << id;
        	packet << type;
        	packet << state;
        	packet << x;
        	packet << y;
			sf::Socket::Status status;
            if((status = info.sendToUdp(packet)) != sf::Socket::Done){
				std::cout << "Error. Can't send packet with plant update " << status << "\n";
				pause();
				exit(EXIT_FAILURE);
			}
        }
    }
};

std::uniform_real_distribution<float> Plant::timeDistribution;
std::default_random_engine *Plant::engine = nullptr;


void giveIpHint() {
    sf::IpAddress localAddress = sf::IpAddress::getLocalAddress();
    sf::IpAddress publicAddress = sf::IpAddress::getPublicAddress();
    std::cout << "Local address: " << localAddress.toString() << "\n";
    std::cout << "Public address: " << publicAddress.toString() << "\n";
    std::cout << "What port to use?: ";
}

int main() {
    std::default_random_engine engine;
    Plant::engine = &engine;
    Plant::timeDistribution = std::uniform_real_distribution<float>(20.0, 50.0);
    giveIpHint();
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
    uint64_t playersCount = 0;
    PlayerInfo playersInfo[PLAYER_MAX];
    sf::Clock clock;
    listener.setBlocking(false);
    int prevSec = -1;
    while (playersCount != PLAYER_MAX) {
        int timeLeft = (int)(GAME_START_TIME - clock.getElapsedTime().asSeconds());
        if (timeLeft <= 0 && playersCount >= 2) {
            break;
        }
        if (listener.accept(playersInfo[playersCount].authSocket) == sf::Socket::Done) {
            std::cout << "Player with id #" << playersCount << " connected\n";
            playersInfo[playersCount].ipAddress = playersInfo[playersCount].authSocket.getRemoteAddress();
            playersCount++;
            if (playersCount >= 2) {
                clock.restart();
            }
        }
        if (timeLeft != prevSec) {
            prevSec = timeLeft;
            if (playersCount >= 2) {
                if (prevSec >= 60) {
                    int minutes = prevSec / 60;
                    int seconds = prevSec % 60;
                    std::cout << minutes << "m " << seconds << "s"
                              << "\n";
                }
                if ((prevSec % 60 == 0 || prevSec == 20 || prevSec == 30 || prevSec == 10 || prevSec <= 5))
                    std::cout << timeLeft << "s left" << std::endl;
            }
        }
    }
    std::cout << "Game started\n";
    for (size_t i = 0; i < playersCount; ++i) {
        if (playersInfo[i].sendSocket.bind(0) != sf::Socket::Done) {
            std::cout << "Can't open udp socket\n";
            pause();
            return EXIT_FAILURE;
        }
		if (playersInfo[i].recvSocket.bind(0) != sf::Socket::Done) {
			std::cout << "Can't open udp socket\n";
			pause();
			return EXIT_FAILURE;
		}
        sf::Uint16 sendPort = playersInfo[i].sendSocket.getLocalPort();
		sf::Uint16 recvPort = playersInfo[i].recvSocket.getLocalPort();
        sf::Packet packet;
		packet << sendPort;
        packet << recvPort;
        if (playersInfo[i].authSocket.send(packet) != sf::Socket::Done) {
            std::cout << "Can't send server UDP port data\n";
            pause();
            return EXIT_FAILURE;
        }
    }
    for (size_t i = 0; i < playersCount; ++i) {
        sf::Packet packet;
        if (playersInfo[i].authSocket.receive(packet) != sf::Socket::Done) {
            std::cout << "Can't recieve client UDP port data\n";
            pause();
            return EXIT_FAILURE;
        }
        packet >> playersInfo[i].sendPort;
		packet >> playersInfo[i].recvPort;
        packet >> playersInfo[i].nickname;
        playersInfo[i].recvSocket.setBlocking(false);
		playersInfo[i].sendSocket.setBlocking(false);
    }
    for (size_t i = 0; i < playersCount; ++i) {
        std::cout << "#" << i << " nickname: " << playersInfo[i].nickname << "\n";
    }

    //game state
    std::map<size_t, Plant> plants;
    double minx = 100;
    double miny = 100;
    double maxx = 400;
    double maxy = 400;
    std::uniform_real_distribution<double> xDistribution(minx, maxx);
    std::uniform_real_distribution<double> yDistribution(miny, maxy);
    std::uniform_int_distribution<int> typeDistribution(0, 6);
    for (size_t i = 0; i < 20; ++i) {
        plants[i].x = xDistribution(engine);
        plants[i].y = yDistribution(engine);
        plants[i].type = typeDistribution(engine);
        plants[i].id = i;
        plants[i].state = 2;
        plants[i].generateNewDeadline();
        plants[i].notify(playersInfo, playersCount);
    }
    sf::Clock phaseClock;
    while (true) {
        phaseClock.restart();
        //recieve packets stage
       /* while (phaseClock.getElapsedTime().asMilliseconds() <= RECV_PHASE_TIMEOUT) {
            for (size_t i = 0; i < playersCount; ++i) {
                sf::Packet packet;
                if (playersInfo[i].messages.size() >= RECV_MSG_HARD_LIMIT) {
                    continue;
                }
                if (playersInfo[i].udpSocket.receive(
                        packet, playersInfo[i].ipAddress, playersInfo[i].udpPort) == sf::Socket::Done) {
                    playersInfo[i].messages.push(packet);
                }
            }
        }*/
        //game update stage.
        for (size_t i = 0; i < plants.size(); ++i) {
            if (plants[i].deadlinePassed()) {
                plants[i].grow(playersInfo, playersCount);
                plants[i].generateNewDeadline();
				std::cout << "State update\n";
            }
        }
        //clear all queues
        for (size_t i = 0; i < playersCount; ++i) {
            while (!playersInfo[i].messages.empty()) {
                playersInfo[i].messages.pop();
            }
        }
    }
    pause();
}