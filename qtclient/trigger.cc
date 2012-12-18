#include <iterator>

#include "trigger.h"
#include "layout.h"
#include "hidtables.h"

template <typename T>
static T unaligned_read(const char *p) {
	T x;
	memcpy(&x, p, sizeof(x));
	return x;
}

QList<Trigger> Trigger::readTriggers(const QByteArray& index,
                                     const QByteArray& rawData,
                                     unsigned int maxKeys)
{
	const QByteArray data =
		rawData.mid(sizeof(uint16_t)); // skip free pointer

	QList<Trigger> triggers;
	int idxOff = 0;
	while (uint8_t(index.at(idxOff)) != 0xff && idxOff < index.length()) {
		Trigger t;
		QSet<uint8_t> triggerSet;
		for (unsigned int key = 0; key < maxKeys; key++) {
			uint8_t keyValue = index.at(idxOff + key);
			if (keyValue == 0xff)
				break;
			triggerSet << keyValue;
		}
		t.setTriggerSet(triggerSet);

		uint16_t dataOffset = unaligned_read<uint16_t>(
			index.constData() + idxOff + maxKeys);
		bool isProgram = !!(dataOffset & 0x8000);
		dataOffset &= 0x7fff;

		if (isProgram) {
			t.setType(Trigger::Program);
			t.setProgram(dataOffset);;
		}
		else {
			t.setType(Trigger::Macro);
			uint16_t macroLength = unaligned_read<uint16_t>(
				data.constData() + dataOffset);
			t.setMacro(
				data.mid(dataOffset + sizeof(uint16_t), macroLength));
		}
		triggers << t;
		idxOff += maxKeys + sizeof(uint16_t);
	}
	return triggers;
}

QString Trigger::nameType(TriggerType t) {
	switch (t) {
	case Trigger::Program:
		return "Program";
	case Trigger::Macro:
		return "Macro";
	default:
		return "Invalid Type";
	}
}

QString Trigger::formatMacro(const QByteArray& macro) {
	QString formatted;
	QSet<uint8_t> downKeys;
	for (QByteArray::const_iterator it = macro.constBegin();
		 it != macro.constEnd();
		 ++it)
	{
		if (formatted.length() != 0)
			formatted += " ";

		if (downKeys.contains(*it)) {
			downKeys -= *it;
			formatted += "-";
		}
		else {
			downKeys += *it;
			formatted += "+";
		}
		formatted += HIDTables::nameUsage(*it);
	}
	return formatted;
}

QString Trigger::formatTriggerSet(const Layout& layout,
                                  const QSet<uint8_t>& keys)
{
	QList<uint8_t> keyList;
	keyList.reserve(keys.count());
	qCopy(keys.constBegin(), keys.constEnd(),
	      std::back_inserter(keyList));
	qSort(keyList.begin(), keyList.end(), qGreater<uint8_t>());

	QString formatted;
	for (QList<uint8_t>::const_iterator it = keyList.constBegin();
	     it != keyList.constEnd();
	     ++it)
	{
		if (formatted.length() != 0)
			formatted += " ";
		formatted += layout.namePosition(*it);
	}
	return formatted;
}
