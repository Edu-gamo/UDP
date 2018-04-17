#include "SFML\Network.hpp"
#include <iostream>

int MAX_PLAYERS = 4;

unsigned short port = 5000;

sf::UdpSocket socket;
sf::Socket::Status status;

sf::Packet packetIn, packetOut;

sf::IpAddress senderIP;
unsigned short senderPort;

INT8 playerIds = 0;

enum Commands {
	HELLO, WELCOME, NEWPLAYER, ACK, DISCONECT
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

std::map<int, Player> players;

std::map<int, sf::Packet> criticalMSG;
int criticalID;

bool send(sf::Packet packet, int index) {
	return (socket.send(packet, players[index].ip, players[index].port) == sf::Socket::Done);
}

//Envia un Packet a todos los clientes menos al del indice
void sendAll(sf::Packet packet, int index) {
	for (int i = 0; i < players.size(); i++) {
		if(index != i) socket.send(packet, players[i].ip, players[i].port);
	}
}

//Envia un Packet a todos los clientes
void sendAll(sf::Packet packet) {
	for (int i = 0; i < players.size(); i++) {
		socket.send(packet, players[i].ip, players[i].port);
	}
}

void sendCriticalMSG(sf::Packet packet) {
	
}

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
						newPlayer.id_player = playerIds;
						playerIds++;
						players.insert(newPlayer.id_player, newPlayer);
					}
					packetOut.clear();
					packetOut << Commands::WELCOME << players[i].id_player; /*<< players[i].posX << players[i].posY*/
					if (!send(packetOut, i)) {
						std::cout << "Error al enviar datos al jugador: " << players[i].nickname << std::endl;
					} else {
						packetOut.clear();
						packetOut << Commands::NEWPLAYER << players[i].id_player;
						//Hay que enviar NEWPLAYER a todos y guardarlo como critical hasta recibir el ACK, por cada cliente
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