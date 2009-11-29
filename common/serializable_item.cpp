#include "serializable_item.h"
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

QHash<QString, SerializableItem::NewItemFunction> SerializableItem::itemNameHash;

SerializableItem *SerializableItem::getNewItem(const QString &name)
{
	if (!itemNameHash.contains(name))
		return 0;
	return itemNameHash.value(name)();
}

void SerializableItem::registerSerializableItem(const QString &name, NewItemFunction func)
{
	itemNameHash.insert(name, func);
}

bool SerializableItem::read(QXmlStreamReader *xml)
{
	while (!xml->atEnd()) {
		xml->readNext();
		readElement(xml);
		if (xml->isEndElement() && (xml->name() == itemType))
			return true;
	}
	return false;
}

void SerializableItem::write(QXmlStreamWriter *xml)
{
	xml->writeStartElement(itemType);
	if (!itemSubType.isEmpty())
		xml->writeAttribute("type", itemSubType);
	writeElement(xml);
	xml->writeEndElement();
}

SerializableItem_Map::~SerializableItem_Map()
{
	QMapIterator<QString, SerializableItem *> mapIterator(itemMap);
	while (mapIterator.hasNext())
		delete mapIterator.next().value();
	for (int i = 0; i < itemList.size(); ++i)
		delete itemList[i];
}

void SerializableItem_Map::readElement(QXmlStreamReader *xml)
{
	if (currentItem) {
		if (currentItem->read(xml))
			currentItem = 0;
	} else if (xml->isEndElement() && (xml->name() == itemType))
		extractData();
	else if (xml->isStartElement()) {
		QString childName = xml->name().toString();
		QString childSubType = xml->attributes().value("type").toString();
		qDebug() << "Map: started new item, name=" << childName << "subtype=" << childSubType;
		currentItem = itemMap.value(childName);
		if (!currentItem) {
			qDebug() << "Item not found in map";
			currentItem = getNewItem(childName + childSubType);
			itemList.append(currentItem);
			if (!currentItem) {
				qDebug() << "Item still not found";
				currentItem = new SerializableItem_Invalid(childName);
			}
		}
		if (currentItem->read(xml))
			currentItem = 0;
	}
}

void SerializableItem_Map::writeElement(QXmlStreamWriter *xml)
{
	QMapIterator<QString, SerializableItem *> mapIterator(itemMap);
	while (mapIterator.hasNext())
		mapIterator.next().value()->write(xml);
	for (int i = 0; i < itemList.size(); ++i)
		itemList[i]->write(xml);
}

void SerializableItem_String::readElement(QXmlStreamReader *xml)
{
	if (xml->isCharacters() && !xml->isWhitespace())
		data = xml->text().toString();
}

void SerializableItem_String::writeElement(QXmlStreamWriter *xml)
{
	xml->writeCharacters(data);
}

void SerializableItem_Int::readElement(QXmlStreamReader *xml)
{
	if (xml->isCharacters() && !xml->isWhitespace()) {
		bool ok;
		data = xml->text().toString().toInt(&ok);
		if (!ok)
			data = -1;
	}
}

void SerializableItem_Int::writeElement(QXmlStreamWriter *xml)
{
	xml->writeCharacters(QString::number(data));
}

void SerializableItem_Bool::readElement(QXmlStreamReader *xml)
{
	if (xml->isCharacters() && !xml->isWhitespace())
		data = xml->text().toString() == "1";
}

void SerializableItem_Bool::writeElement(QXmlStreamWriter *xml)
{
	xml->writeCharacters(data ? "1" : "0");
}

int SerializableItem_Color::colorToInt(const QColor &color) const
{
	return color.red() * 65536 + color.green() * 256 + color.blue();
}

QColor SerializableItem_Color::colorFromInt(int colorValue) const
{
	return QColor(colorValue / 65536, (colorValue % 65536) / 256, colorValue % 256);
}

void SerializableItem_Color::readElement(QXmlStreamReader *xml)
{
	if (xml->isCharacters() && !xml->isWhitespace()) {
		bool ok;
		int colorValue = xml->text().toString().toInt(&ok);
		data = ok ? colorFromInt(colorValue) : Qt::black;
	}
}

void SerializableItem_Color::writeElement(QXmlStreamWriter *xml)
{
	xml->writeCharacters(QString::number(colorToInt(data)));
}

void SerializableItem_DateTime::readElement(QXmlStreamReader *xml)
{
	if (xml->isCharacters() && !xml->isWhitespace()) {
		bool ok;
		unsigned int dateTimeValue = xml->text().toString().toUInt(&ok);
		data = ok ? QDateTime::fromTime_t(dateTimeValue) : QDateTime();
	}
}

void SerializableItem_DateTime::writeElement(QXmlStreamWriter *xml)
{
	xml->writeCharacters(QString::number(data.toTime_t()));
}
