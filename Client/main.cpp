#include "SFML\Graphics.hpp"
#include "SFML\Network.hpp"
#include <iostream>

sf::IpAddress serverIp = sf::IpAddress::getLocalAddress();
unsigned short serverPort = 5000;

sf::UdpSocket socket;
sf::Socket::Status status;

sf::Packet packetIn, packetOut;

sf::IpAddress senderIP;
unsigned short senderPort;

sf::RenderWindow window;

int idMove = 0;

int pasosInterpolacion = 4;

int RND = 20;

enum Commands {
	HELLO, WELCOME, NEWPLAYER, ACK, DISCONECT, PING, OBSTACLE_SPAWN, MOVE, DEAD
};

class Player {
public:
	Player();
	~Player();

	std::string nickname;
	int id_player;

	int posX, posY, newPosX, newPosY;
	int deltaX, deltaY;

	int lastMoveId = 0;

};

Player::Player()
{
}

Player::~Player()
{
}

std::map<int, Player> players;
Player me;

class obstacle
{
public:

	int spawnPointY;
	int speed;

	int currentX = 800;
	int deltaX;

	int size = 20;

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

void removeObstacles() {
	for (int i = 0; i < obstaclesToRemove.size(); i++) {
		ObstacleMap.erase(obstaclesToRemove[i]);
	}
	obstaclesToRemove.clear();
}

bool send(sf::Packet packet) {	
	if (rand() % RND == 1) {
		std::cout << "Se ha perdido el Paquete al enviarse\n";
		return true;
	} else {
		std::cout << "Se ha enviado el Paquete correctamente\n";
		return (socket.send(packet, serverIp, serverPort) == sf::Socket::Done);
	}
}

sf::Socket::Status receive() {
	return socket.receive(packetIn, senderIP, senderPort);
}

void dead() {
	packetOut.clear();
	packetOut << Commands::DEAD << me.id_player;
	send(packetOut);
	std::cout << "HAS MUERTO\n";
	window.close();
}

void checkColision() {
	
	if (me.posX > 602 || me.posX < 172) {
		dead();
	} else {
		for (std::map<int, obstacle>::iterator iterator = ObstacleMap.begin(); iterator != ObstacleMap.end(); iterator++) {
			if (me.posX + 30 >= iterator->second.currentX && me.posX <= iterator->second.currentX + 20) {
				if (me.posY <= iterator->second.spawnPointY + 20 && me.posY + 30 >= iterator->second.spawnPointY) {
					dead();
				}
			}
		}
	}
}

bool connect() {

	bool connected = false;
	int intents = 0;
	sf::Clock clock;

	do {

		if (clock.getElapsedTime().asMilliseconds() > 500) {
			packetOut.clear();
			packetOut << Commands::HELLO << me.nickname;
			if (!send(packetOut)) return false;
			clock.restart();
			intents++;
		}

		status = receive();
		if (status == sf::Socket::Done) {
			int enmunVar;
			Commands com;
			packetIn >> enmunVar;
			com = (Commands)enmunVar;
			switch (com) {
			case WELCOME:
				packetIn >> me.id_player;
				packetIn >> me.posX;
				packetIn >> me.posY;
				std::cout << "Eres el jugador: " << me.id_player << std::endl;
				connected = true;
			default:
				break;
			}
		}


	} while (!connected && intents < 100);

	return connected;

}

void movePlayers() {

	for (std::map<int, Player>::iterator playerIt = players.begin(); playerIt != players.end(); playerIt++) {

		if (playerIt->second.posX != playerIt->second.newPosX) playerIt->second.posX += playerIt->second.deltaX;
		if (playerIt->second.posY != playerIt->second.newPosY) playerIt->second.posY += playerIt->second.deltaY;

	}

}

void update(sf::Clock clock) {
	
	sf::Event event;

	//Este primer WHILE es para controlar los eventos del mouse
	while (window.pollEvent(event)) {
		switch (event.type) {
			case sf::Event::Closed:
				window.close();
				break;

			case sf::Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Left) {
					me.deltaX = me.deltaX - 5;
				} else if (event.key.code == sf::Keyboard::Right) {
					me.deltaX = me.deltaX + 5;
				} else if (event.key.code == sf::Keyboard::Up) {
					me.deltaY = me.deltaY - 5;
				} else if (event.key.code == sf::Keyboard::Down) {
					me.deltaY = me.deltaY + 5;
				}
				break;

			default:
				break;
		}
	}

