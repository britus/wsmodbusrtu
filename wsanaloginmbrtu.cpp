#include <QDebug>
#include <dlgadcindatatype.h>
#include <wsanaloginmbrtu.h>

WSAnalogInMbRtu::WSAnalogInMbRtu(MBRtuClient* modbus, QObject* parent)
    : WSModbusRtu {modbus, parent}
    , m_values()
{
    setDeviceAddress(1, false);
    setQueryInterval(1000);
}

WSAnalogInMbRtu::~WSAnalogInMbRtu()
{
}

/* -------------------------------------------------------
 * Api Methods
 * ------------------------------------------------------- */

const char* WSAnalogInMbRtu::id() const
{
    return "WMBADC:";
}

quint8 WSAnalogInMbRtu::maxInputs() const
{
    return 8;
}

quint8 WSAnalogInMbRtu::maxOutputs() const
{
    return 0;
}

float WSAnalogInMbRtu::channelValue(quint8 channel) const
{
    return m_values[channel];
}

QWidget* WSAnalogInMbRtu::settingsWidget(QWidget* parent)
{
    return new DlgAdcInDataType(this, parent);
}

WSAnalogInMbRtu::TChannelType WSAnalogInMbRtu::channelType(quint8 channel) const
{
    return m_types[channel];
}

void WSAnalogInMbRtu::setChannelTypes(const QMap<quint8, TChannelType>& types, bool updateDevice)
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Set all channel types. deviceUpdate:" << updateDevice;
    }

    if (types.count() != maxInputs()) {
        qCritical() << id() << "Invalid number of channel types. count:" << types.count();
        return;
    }

    m_types = types;

    if (updateDevice) {
        QByteArray data;
        data.append((quint8) 0x10);                  // 16bit Start address (0x1000) HI
        data.append((quint8) 0x00);                  // 16bit Start address (0x1000) LO
        data.append((quint8) 0x00);                  // 16bit Number of channels HI
        data.append((quint8) 0x08);                  // 16bit Number of channels LO
        data.append((quint8) (m_types.count() * 2)); // Set number of output bytes (two bytes per channel)

        for (quint8 i = 0; i < m_types.count(); i++) {
            data.append((quint8) 0x00);       // HI byte
            data.append((quint8) m_types[i]); // LO byte
        }

        send(
           WriteChannelTypes,
           deviceAddress(),
           QModbusRequest( //
              QModbusRequest::WriteMultipleRegisters,
              data));
    }

    for (quint8 i = 0; i < m_types.count(); i++) {
        emit channelChanged(i, m_types[i]);
    }
}

void WSAnalogInMbRtu::setChannelType(quint8 channel, const TChannelType type, bool updateDevice)
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Set single channel type. deviceUpdate:" << updateDevice;
    }

    if (channel >= maxInputs()) {
        qCritical() << id() << "Invalid channel number:" << channel;
        return;
    }

    m_types[channel] = type;

    if (updateDevice) {
        send(
           WriteChannelType,
           deviceAddress(),
           QModbusRequest(
              QModbusRequest::WriteSingleRegister,
              (quint16) (0x1000 + channel), // 16bit Register start address
              (quint8) 0x00,                // 16bit Channel data type HI
              (quint8) type)                // 16bit Channel data type LO
        );
    }

    emit channelChanged(channel, type);
}

/* -------------------------------------------------------
 * Protected Methods
 * ------------------------------------------------------- */

/* schedule inital queries */
void WSAnalogInMbRtu::doModbusOpened()
{
    scheduleFunction(ReadChannelTypes);
    scheduleFunction(ReadDataValues);
}

/* schedule status query */
void WSAnalogInMbRtu::startStatusWorker()
{
    scheduleFunction(ReadDataValues);
}

/* handle status query */
void WSAnalogInMbRtu::doFunction(uint function)
{
    switch (function) {
        case ReadChannelTypes: {
            readChannelTypes();
            break;
        }
        case ReadDataValues: {
            readDataValues();
            break;
        }
    }
}

bool WSAnalogInMbRtu::doMduInputRegisters(const QModbusDataUnit& unit)
{
    switch (function()) {
        case ReadDataValues: {
            if (checkValueCount(8, unit)) {
                for (uint i = 0; i < unit.valueCount(); i++) {
                    float value = unit.value(i);
                    // TODO: Dval to Volt: calculate something?
                    m_values[i] = value;
                    emit valueChanged(i, value);
                }
                return true;
            }
            break;
        }
    }

    return WSModbusRtu::doMduInputRegisters(unit);
}

bool WSAnalogInMbRtu::doMduHoldingRegisters(const QModbusDataUnit& unit)
{
    switch (function()) {
        case ReadChannelTypes: {
            if (checkValueCount(8, unit)) {
                for (uint i = 0; i < unit.valueCount(); i++) {
                    TChannelType type = //
                       static_cast<TChannelType>(unit.value(i));
                    m_types[i] = type;
                    emit channelChanged(i, type);
                }
                return true;
            }
            break;
        }
    }

    return WSModbusRtu::doMduHoldingRegisters(unit);
}

/* -------------------------------------------------------
 * Private Methods
 * ------------------------------------------------------- */

inline void WSAnalogInMbRtu::readChannelTypes()
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Read channel types";
    }

    send(
       ReadChannelTypes,
       deviceAddress(),
       QModbusRequest(
          QModbusRequest::ReadHoldingRegisters,
          (quint16) 0x1000, // 16bit Register start address
          (quint8) 0x00,    // 16bit Number of channels HI
          (quint8) 0x08)    // 16bit Number of channels LO
    );
}

inline void WSAnalogInMbRtu::readDataValues()
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Read data values";
    }

    send(
       ReadDataValues,
       deviceAddress(),
       QModbusRequest(
          QModbusRequest::ReadInputRegisters,
          (quint16) 0x0000, // 16bit Register start address
          (quint8) 0x00,    // 16bit Number of channels HI
          (quint8) 0x08)    // 16bit Number of channels LO
    );
}
