#include "SFML\Network.hpp"
#include <iostream>
#include <time.h>

int MAX_PLAYERS = 4;
int RND = 20;

unsigned short port = 5000;

unsigned short max_pings = 5;

sf::UdpSocket socket;
sf::Socket::Status status;

sf::Packet packetIn, packetOut;

sf::IpAddress senderIP;
unsigned short senderPort;

INT8 playerIds = 0;

int pingCount = 0;
bool gameReady = false;

enum Commands {
	HELLO, WELCOME, NEWPLAYER, ACK, DISCONECT, PING, OBSTACLE_SPAWN, MOVE, DEAD
};


class Player {
public:
	Player();
	~Player();

	int lastMoveId = 0;

	sf::IpAddress ip;
	unsigned short port;

	std::string nickname;
	int id_player;

	int posX, posY;

	unsigned short pings;

	std::map<int, sf::Packet> criticalMSG;
	int criticalID = 0;

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
		/*for (std::map<int, CriticalPacket>::iterator it = criticalMSG.begin(); it != criticalMSG.end(); it++) {
			if (players.at(playersToRemove[i]).port == it->second.port) criticalMSG.erase(it->first);
		}*/
		players.erase(playersToRemove[i]);
	}
	playersToRemove.clear();
}

bool send(sf::Packet packet, int index) {

	if (rand() % RND == 1) {
		std::cout << "Se ha perdido el Paquete al enviarse\n";
		return true;
	} else {
		return (socket.send(packet, players.at(index).ip, players.at(index).port) == sf::Socket::Done);
	}
}

//Envia un Packet a todos los clientes menos al del indice
void sendAll(sf::Packet packet, int index) {
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		if (rand() % RND == 1) {
			std::cout << "Se ha perdido el Paquete al enviarse a \n" << it->second.nickname <<"\n";
		} else {
			if (index != it->first) socket.send(packet, it->second.ip, it->second.port);
		}
	}
}

//Envia un Packet a todos los clientes
void sendAll(sf::Packet packet) {
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {		
		if (rand() % RND == 1) {
			std::cout << "Se ha perdido el Paquete al enviarse a \n" << it->second.nickname << "\n";
		} else {
			socket.send(packet, it->second.ip, it->second.port);
		}
	}
}

void sendCriticalMSG(sf::Packet packet, int index) {

	packet << players.at(index).criticalID;
	players.at(index).criticalMSG.emplace(players.at(index).criticalID, packet);
	players.at(index).criticalID++;

	send(packet, index);
	
}

