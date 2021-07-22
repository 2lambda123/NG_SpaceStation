#include "server.h"

Server::Server()
{
	m_mapFileLoader = new MapFileLoader();
	m_mapWorker = new MapWorker();

	m_mapWorker->processMap(m_mapFileLoader->getMap());

	log ("Server ready");
}

void Server::sendToAll(QByteArray message)
{
	for (QTcpSocket* client : m_players) {
		client->write(message);
	}
}

void Server::log(QString msg)
{
	qDebug() << "[Server]: " << msg;
}

void Server::readyRead()
{
	QTcpSocket *client = (QTcpSocket*)sender();
	QByteArray data = client->readAll();

	if (data == "UP") { sendToAll(m_mapWorker->getMovementResponse(client, playerMovements::sup)); }
	if (data == "DOWN") { sendToAll(m_mapWorker->getMovementResponse(client, playerMovements::sdown)); }
	if (data == "LEFT") { sendToAll(m_mapWorker->getMovementResponse(client, playerMovements::sleft)); }
	if (data == "RIGHT") { sendToAll(m_mapWorker->getMovementResponse(client, playerMovements::sright)); }
	if (data.indexOf("OPEN") != -1) sendToAll(m_mapWorker->processPlayerAction(client, actions::open, data.split(':')[1]));
	if (data.indexOf("CLOSE") != -1) sendToAll(m_mapWorker->processPlayerAction(client, actions::close, data.split(':')[1]));
	if (data == "ASKID") { client->write("ID" + m_mapWorker->getUserId(client)); }
}

void Server::disconnected()
{
	QTcpSocket *client = (QTcpSocket*)sender();
	log ("Client " + client->peerAddress().toString() + " disconnected!");

	disconnect (client, &QTcpSocket::readyRead, this, &Server::readyRead);
	disconnect (client, &QTcpSocket::disconnected, this, &Server::disconnected);

	m_players.remove(m_players.indexOf(client));
	sendToAll("DIS" + m_mapWorker->getUserId(client));
	m_mapWorker->removeUser(client);
}

void Server::incomingConnection(qintptr handle)
{
	QTcpSocket *client = new QTcpSocket();
	client->setSocketDescriptor(handle);

	m_players.append(client);
	m_mapWorker->addUser(client);

	log ("New connection from: " + client->peerAddress().toString() + "!");

	connect (client, &QTcpSocket::readyRead, this, &Server::readyRead);
	connect (client, &QTcpSocket::disconnected, this, &Server::disconnected);

	client->write("MAP_DATA" + m_mapWorker->getMap());
}