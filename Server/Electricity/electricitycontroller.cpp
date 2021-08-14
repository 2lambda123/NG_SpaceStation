#include "electricitycontroller.h"

ElectricityController::ElectricityController(QObject *parent) : QObject(parent)
{
	loadMap();
}

void ElectricityController::addGenerator(position pos)
{
	ElectricGenerator *generator = new ElectricGenerator();
	m_generators[generator] = pos;
    m_wireMap[pos.y][pos.x] = 'g';

    connect (generator, &ElectricGenerator::stateChanged, this, &ElectricityController::updatedGeneratorState);
    emit updateGeneratorState(pos, "STOPPED");

    emit requiredElectricityUpdate();
}

void ElectricityController::removeGenerator(position pos)
{
    for (ElectricGenerator *generator : m_generators.keys()) {
        if (m_generators[generator] == pos) {
            m_generators.remove(generator);
            m_wireMap[pos.y][pos.x] = '.';
            delete generator;
        }
    }
    emit requiredElectricityUpdate();
}

void ElectricityController::addNode(position pos)
{
	ElectricNode *node = new ElectricNode();
	m_nodes[pos] = node;
    m_wireMap[pos.y][pos.x] = 'n';
    emit requiredElectricityUpdate();
}

void ElectricityController::removeNode(position pos)
{
    if (m_nodes.contains(pos) == false) return;
    ElectricNode *node = m_nodes[pos];
    m_nodes.remove(pos);
    m_wireMap[pos.y][pos.x] = '.';
    delete node;
}

void ElectricityController::addWire(position pos)
{
    m_wireMap[pos.y][pos.x] = 'w';
    emit requiredElectricityUpdate();
}

void ElectricityController::removeWire(position pos)
{
    if (m_wireMap[pos.y][pos.x] == 'w') {
        m_wireMap[pos.y][pos.x] = '.';
        emit requiredElectricityUpdate();
    }
}

QByteArray ElectricityController::getNewPlayerInfo()
{
    QByteArray responce;
    for (ElectricGenerator *generator : m_generators.keys()) {
        position pos = m_generators[generator];
        responce.append("GEN:" + QByteArray::number(pos.x) + ":" + QByteArray::number(pos.y) + ":" + generator->getGeneratorState() + "|");
    }
    return responce;
}

void ElectricityController::processElectricityLines()
{
	for (ElectricGenerator *generator : m_generators.keys()) {
		generator->setRequiredPower(inspectLine(m_generators[generator], position(0,0), generator->isWorking()));
    }
}

void ElectricityController::updatedGeneratorState(QByteArray state)
{
    ElectricGenerator *generator = (ElectricGenerator*)sender();
    emit updateGeneratorState(m_generators[generator], state);
    processElectricityLines();
}

void ElectricityController::loadMap()
{
	QByteArray map;
	QFile mapFile("electricityMap.txt");

	if (mapFile.open(QIODevice::ReadOnly) == false) {
		log ("Error opening file: " + mapFile.errorString());
		return;
	}

	map = mapFile.readAll();
	mapFile.close();
	processMap(&map);
    log ("Loaded map (" + QString::number(map.size()) + " bytes)");
    connect (this, &ElectricityController::requiredElectricityUpdate, this, &ElectricityController::processElectricityLines);
}

bool ElectricityController::checkWireExist(position pos)
{
    if (m_wireMap[pos.y][pos.x] == 'w') return true;
    return false;
}

void ElectricityController::processMap(QByteArray *map)
{
	QList<QByteArray> rows = (*map).split('\n');
	int rowNumber = 0;
	for (QByteArray line : rows) {
		int length = 0;
		QVector<char> buffer;
		for (char cell : line) {
			buffer.push_back(cell);
		}
		m_wireMap.push_back(buffer);
        for (char cell : line) {
            if (cell == 'g')
                addGenerator(position(length, rowNumber));
            if (cell == 'n')
                addNode(position(length, rowNumber));
            length++;
        }
        rowNumber++;
	}
}

int ElectricityController::inspectLine(position pos, position previous, bool powered)
{
	// Check here any technic exist
	updateNodeState(pos, powered);
	int powerRequired = 0;

	if (checkSideLine(position(pos.x + 1, pos.y), previous)) powerRequired+=inspectLine(position(pos.x + 1, pos.y), pos, powered);
	if (checkSideLine(position(pos.x - 1, pos.y), previous)) powerRequired+=inspectLine(position(pos.x - 1, pos.y), pos, powered);
	if (checkSideLine(position(pos.x, pos.y + 1), previous)) powerRequired+=inspectLine(position(pos.x, pos.y + 1), pos, powered);
	if (checkSideLine(position(pos.x, pos.y - 1), previous)) powerRequired+=inspectLine(position(pos.x, pos.y - 1), pos, powered);
	return powerRequired;
}

bool ElectricityController::checkSideLine(position pos, position previous)
{
	if (pos == previous) return false;
	return checkLineExist(pos);

}

bool ElectricityController::checkLineExist(position pos)
{
	char item = m_wireMap[pos.y][pos.x] ;
	if (item == 'w' || item == 'n')
		return true;
	return false;
}

void ElectricityController::updateNodeState(position pos, bool state)
{
	if (m_nodes.contains(pos)) {
		m_nodes[pos]->setPowered(state);
	}
}

void ElectricityController::log(QString msg)
{
	qDebug() << "[ElectricityController]: " << msg;
}