void sendAllCriticalMSG() {

	for (std::map<int, Player>::iterator playerIt = players.begin(); playerIt != players.end(); playerIt++) {

		for (std::map<int, sf::Packet>::iterator criticalIt = playerIt->second.criticalMSG.begin(); criticalIt != playerIt->second.criticalMSG.end(); criticalIt++) {
		
			if (rand() % RND == 1) {
				std::cout << "Se ha perdido el Paquete al enviarse\n";
			} else {
				socket.send(criticalIt->second, playerIt->second.ip, playerIt->second.port);
			}

		}

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
		std::cout << "Pings de " << it->second.nickname << " : " << it->second.pings << std::endl;
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

class obstacle
{
public:

	int spawnPointY;
	int speed;

	int currentX = 800;
	int deltaX;

	obstacle();
	~obstacle();

private:

};

obstacle::obstacle()
{
}

obstacle::~obstacle()
{
}

std::map<int, obstacle> ObstacleMap;
std::vector<int> obstaclesToRemove;
int obstacleId = 0;

void removeObstacles() {
	for (int i = 0; i < obstaclesToRemove.size(); i++) {
		ObstacleMap.erase(obstaclesToRemove[i]);
	}
	obstaclesToRemove.clear();
}

void spawnObstacle() {
	
	if (gameReady == true) {
		int spawnPoint = rand() % 570;
		int speed = rand() % 1 + 1;

		packetOut.clear();
		packetOut << Commands::OBSTACLE_SPAWN << obstacleId << spawnPoint << speed;

		obstacle newObstacle = obstacle();
		newObstacle.spawnPointY = spawnPoint;
		newObstacle.speed = speed;
		ObstacleMap.emplace(obstacleId, newObstacle);

		for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
			sendCriticalMSG(packetOut, it->first);
		}

		obstacleId++;
	}
}

void checkColision() {

	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		if (it->second.posX > 602 || it->second.posX < 172) {
			packetOut << Commands::DEAD;
			sendCriticalMSG(packetOut, it->first);
			packetOut.clear();
		} else {
			for (std::map<int, obstacle>::iterator iterator = ObstacleMap.begin(); iterator != ObstacleMap.end(); iterator++) {
				if (it->second.posX + 30 >= iterator->second.currentX && it->second.posX <= iterator->second.currentX + 20) {
					if (it->second.posY <= iterator->second.spawnPointY + 20 && it->second.posY + 30 >= iterator->second.spawnPointY) {
						packetOut << Commands::DEAD;
						sendCriticalMSG(packetOut, it->first);
						packetOut.clear();
					}
				}
			}
		}
	}
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
						std::cout << "Genero el player " << newPlayer.id_player << std::endl;
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
								packetOut << Commands::NEWPLAYER << it->first << it->second.nickname << it->second.posX << it->second.posY;
								sendCriticalMSG(newPlayerPacket, it->first);
								sendCriticalMSG(packetOut, i->first);
							}
						}
					}
					std::cout << "Se ha conectado el jugador: " << i->second.nickname << " : " << i->first << std::endl;
					if (players.size() == 4) { gameReady = true; std::cout << "GAME READY\n"; }
				}
				break;
			}
			case PING: {
				int id;
				packetIn >> id;
				players.at(id).pings = 0;
				std::cout << "Arriba ping del player " << id << std::endl;
			}
				break;
			case ACK: {
				int idPlayer, idPacket;
				packetIn >> idPlayer >> idPacket;
				players.at(idPlayer).criticalMSG.erase(idPacket);
			}
				break;
			case MOVE: {
				int idPlayer, idPacket, deltaX, deltaY;
				packetIn >> idPlayer >> idPacket;
				if (players.find(idPlayer) != players.end()) {
					if (players.at(idPlayer).lastMoveId < idPacket) {
						packetIn >> deltaX >> deltaY;
						players.at(idPlayer).posX += deltaX;
						players.at(idPlayer).posY += deltaY;
						packetOut.clear();
						packetOut << Commands::MOVE << idPlayer << idPacket << players.at(idPlayer).posX << players.at(idPlayer).posY;
						sendAll(packetOut);
					}
				}
			}
				break;
			case DEAD: {
				int idPlayer;
				packetIn >> idPlayer;
				std::cout << "Ha muerto el jugador: " << idPlayer << std::endl;
				disconect(idPlayer);
				packetIn.clear();
			}
				break;
			default:
				break;
			}
		} else if (status != sf::Socket::NotReady) {
			std::cout << "Error al recibir datos" << std::endl;
		}

		if (clock.getElapsedTime().asMilliseconds() > 100) {
			clock.restart();
			if (pingCount >= 10) {
				sendPing();
				pingCount = 0;
				spawnObstacle();
			}
			sendAllCriticalMSG();
			pingCount++;
		}

		//Update obstacle Position
		for (std::map<int, obstacle>::iterator it = ObstacleMap.begin(); it != ObstacleMap.end(); it++) {
			if (it->second.currentX <= 175 - it->second.speed * 10) {
				obstaclesToRemove.push_back(it->first);
			} else {
				it->second.currentX -= it->second.speed;
			}
		}

		removePlayersDisconected();
		removeObstacles();
		//checkColision();

	}

	socket.unbind();

}