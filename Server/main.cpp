#include "SFML\Network.hpp"
#include <iostream>
#include <time.h>

int MAX_PLAYERS = 4;

unsigned short port = 5000;

unsigned short max_pings = 5;

sf::UdpSocket socket;
sf::Socket::Status status;

sf::Packet packetIn, packetOut;

sf::IpAddress senderIP;
unsigned short senderPort;

INT8 playerIds = 0;

int pingCount = 0;

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
std::vector<int> playersToRemove;

void removePlayersDisconected() {
	for (int i = 0; i < playersToRemove.size(); i++) {
		players.erase(playersToRemove[i]);
	}
}

struct CriticalPacket {

	sf::IpAddress ip;
	unsigned short port;
	sf::Packet packet;

};

std::map<int, CriticalPacket> criticalMSG;
int criticalID;

bool send(sf::Packet packet, int index) {
	return (socket.send(packet, players.at(index).ip, players.at(index).port) == sf::Socket::Done);
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
	playersToRemove.push_back(id);
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		if(it->first != id) sendCriticalMSG(packetOut, it->first);
	}
}

void sendPing() {
	packetOut.clear();
	packetOut << Commands::PING;
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		socket.send(packetOut, it->second.ip, it->second.port);
		it->second.pings++;
		std::cout << it->second.nickname << " : " << it->second.pings << std::endl;
		if (it->second.pings >= max_pings) {
			std::cout << it->second.nickname << " se ha desconectado\n";
			disconect(it->first);
		}
	}
}

void giveRandomPos(int id) {

	players.at(id).posX = rand() % 370 + 200;
	players.at(id).posY = rand() % 570;

}

void main() {

	srand(time(NULL));

	socket.setBlocking(false);

	bool running = true;

	status = socket.bind(port);
	if (status != sf::Socket::Done) {
		std::cout << "Error al vincular al puerto " << port << std::endl;
		running = false;
	} else {
		std::cout << "Vinculado al puerto " << port << std::endl;
	}

	sf::Clock clock;

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
						Player newPlayer = Player();
						newPlayer.ip = senderIP;
						newPlayer.port = senderPort;
						packetIn >> newPlayer.nickname;
						newPlayer.id_player = playerIds;
						playerIds++;
						players.emplace(newPlayer.id_player, newPlayer);
						i--;
						giveRandomPos(i->first);
					}
					packetOut.clear();
					packetOut << Commands::WELCOME << i->first << i->second.posX << i->second.posY;
					if (!send(packetOut, i->first)) {
						std::cout << "Error al enviar datos al jugador: " << i->second.nickname << std::endl;
					} else {
						sf::Packet newPlayerPacket;
						newPlayerPacket.clear();
						newPlayerPacket << Commands::NEWPLAYER << i->first << i->second.nickname << i->second.posX << i->second.posY;
						for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
							if (it->first != i->first) {
								packetOut.clear();
								packetOut << Commands::NEWPLAYER << it->first << i->second.nickname << it->second.posX << it->second.posY;
								sendCriticalMSG(newPlayerPacket, it->first);
								sendCriticalMSG(packetOut, i->first);
							}
						}
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
			case ACK: {
				int id;
				packetIn >> id;
				criticalMSG.erase(id);
			}
					   break;
				default:
					break;
			}
		} else if (status != sf::Socket::NotReady) {
			std::cout << "Error al recibir datos" << std::endl;
		}

		if (clock.getElapsedTime().asMilliseconds() > 500) {
			pingCount++;
			clock.restart();
			if (pingCount >= 5) {
				sendPing();
				pingCount = 0;
			}
			sendAllCriticalMSG();
		}

		removePlayersDisconected();

	}

	socket.unbind();

}