#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QTcpSocket>
#include <QObject>

enum playerMovement {
	moveUp,
	moveDown,
	moveLeft,
	moveRight
};

class ConnectionManager : public QObject
{
	Q_OBJECT
public:
	explicit ConnectionManager();
	void connectToServer(QString ip, QString port);
	QByteArray getMap() { return m_map; }

	void movePlayer(playerMovement side);
	void actionPlayer(QString action, int side);

private:
	void log(QString msg);

private slots:
	void connectedToServer();
	void socketReady();

signals:
	void dataReady();
	void connected();
	void gotMap();
	void playerPosition(int x, int y);
	void message(QString message);
	void mapChanged(int x, int y, char object);

private:
	QTcpSocket *m_socket;
	QByteArray m_map;

};

#endif // CONNECTIONMANAGER_H
