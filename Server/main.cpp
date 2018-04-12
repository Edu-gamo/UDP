#include "SFML\Network.hpp"
#include <iostream>

int MAX_PLAYERS = 4;

unsigned short port = 5000;

sf::UdpSocket socket;
sf::Socket::Status status;

sf::Packet packetIn, packetOut;

sf::IpAddress senderIP;
unsigned short senderPort;

enum Commands {
	HELLO, WELCOME
};

class Player {
public:
	Player();
	~Player();

	sf::IpAddress ip;
	unsigned short port;

	std::string nickname;
	int id_player;

	int posX, posY;

};

Player::Player()
{
}

Player::~Player()
{
}

std::vector<Player> players;

bool send(sf::Packet packet, int index) {
	return (socket.send(packet, players[index].ip, players[index].port) == sf::Socket::Done);
}

/*void initPosition(int index) {



}*/

void main() {

	socket.setBlocking(false);

	bool running = true;

	status = socket.bind(port);
	if (status != sf::Socket::Done) {
		std::cout << "Error al vincular al puerto " << port << std::endl;
		running = false;
	} else {
		std::cout << "Vinculado al puerto " << port << std::endl;
	}

	while (running) {

		status = socket.receive(packetIn, senderIP, senderPort);
		if (status == sf::Socket::Done) {
			int enmunVar;
			Commands com;
			packetIn >> enmunVar;
			com = (Commands)enmunVar;
			switch (com) {
			case HELLO: {
				if (players.size() < MAX_PLAYERS) {
					bool exist = false;
					int i = 0;
					while (i < players.size() && !exist) {
						if (senderIP == players[i].ip && senderPort == players[i].port) exist = true;
						else i++;
					}
					if (!exist) {
						Player newPlayer;
						newPlayer.ip = senderIP;
						newPlayer.port = senderPort;
						packetIn >> newPlayer.nickname;
						players.push_back(newPlayer);
						players.back().id_player = players.size();
					}
					//packetOut << Commands::WELCOME << players[i].id_player /*<< players[i].posX << players[i].posY*/;
					packetOut << Commands::WELCOME << players[i].nickname;
					if (!send(packetOut, i)) {
						std::cout << "Error al enviar datos al jugador: " << players[i].nickname << std::endl;
					}
					std::cout << "Se ha conectado el jugador: " << players[i].nickname << " : " << players[i].id_player << std::endl;
				}
				break;
			}
				default:
					break;
			}
		} else if (status != sf::Socket::NotReady) {
			std::cout << "Error al recibir datos" << std::endl;
		}

	}

}