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

sf::RenderWindow window(sf::VideoMode(800, 600), "Client");

enum Commands {
	HELLO, WELCOME, NEWPLAYER, ACK, DISCONECT, PING
};

class Player {
public:
	Player();
	~Player();

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
Player me;

bool send(sf::Packet packet) {
	return (socket.send(packet, serverIp, serverPort) == sf::Socket::Done);
}

sf::Socket::Status receive() {
	return socket.receive(packetIn, senderIP, senderPort);
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

void update() {
	
	sf::Event event;

	//Este primer WHILE es para controlar los eventos del mouse
	while (window.pollEvent(event)) {
		switch (event.type) {
		case sf::Event::Closed:
			window.close();
			break;
			/*case sf::Event::KeyPressed:
			if (event.key.code == sf::Keyboard::Left) {
			sf::Packet pckLeft;
			int posAux = pos - 1;
			pckLeft << posAux;
			sock.send(pckLeft, IP_SERVER, PORT_SERVER);

			} else if (event.key.code == sf::Keyboard::Right) {
			sf::Packet pckRight;
			int posAux = pos + 1;
			pckRight << posAux;
			sock.send(pckRight, IP_SERVER, PORT_SERVER);
			}
			break;*/

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
				packetIn >> idPacket;
				players.emplace(newIdPlayer, newPlayer);
				std::cout << "Nuevo jugador: " << newPlayer.nickname << std::endl;
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

void draw() {

	window.clear();

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

	//Pîntar jugadores
	for (std::map<int, Player>::iterator it = players.begin(); it != players.end(); it++) {
		sf::RectangleShape rectAvatar(sf::Vector2f(30, 30));
		rectAvatar.setFillColor(sf::Color::Red);
		rectAvatar.setPosition(sf::Vector2f(it->second.posX, it->second.posY));
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

		window.setTitle("Player " + me.nickname);

		while (window.isOpen()) {
			update();
			draw();
		}

	}

}