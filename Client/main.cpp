#include "SFML\Network.hpp"
#include <iostream>
#include <chrono>

typedef std::chrono::high_resolution_clock Clock;

sf::IpAddress serverIp = sf::IpAddress::getLocalAddress();
unsigned short serverPort = 5000;

sf::UdpSocket socket;
sf::Socket::Status status;

sf::Packet packetIn, packetOut;

sf::IpAddress senderIP;
unsigned short senderPort;

enum Commands {
	HELLO, WELCOME, NEWPLAYER, ACK, DISCONECT, PING
};

class Player {
public:
	Player();
	~Player();

	std::string nickname;
	int id_player;

	int position[2];

};

Player::Player()
{
}

Player::~Player()
{
}

std::map<int, Player> players;
Player me;

bool send(sf::Packet packet) {
	return (socket.send(packet, serverIp, serverPort) == sf::Socket::Done);
}

sf::Socket::Status receive() {
	return socket.receive(packetIn, senderIP, senderPort);
}

bool connect() {
	packetOut << Commands::HELLO << me.nickname;
	if (!send(packetOut)) return false;

	auto c1 = Clock::now();
	auto c2 = Clock::now();
	int time = std::chrono::duration_cast<std::chrono::milliseconds>(c2 - c1).count();

	while (time < 100) {
		c2 = Clock::now();
		time = std::chrono::duration_cast<std::chrono::milliseconds>(c2 - c1).count();

		status = receive();
		if (status == sf::Socket::Done) {
			int enmunVar;
			Commands com;
			packetIn >> enmunVar;
			com = (Commands)enmunVar;
			switch (com) {
			case WELCOME:
				packetIn >> me.id_player;
				std::cout << "Eres el jugador: " << me.id_player << std::endl;
				return true;
			default:
				break;
			}
		} else if (status != sf::Socket::NotReady) {
			std::cout << "Error al recibir datos" << std::endl;
		}
	}

	return connect();

}

void main() {

	socket.setBlocking(false);

	std::cout << "Enter nickname: ";
	std::cin >> me.nickname;

	bool running = true;

	if (!connect()) {
		std::cout << "Error al conectar al servidor (" << serverIp << ":" << serverPort << ")" << std::endl;
		running = false;
	} else {
		std::cout << "Conectado al servidor (" << serverIp << ":" << serverPort << ")" << std::endl;
	}

	while (running) {

		status = receive();
		if (status == sf::Socket::Done) {
			int enmunVar;
			int idPacket;
			Commands com;
			packetIn >> enmunVar;
			com = (Commands)enmunVar;
			switch (com) {
			case NEWPLAYER:
			{
				int newIdPlayer;
				packetIn >> newIdPlayer; //recibir nickname y posicion
				packetIn >> idPacket;
				bool exist = false;
				std::map<int, Player>::iterator i = players.begin();
				while (i != players.end() && !exist) {
					if (newIdPlayer == i->first) exist = true;
					else i++;
				}
				if (!exist) {
					Player newPlayer;
					newPlayer.id_player = newIdPlayer;
					players.emplace(newIdPlayer, newPlayer);
				}
				packetOut.clear();
				packetOut << Commands::ACK << idPacket;
				send(packetOut);
				break;
			}
			case PING: {
				packetOut.clear();
				packetOut << Commands::PING << me.id_player;
				send(packetOut);
			}
				break;
			case DISCONECT: {
				int id;
				packetIn >> id;
				packetIn >> idPacket;
				bool exist = false;
				std::map<int, Player>::iterator i = players.begin();
				while (i != players.end() && !exist) {
					if (id == i->first) exist = true;
					else i++;
				}
				if (exist) {
					players.erase(id);
				}
				packetOut.clear();
				packetOut << Commands::ACK << idPacket;
			}
				break;
			default:
				break;
			}
		} else if (status != sf::Socket::NotReady) {
			std::cout << "Error al recibir datos" << std::endl;
		}
		
	}

}