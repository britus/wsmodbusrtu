#include <QDebug>
#include <QTimer>
#include <dlgrelaylinkcontrol.h>
#include <wsrelaydiginmbrtu.h>

#define QUERY_STATUS_WITH_WORKER

WSRelayDigInMbRtu::WSRelayDigInMbRtu(MBRtuClient* modbus, QObject* parent)
    : WSModbusRtu {modbus, parent}
    , m_relays()
    , m_dinputs()
{
    setDeviceAddress(3, false);
    setQueryInterval(2000);
}

WSRelayDigInMbRtu::~WSRelayDigInMbRtu()
{
}

/* -------------------------------------------------------
 * Api Methods
 * ------------------------------------------------------- */

const char* WSRelayDigInMbRtu::id() const
{
    return "WRELAY:";
}

quint8 WSRelayDigInMbRtu::maxInputs() const
{
    return 8;
}

quint8 WSRelayDigInMbRtu::maxOutputs() const
{
    return 8;
}

QWidget* WSRelayDigInMbRtu::settingsWidget(QWidget* parent)
{
    return new DlgRelayLinkControl(this, parent);
}

void WSRelayDigInMbRtu::setRelayStatus(const quint8 relay, const bool state)
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Set relay:" << relay << state;
    }

    if (relay >= maxOutputs()) {
        qCritical() << id() << "Invalid relay number:" << relay;
        return;
    }

    send(
       (relay < 0xff ? UpdateRelay : WriteRelayStatus),
       deviceAddress(),
       QModbusRequest( //
          QModbusRequest::WriteSingleCoil,
          (quint16) (relay & 0x00ff),     // 16bit coil address
          (quint8) (state ? 0xff : 0x00), // 16bit state - byte HI
          (quint8) 0x00)                  // 16bit state - byte LO
    );
}

void WSRelayDigInMbRtu::setAllRelays(const quint8 mask)
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Set relay mask:" << Qt::hex << mask;
    }

    /* Save relay mask for later processing in response
     * event. This command does not reflect the 'mask'
     * in the modbus response unit. */
    setProperty("rmask", QVariant::fromValue(mask));

    send(
       WriteRelayMask,
       deviceAddress(),
       QModbusRequest( //
          QModbusRequest::WriteMultipleCoils,
          (quint16) 0x0000, // 16bit Relay Start Address
          (quint8) 0x00,    // 16bit Number of relays (Fixed: 0x00) HI
          (quint8) 0x08,    // 16bit Number of relays (Fixed: 0x08) LO
          (quint8) 0x01,    // 16bit Number of bit masks (Fixed 0x01)
          (quint8) mask)    // Relay bit mask
    );
}

void WSRelayDigInMbRtu::setControlModes(const QMap<quint8, TControlMode>& modes, bool updateDevice)
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Set relay control modes. deviceUpdate:" << updateDevice;
    }

    if (modes.count() != maxOutputs()) {
        qCritical() << id() << "Invalid number of control mode count:" << modes.count();
        return;
    }

    m_control = modes;

    if (updateDevice) {
        QByteArray data;
        data.append((quint8) 0x10);                    // 16bit Start address (0x1000) HI
        data.append((quint8) 0x00);                    // 16bit Start address (0x1000) LO
        data.append((quint8) 0x00);                    // 16bit Number of channels HI
        data.append((quint8) 0x08);                    // 16bit Number of channels LO
        data.append((quint8) (m_control.count() * 2)); // Set number of output bytes (two bytes per channel)

        for (quint8 i = 0; i < m_control.count(); i++) {
            data.append((quint8) 0x00);         // HI byte
            data.append((quint8) m_control[i]); // LO byte
        }

        send(
           WriteControlModes,
           deviceAddress(),
           QModbusRequest( //
              QModbusRequest::WriteMultipleRegisters,
              data));
    }

    for (quint8 i = 0; i < m_control.count(); i++) {
        emit modeChanged(i, m_control[i]);
    }
}

void WSRelayDigInMbRtu::setControlMode(quint8 relay, const TControlMode mode, bool updateDevice)
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Set single relay control mode. deviceUpdate:" << updateDevice;
    }

    if (relay >= maxOutputs()) {
        qCritical() << id() << "Invalid relay number:" << relay;
        return;
    }

    m_control[relay] = mode;

    if (updateDevice) {
        send(
           WriteControlMode,
           deviceAddress(),
           QModbusRequest(
              QModbusRequest::WriteSingleRegister,
              (quint16) (0x1000 + relay), // 16bit Register start address
              (quint8) 0x00,              // 16bit Control mode type HI
              (quint8) mode)              // 16bit Control mode type LO
        );
    }

    emit modeChanged(relay, mode);
}

WSRelayDigInMbRtu::TControlMode WSRelayDigInMbRtu::controlMode(const quint8 relay) const
{
    return m_control[relay];
}

bool WSRelayDigInMbRtu::relayStatus(const quint8 relay) const
{
    return m_relays[relay];
}

bool WSRelayDigInMbRtu::digitalInput(const quint8 channel) const
{
    return m_dinputs[channel];
}