	status = receive();
	if (status == sf::Socket::Done) {
		int enmunVar;
		int idPacket;
		Commands com;
		packetIn >> enmunVar;
		com = (Commands)enmunVar;
		switch (com) {

		case NEWPLAYER: {
			int newIdPlayer;
			packetIn >> newIdPlayer;
			bool exist = false;
			std::map<int, Player>::iterator i = players.begin();
			while (i != players.end() && !exist) {
				if (newIdPlayer == i->first) exist = true;
				else i++;
			}
			if (!exist) {
				Player newPlayer;
				newPlayer.id_player = newIdPlayer;
				packetIn >> newPlayer.nickname;
				packetIn >> newPlayer.posX;
				packetIn >> newPlayer.posY;
				newPlayer.newPosX = newPlayer.posX;
				newPlayer.newPosY = newPlayer.posY;
				packetIn >> idPacket;
				players.emplace(newIdPlayer, newPlayer);
				std::cout << "Nuevo jugador: " << newPlayer.nickname << std::endl;
			}
			packetOut.clear();
			packetOut << Commands::ACK << me.id_player << idPacket;
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
			packetOut << Commands::ACK << me.id_player << idPacket;
			send(packetOut);
		}
			break;

		case OBSTACLE_SPAWN: {
			int obstacleId, spawnPos, speed;

			packetIn >> obstacleId;
			packetIn >> spawnPos;
			packetIn >> speed;
			packetIn >> idPacket;

			bool exist = false;
			std::map<int, obstacle>::iterator i = ObstacleMap.begin();
			while (i != ObstacleMap.end() && !exist) {
				if (obstacleId == i->first) exist = true;
				else i++;
			}
			if (!exist) {
				obstacle newObstacle = obstacle();
				newObstacle.spawnPointY = spawnPos;
				newObstacle.speed = speed;
				ObstacleMap.emplace(obstacleId, newObstacle);
			}
			packetOut.clear();
			packetOut << Commands::ACK << me.id_player << idPacket;
			std::cout << "Envio ACK de spawn\n";
			send(packetOut);
			break;
		}
			break;
		case MOVE: {
			int idPlayer, idPacket;
			packetIn >> idPlayer >> idPacket;
			if (idPlayer == me.id_player) {
				if (idPacket > me.lastMoveId) {
					packetIn >> me.posX >> me.posY;
					me.lastMoveId = idPacket;
				}
			} else {
				if (idPacket > players.at(idPlayer).lastMoveId) {
					packetIn >> players.at(idPlayer).newPosX >> players.at(idPlayer).newPosY;
					//Sin interpolaci�n
					/*players.at(idPlayer).posX = newPosX;
					players.at(idPlayer).posY = newPosY;*/

					//Con interpolaci�n
					players.at(idPlayer).deltaX = players.at(idPlayer).newPosX - players.at(idPlayer).posX;
					players.at(idPlayer).deltaY = players.at(idPlayer).newPosY - players.at(idPlayer).posY;
					players.at(idPlayer).deltaX /= pasosInterpolacion;
					players.at(idPlayer).deltaY /= pasosInterpolacion;

					players.at(idPlayer).lastMoveId = idPacket;
				}
			}
		}
			break;
		/*case DEAD: {
			packetIn >> idPacket;			
			packetOut << Commands::DEAD << me.id_player << idPacket;
			send(packetOut);
			std::cout << "HAS MUERTO\n";
			window.close();
		}
			break;*/
		default:
			break;
		}

	} else if (status != sf::Socket::NotReady) {
		std::cout << "Error al recibir datos" << std::endl;
	}

	//Update obstacle Position
	for (std::map<int, obstacle>::iterator it = ObstacleMap.begin(); it != ObstacleMap.end(); it++) {
		if (it->second.size <= 0) {
			obstaclesToRemove.push_back(it->first);
		} else if(it->second.currentX <= 175 - it->second.speed * 10) {
			it->second.size--;
		} else {
			it->second.currentX -= it->second.speed;
		}
	}

	removeObstacles();
	checkColision();
	movePlayers();

	if (clock.getElapsedTime().asMilliseconds() > 100) {
		packetOut.clear();
		packetOut << Commands::MOVE << me.id_player << idMove << me.deltaX << me.deltaY;
		idMove++;
		if (me.deltaX != 0 || me.deltaY != 0) {
			std::cout << "envio delta\n";
			send(packetOut);
		}
		me.deltaX = me.deltaY = 0;
		clock.restart();
	}

}

void draw() {

	window.clear();

	sf::Text text;
	sf::Font font;

	if (!font.loadFromFile("comicSans.ttf")) {
		std::cout << "Can't load the font file" << std::endl;
	}
	text.setFont(font);
	text.setFillColor(sf::Color::White);
	text.setCharacterSize(16);

	//Pintar mapa
	sf::RectangleShape rectBlanco(sf::Vector2f(1, 600));
	rectBlanco.setFillColor(sf::Color::White);
	rectBlanco.setPosition(sf::Vector2f(200, 0));
	window.draw(rectBlanco);
	rectBlanco.setPosition(sf::Vector2f(600, 0));
	window.draw(rectBlanco);

	//Pintarte a ti mismo
	sf::RectangleShape rectAvatar(sf::Vector2f(30, 30));
	rectAvatar.setFillColor(sf::Color::Green);
	rectAvatar.setPosition(sf::Vector2f(me.posX, me.posY));
	window.draw(rectAvatar);
	text.setString(me.nickname);
	text.setPosition(me.posX, me.posY - 20);
	window.draw(text);

	//P�ntar jugadores
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		sf::RectangleShape rectAvatar(sf::Vector2f(30, 30));
		rectAvatar.setFillColor(sf::Color::Red);
		rectAvatar.setPosition(sf::Vector2f(it->second.posX, it->second.posY));
		window.draw(rectAvatar);
		text.setString(it->second.nickname);
		text.setPosition(it->second.posX, it->second.posY - 20);
		window.draw(text);
	}

	//Pintar obstaculos
	for (std::map<int, obstacle>::iterator it = ObstacleMap.begin(); it != ObstacleMap.end(); it++) {
		sf::RectangleShape rectAvatar(sf::Vector2f(it->second.size, it->second.size));
		rectAvatar.setFillColor(sf::Color::White);
		rectAvatar.setPosition(sf::Vector2f(it->second.currentX, it->second.spawnPointY));
		window.draw(rectAvatar);
	}

	window.display();
}

void main() {

	socket.setBlocking(false);

	std::cout << "Enter nickname: ";
	std::cin >> me.nickname;

	if (!connect()) {
		std::cout << "Error al conectar al servidor (" << serverIp << ":" << serverPort << ")" << std::endl;
	} else {
		std::cout << "Conectado al servidor (" << serverIp << ":" << serverPort << ")" << std::endl;

		window.create(sf::VideoMode(800, 600), "Player " + me.nickname);

		sf::Clock clock;
		while (window.isOpen()) {
			update(clock);
			draw();
		}

	}

	system("pause");

}