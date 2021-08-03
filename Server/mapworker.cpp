#include "mapworker.h"

MapWorker::MapWorker(ItemLoader *loader, HealthControl *health)
{
	m_itemController = new ItemController();
	m_itemLoader = loader;
	m_healthController = health;
}

void MapWorker::processMap(QByteArray mapData)
{
	m_mapData = mapData;
	m_map.clear();

	QList<QByteArray> rows = mapData.split('\n');
	for (QByteArray line : rows) {
		int length = 0;
		QVector<char> buffer;
		for (char cell : line) {
			buffer.push_back(cell);
			length++;
		}
		m_map.push_back(buffer);
	}

    log ("Map loading done! (" + QByteArray::number(mapData.size()) + " bytes)");
}

void MapWorker::processPlayerPush(QTcpSocket *buffer, actions act, QString direction)
{
	playerMovements side = Utilities::getSideFromString(direction);
	if (act == actions::push) pushPlayer(side,buffer);
}

void MapWorker::processPlayerKick(QTcpSocket *buffer, QString direction)
{
	playerMovements side = Utilities::getSideFromString(direction);
	position playerToPushCords=Utilities::getCoordsBySide(m_playerPositions[buffer],side);
	QTcpSocket *playerToPush=getPlayerByPosition(playerToPushCords);

	if (playerToPush == nullptr) return;

    QByteArray id=m_inventoryController->getWear(playerWearable::holdable, buffer);

	int damage=1;
    if(id.isEmpty()==false){
        damage=m_itemLoader->getItemById(id).getDamage();
    }

	m_healthController->makeDamage(playerToPush,damage);
	m_itemController->addItem(playerToPushCords, "24"); // blood id
	emit sendToPlayer(playerToPush, "HEALTH:" + QByteArray::number(m_healthController->getHealth(playerToPush)) + "|");
	emit sendToAll("IPLACE:" + QByteArray::number(playerToPushCords.x) + ":" + QByteArray::number(playerToPushCords.y) + ":24|");
	pushPlayer(side,buffer);
}


bool MapWorker::checkMovementPosition(position pos)
{
	bool conditionIsFloor =
			m_map[pos.y][pos.x] == '.' ||
			m_map[pos.y][pos.x] == 'o' ||
			m_map[pos.y][pos.x] == '_';

	bool conditionNoItemsOnTheWay = m_itemLoader->getItemById(m_itemController->getItemIdByPos(pos)).getType() != itemType::furniture;

	return conditionIsFloor && conditionNoItemsOnTheWay;
}

QTcpSocket *MapWorker::getPlayerByPosition(position pos)
{
    for(QTcpSocket* buffer : m_playerPositions.keys()){
        if(m_playerPositions[buffer].x==pos.x && m_playerPositions[buffer].y==pos.y){
            return buffer;
        }
    }
    return nullptr;
}

void MapWorker::pushPlayer(playerMovements side, QTcpSocket* buffer)
{
	if(buffer==nullptr) return;

    position pushes = m_playerPositions[buffer];
	position playerToPushCords=Utilities::getCoordsBySide(pushes,side);
	QTcpSocket *playerToPush=getPlayerByPosition(playerToPushCords);

	if(playerToPush==nullptr) return;

	movePlayer(playerToPush, side);
}

void MapWorker::addUser(QTcpSocket *socket, position pos)
{
	m_playerPositions[socket] = pos;
	m_playerIds[socket] = Utilities::generateId();
}

void MapWorker::movePlayer(QTcpSocket *socket, playerMovements side)
{
	position pos = m_playerPositions[socket];

	switch (side) {
		case playerMovements::sup: pos.y--; break;
		case playerMovements::sdown: pos.y++; break;
		case playerMovements::sleft: pos.x--; break;
		case playerMovements::sright: pos.x++; break;
		default: log ("Unknown side passed to movePlayer");
	}

	processPlayerMovement(pos, socket);
}

void MapWorker::updatePlayerPos(QTcpSocket* socket, position pos)
{
	m_playerPositions[socket] = pos;
}