/* -------------------------------------------------------
 * Protected Methods
 * ------------------------------------------------------- */

/* schedule inital queries */
void WSRelayDigInMbRtu::doModbusOpened()
{
    scheduleFunction(ReadControlMode);
    scheduleFunction(ReadRelayStatus);
    scheduleFunction(ReadDigitalInput);
}

/* schedule status queries */
void WSRelayDigInMbRtu::startStatusWorker()
{
    scheduleFunction(ReadDigitalInput);
    scheduleFunction(ReadRelayStatus);
}

/* handle status query */
void WSRelayDigInMbRtu::doFunction(uint function)
{
    switch (function) {
        case ReadControlMode: {
            readControlModes();
            break;
        }
        case ReadRelayStatus: {
            readRelayStatus();
            break;
        }
        case ReadDigitalInput: {
            readInputStatus();
            break;
        }
    }
}

bool WSRelayDigInMbRtu::doMduCoils(const QModbusDataUnit& unit)
{
    switch (function()) {
        case ReadRelayStatus: {
            for (uint i = 0; i < unit.valueCount(); i++) {
                bool state = (unit.value(i) == 1 ? true : false);
                m_relays[i] = state;
                emit relayChanged(i, state);
            }
            return true;
        }
    }

    return WSModbusRtu::doMduCoils(unit);
}

bool WSRelayDigInMbRtu::doMduDiscreteInputs(const QModbusDataUnit& unit)
{
    switch (function()) {
        case ReadDigitalInput: {
            for (uint i = 0; i < unit.valueCount(); i++) {
                bool state = (unit.value(i) == 1 ? true : false);
                m_dinputs[i] = state;
                emit inputChanged(i, state);
            }
            return true;
        }
    }

    return WSModbusRtu::doMduDiscreteInputs(unit);
}

bool WSRelayDigInMbRtu::doMduInputRegisters(const QModbusDataUnit& unit)
{
    switch (function()) {
        case UpdateRelay: {
            if (checkValueCount(2, unit)) {
                quint8 relay = unit.value(0);
                bool state = (unit.value(1) != 0);

                m_relays[relay] = state;
                emit relayChanged(relay, state);
                return true;
            }

            break;
        }
        case WriteRelayStatus: {
            if (checkValueCount(2, unit)) {
                quint16 relay = unit.value(0);
                bool state = (unit.value(1) != 0);

                m_relays[relay] = state;
                emit relayChanged(relay, state);

                for (int i = 0; i < maxOutputs(); i++) {
                    m_relays[i] = state;
                    emit relayChanged(i, state);
                }
                return true;
            }

            break;
        }
        case WriteRelayMask: {
            if (checkValueCount(2, unit)) {
                QVariant rmask = property("rmask");
                if (!rmask.isNull() && rmask.isValid()) {
                    quint8 mask = rmask.value<quint8>();
                    /* sync local state map before send!! */
                    for (quint8 b = 0; b < maxOutputs(); b++) {
                        if ((mask & (1 << b)) != 0) {
                            m_relays[b] = true;
                        }
                        else {
                            m_relays[b] = false;
                        }
                        emit relayChanged(b, m_relays[b]);
                    }
                    return true;
                }
            }

            break;
        }
    }

    return WSModbusRtu::doMduInputRegisters(unit);
}

bool WSRelayDigInMbRtu::doMduHoldingRegisters(const QModbusDataUnit& unit)
{
    switch (function()) {
        case ReadControlMode: {
            if (checkValueCount(maxOutputs(), unit)) {
                for (uint i = 0; i < unit.valueCount(); i++) {
                    TControlMode mode = //
                       static_cast<TControlMode>(unit.value(i));
                    m_control[i] = mode;

                    emit modeChanged(i, mode);
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

/* Query Relay ON / OFF Status */
inline void WSRelayDigInMbRtu::readRelayStatus()
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Read Relay Status";
    }

    send(
       ReadRelayStatus,
       deviceAddress(),
       QModbusRequest(
          QModbusRequest::ReadCoils,
          (quint16) 0x0000, // 16bit Relay Start Address
          (quint8) 0x00,    // 16bit Number of relays HI
          (quint8) 0x08)    // 16bit Number of relays LO
    );
}

/* Query Relay Control Status */
inline void WSRelayDigInMbRtu::readControlModes()
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Read Relay Control Modes";
    }

    send(
       ReadControlMode,
       deviceAddress(),
       QModbusRequest(
          QModbusRequest::ReadHoldingRegisters,
          (quint16) 0x1000, // 16bit Relay Start Address
          (quint8) 0x00,    // 16bit Number of relays HI
          (quint8) 0x08)    // 16bit Number of relays LO
    );
}

/* Query Digital Input Status */
inline void WSRelayDigInMbRtu::readInputStatus()
{
    if (isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Read Digital Input";
    }

    send(
       ReadDigitalInput,
       deviceAddress(),
       QModbusRequest(
          QModbusRequest::ReadDiscreteInputs,
          (quint16) 0x0000, // 16bit Digitial Input Start Address
          (quint8) 0x00,    // 16bit Number of inputs HI
          (quint8) 0x08)    // 16bit Number of inputs LO
    );
}
