#include "SFML\Network.hpp"
#include <iostream>
#include <chrono>

typedef std::chrono::high_resolution_clock Clock;

int MAX_PLAYERS = 4;

unsigned short port = 5000;

unsigned short max_pings = 5;

sf::UdpSocket socket;
sf::Socket::Status status;

sf::Packet packetIn, packetOut;

sf::IpAddress senderIP;
unsigned short senderPort;

INT8 playerIds = 0;

enum Commands {
	HELLO, WELCOME, NEWPLAYER, ACK, DISCONECT, PING
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

	unsigned short pings;

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
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		if(index != it->first) socket.send(packet, it->second.ip, it->second.port);
	}
}

//Envia un Packet a todos los clientes
void sendAll(sf::Packet packet) {
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		socket.send(packet, it->second.ip, it->second.port);
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
	for (std::map<int, CriticalPacket>::iterator it = criticalMSG.begin(); it != criticalMSG.end(); it++) {
		socket.send(it->second.packet, it->second.ip, it->second.port);
	}
}

void disconect(int id) {
	packetOut.clear();
	packetOut << Commands::DISCONECT << id;
	players.erase(id);
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		sendCriticalMSG(packetOut, it->first);
	}
}

void sendPing() {
	packetOut.clear();
	packetOut << Commands::PING;
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		socket.send(packetOut, it->second.ip, it->second.port);
		it->second.pings++;
		if (it->second.pings >= max_pings) {
			disconect(it->first);
		}
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

	auto c1 = Clock::now();
	auto c2 = Clock::now();
	int time = std::chrono::duration_cast<std::chrono::milliseconds>(c2 - c1).count();

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
						//Hay que recibir el ACK, por cada cliente
					}
					std::cout << "Se ha conectado el jugador: " << i->second.nickname << " : " << i->first << std::endl;
				}
				break;
			}
			case PING: {
				int id;
				packetIn >> id;
				players.at(id).pings = 0;
			}
				break;
				default:
					break;
			}
		} else if (status != sf::Socket::NotReady) {
			std::cout << "Error al recibir datos" << std::endl;
		}

		c2 = Clock::now();
		time = std::chrono::duration_cast<std::chrono::milliseconds>(c2 - c1).count();
		if (time > 1000) {
			c1 = Clock::now();
			sendPing();
			sendAllCriticalMSG();
		}

	}

}