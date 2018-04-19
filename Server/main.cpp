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

struct CriticalPacket {

	sf::IpAddress ip;
	unsigned short port;
	sf::Packet packet;

};

std::map<int, CriticalPacket> criticalMSG;
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

void sendCriticalMSG(sf::Packet packet, int index) {

	CriticalPacket cp;
	cp.ip = players.at(index).ip;
	cp.port = players.at(index).port;
	packet << criticalID;
	cp.packet = packet;

	criticalMSG.emplace(criticalID, cp);
	criticalID++;

	send(packet, index);
	
}

void sendAllCriticalMSG() {
	for (int i = 0; i < criticalMSG.size(); i++) {
		socket.send(criticalMSG.at(i).packet, criticalMSG.at(i).ip, criticalMSG.at(i).port);
	}
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
					std::map<int, Player>::iterator i = players.begin();
					while (i != players.end() && !exist) {
						if (senderIP == i->second.ip && senderPort == i->second.port) exist = true;
						else i++;
					}
					if (!exist) {
						Player newPlayer;
						newPlayer.ip = senderIP;
						newPlayer.port = senderPort;
						packetIn >> newPlayer.nickname;
						newPlayer.id_player = playerIds;
						playerIds++;
						players.emplace(newPlayer.id_player, newPlayer);
					}
					packetOut.clear();
					packetOut << Commands::WELCOME << i->first; /*<< i->second.posX << i->second.posY*/
					if (!send(packetOut, i->first)) {
						std::cout << "Error al enviar datos al jugador: " << i->second.nickname << std::endl;
					} else {
						sf::Packet newPlayerPacket;
						newPlayerPacket.clear();
						newPlayerPacket << Commands::NEWPLAYER << i->first;/*<< i->second.posX << i->second.posY*/
						for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
							if (it->first != i->first) {
								packetOut.clear();
								packetOut << Commands::NEWPLAYER << it->first;/*<< it->second.posX << it->second.posY*/
								sendCriticalMSG(newPlayerPacket, it->first);
								sendCriticalMSG(packetOut, i->first);
							}
						}
						//Hay que enviar NEWPLAYER a todos y guardarlo como critical hasta recibir el ACK, por cada cliente
					}
					std::cout << "Se ha conectado el jugador: " << i->second.nickname << " : " << i->first << std::endl;
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