void MapWorker::processPlayerAction(QTcpSocket *socket, actions act, QString direction)
{
	position pos = m_playerPositions[socket];
	playerMovements side = Utilities::getSideFromString(direction);
	position actPos = Utilities::getCoordsBySide(pos, side);

	switch (act) {
		case actions::open: emit sendToAll(formatMapChange(actPos, processOpen(actPos))); break;
		case actions::close: emit sendToAll(formatMapChange(actPos, processClose(actPos))); break;
		default: log("Unknown action passed to processPlayerAction function");
	}
}

void MapWorker::processPlayerMovement(position pos, QTcpSocket *socket)
{
	if (checkMovementPosition(pos)) {
		updatePlayerPos(socket, pos);
		emit sendToAll(formatResponce(pos, socket));
	}
}

void MapWorker::updateMapData(position pos, char object)
{
	int offset = 0;
	for (int row=0; row<pos.y; row++) {
		offset += m_map[row].size() + 1;
	}
	offset += pos.x;
	m_mapData[offset] = object;
}

QByteArray MapWorker::formatMapChange(position pos, char object)
{
	if (m_map[pos.y][pos.x] == object) {
		return "";
	}

	m_map[pos.y][pos.x] = object;
	updateMapData(pos, object);
	return QByteArray("CHG:" + QByteArray::number(pos.x) + ":" + QByteArray::number(pos.y) + ":" + object + "|");
}

QByteArray MapWorker::formatResponce(position pos, QTcpSocket *socket)
{
	return "POS:" + m_playerIds[socket] + ":" + QByteArray::number(pos.x) + ":" + QByteArray::number(pos.y) + "|";
}

char MapWorker::processOpen(position pos)
{
	switch(m_map[pos.y][pos.x]) {
		case 'c': return 'o';
		case 't': return 'T';
		case 's': return 'S';
		default: return m_map[pos.y][pos.x];
	}
}

char MapWorker::processClose(position pos)
{
	switch(m_map[pos.y][pos.x]) {
		case 'o': return 'c';
		case 'T': return 't';
		case 'S': return 's';
		default: return m_map[pos.y][pos.x];
	}
}

void MapWorker::processDrop(QTcpSocket *socket, QByteArray data, QByteArray bside)
{
	position pos = m_playerPositions[socket];
	playerMovements side = Utilities::getSideFromString(bside);
	position actPos = Utilities::getCoordsBySide(pos, side);
	return dropItem(data, actPos, socket);
}

void MapWorker::processPick(QTcpSocket *socket, QString data)
{
	position pos = m_playerPositions[socket];
	playerMovements side = Utilities::getSideFromString(data);
	position actPos = Utilities::getCoordsBySide(pos, side);

	pickItem(actPos, socket);
}

void MapWorker::pickItem(position pos, QTcpSocket *player)
{
	int itemNumber = 0;
	itemType type = itemType::notype;
	QByteArray id;
	do {
		id = m_itemController->getItemIdByPos(pos, itemNumber);
		if (id.isEmpty())
			break;
		type = m_itemLoader->getItemById(id).getType();
		itemNumber++;
	} while (type == itemType::furniture);

	if (id.isEmpty() == false && type != itemType::furniture && type != itemType::notype) {
		m_inventoryController->addItemToInventory(player, id);
		m_itemController->deleteItem(pos, id);
		emit sendToPlayer(player, "PITEM:" + id + "|");
		emit sendToAll("ICLEAR:" + QByteArray::number(pos.x) + ":" + QByteArray::number(pos.y) + ":" + id + "|");
	}
}

void MapWorker::dropItem(QByteArray id, position pos, QTcpSocket *player)
{
	if (id.isEmpty() == false && m_itemLoader->checkIdExist(id)) {
		m_inventoryController->removeItemFromInventory(player, id);
		m_itemController->addItem(pos, id);
		emit sendToPlayer(player, "DITEM:" + id + "|");
		emit sendToAll("IPLACE:" + QByteArray::number(pos.x) + ":" + QByteArray::number(pos.y) + ":" + id + "|");
		m_inventoryController->takeOff(id, player);
	}
}

void MapWorker::log(QString msg)
{
	qDebug() << "[MapWorker]: " << msg;
}
