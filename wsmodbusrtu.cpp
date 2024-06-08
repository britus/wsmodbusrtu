/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.escrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#include <QDebug>
#include <wsmodbusrtu.h>

#define QUERY_STATUS_WITH_WORKER

#define NULL_MBO_MSG "Modbus NULL pointer object!"
#define CHECK_MODBUS(m)                                 \
    do {                                                \
        Q_ASSERT_X(m != 0L, Q_FUNC_INFO, NULL_MBO_MSG); \
    } while (false)

WSModbusRtu::WSModbusRtu(MBRtuClient* modbus, QObject* parent)
    : QObject {parent}
    , m_modbus(modbus)
    , m_fwVersion(0)
    , m_address(1)
    , m_function(RtuUnspecified)
    , m_funcQueue()
    , m_timer(nullptr)
{
    CHECK_MODBUS(m_modbus);
    connect(m_modbus, &MBRtuClient::opened, this, &WSModbusRtu::onModbusOpened);
    connect(m_modbus, &MBRtuClient::closed, this, &WSModbusRtu::onModbusClosed);
    connect(m_modbus, &MBRtuClient::received, this, &WSModbusRtu::onModbusReceived);
    connect(m_modbus, &MBRtuClient::complete, this, &WSModbusRtu::onModbusComplete);
    connect(m_modbus, &MBRtuClient::error, this, &WSModbusRtu::onModbusError);
}

WSModbusRtu::~WSModbusRtu()
{
    CHECK_MODBUS(m_modbus);
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        m_timer->deleteLater();
    }
    if (m_modbus->isOpen()) {
        m_modbus->close();
    }
}

/* -------------------------------------------------------
 * Api Methods
 * ------------------------------------------------------- */

void WSModbusRtu::open()
{
    Q_ASSERT_X(m_modbus != 0L, Q_FUNC_INFO, "Null pointer modbus object!");
    if (!m_modbus->isOpen()) {
        m_modbus->open();
    }
    else {
        onModbusOpened();
    }
}

void WSModbusRtu::close()
{
    CHECK_MODBUS(m_modbus);
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        m_timer->deleteLater();
    }
    if (m_modbus->isOpen()) {
        m_modbus->close();
    }
    else {
        onModbusClosed();
    }
}

const quint16& WSModbusRtu::firmwareVersion() const
{
    return m_fwVersion;
}

const uint& WSModbusRtu::queryInterval() const
{
    return m_interval;
}

void WSModbusRtu::setQueryInterval(uint interval)
{
    if (m_interval != interval) {
        m_interval = interval;
        /* notify consumer */
        emit intervalChanged(m_interval);
    }
}

const uint& WSModbusRtu::function() const
{
    return m_function;
}

void WSModbusRtu::scheduleFunction(const uint function)
{
    m_funcQueue.append(function);
}

const quint8& WSModbusRtu::deviceAddress() const
{
    return m_address;
}

void WSModbusRtu::setDeviceAddress(const quint8 address, const bool updateDevice)
{
    if (m_address != address) {
        if (!updateDevice) {
            m_address = address;
            emit addressChanged(m_address);
            return;
        }
        CHECK_MODBUS(m_modbus);
        if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
            qDebug() << id() << "Set Device Address:" << address;
        }
        setFunction(RtuWriteDeviceAddr);
        m_modbus->send(
           deviceAddress(),
           QModbusRequest( //
              QModbusRequest::WriteSingleRegister,
              (quint16) 0x4000, // 16bit startAddress [ Coil ]
              (quint8) 0x00,    // 16bit address - byte HI
              (quint8) address) // 16bit address - byte LO
        );
    }
}

const QString& WSModbusRtu::portName() const
{
    CHECK_MODBUS(m_modbus);
    return m_modbus->portName();
}

void WSModbusRtu::setPortName(const QString& name)
{
    CHECK_MODBUS(m_modbus);
    m_modbus->setPortName(name);
}

const QSerialPort::BaudRate& WSModbusRtu::baudRate() const
{
    CHECK_MODBUS(m_modbus);
    return m_modbus->baudRate();
}

void WSModbusRtu::setBaudRate(const QSerialPort::BaudRate rate, const bool updateDevice)
{
    CHECK_MODBUS(m_modbus);
    if (m_modbus->baudRate() != rate) {
        if (!updateDevice) {
            m_modbus->setBaudRate(rate);
            return;
        }
        setDeviceUartParams(rate, m_modbus->parity());
    }
}

const QSerialPort::Parity& WSModbusRtu::parity() const
{
    CHECK_MODBUS(m_modbus);
    return m_modbus->parity();
}

void WSModbusRtu::setParity(const QSerialPort::Parity parity, const bool updateDevice)
{
    CHECK_MODBUS(m_modbus);
    if (m_modbus->parity() != parity) {
        if (!updateDevice) {
            m_modbus->setParity(parity);
            return;
        }
        setDeviceUartParams(m_modbus->baudRate(), parity);
    }
}

const QSerialPort::DataBits& WSModbusRtu::dataBits() const
{
    CHECK_MODBUS(m_modbus);
    return m_modbus->dataBits();
}

void WSModbusRtu::setDataBits(const QSerialPort::DataBits bits)
{
    CHECK_MODBUS(m_modbus);
    m_modbus->setDataBits(bits);
}

const QSerialPort::StopBits& WSModbusRtu::stopBits() const
{
    CHECK_MODBUS(m_modbus);
    return m_modbus->stopBits();
}

void WSModbusRtu::setStopBits(const QSerialPort::StopBits bits)
{
    CHECK_MODBUS(m_modbus);
    m_modbus->setStopBits(bits);
}

bool WSModbusRtu::isValidModbus() const
{
    CHECK_MODBUS(m_modbus);
    return m_modbus->isOpen();
}

/* -------------------------------------------------------
 * Protected Methods
 * ------------------------------------------------------- */

bool WSModbusRtu::isTrace(uint mask) const
{
    CHECK_MODBUS(m_modbus);
    return m_modbus->isTrace(mask);
}

bool WSModbusRtu::isFunctionQueueEmpty() const
{
    return m_funcQueue.isEmpty();
}

void WSModbusRtu::resetFunctionQueue()
{
    m_funcQueue.clear();
}

uint WSModbusRtu::takeFirstFunction()
{
    if (m_funcQueue.isEmpty()) {
        return RtuUnspecified;
    }
    return m_funcQueue.takeFirst();
}

void WSModbusRtu::startStatusWorker()
{
    /* nothing here, may overridden */
}

void WSModbusRtu::send(uint function, quint8 device, const QModbusRequest& mr)
{
    CHECK_MODBUS(m_modbus);
    setFunction(function);
    m_modbus->send(device, mr);
}

void WSModbusRtu::read(uint function, quint8 device, const QModbusDataUnit& du)
{
    CHECK_MODBUS(m_modbus);
    setFunction(function);
    m_modbus->read(device, du);
}

void WSModbusRtu::write(uint function, quint8 device, const QModbusDataUnit& du)
{
    CHECK_MODBUS(m_modbus);
    setFunction(function);
    m_modbus->write(device, du);
}

bool WSModbusRtu::checkValueCount(const uint count, const QModbusDataUnit& unit)
{
    if (count != unit.valueCount()) {
        qWarning() << id() << "Invalid number of values in function:" //
                   << function() << "count:" << count;
        return false;
    }
    return true;
}

void WSModbusRtu::readVersion()
{
    CHECK_MODBUS(m_modbus);
    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Read Version";
    }

    setFunction(RtuReadVersion);
    m_modbus->send(
       deviceAddress(),
       QModbusRequest(
          QModbusRequest::ReadHoldingRegisters,
          (quint16) 0x8000, // 16bit Command Register 'FW Version'
          (quint8) 0x00,    // 16bit Fixed register value HI
          (quint8) 0x01)    // 16bit Fixed register value LO
    );
}

void WSModbusRtu::readDeviceAddress()
{
    CHECK_MODBUS(m_modbus);
    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Read Device Address";
    }

    setFunction(RtuReadDeviceAddr);
    m_modbus->send(
       deviceAddress(),
       QModbusRequest(
          QModbusRequest::ReadHoldingRegisters,
          (quint16) 0x4000, // 16bit Command Register 'Device Address'
          (quint8) 0x00,    // 16bit Fixed register value HI
          (quint8) 0x01)    // 16bit Fixed register value LO
    );
}

bool WSModbusRtu::doMduCoils(const QModbusDataUnit&)
{
    return false;
}

bool WSModbusRtu::doMduDiscreteInputs(const QModbusDataUnit&)
{
    return false;
}

bool WSModbusRtu::doMduInputRegisters(const QModbusDataUnit& unit)
{
    switch (function()) {
        case RtuWriteDeviceAddr: {
            if (checkValueCount(2, unit) && unit.value(0) == 0x4000) {
                /* set to object instance only */
                setDeviceAddress(unit.value(1), false);
            }
            return true;
        }
    }
    return false;
}

bool WSModbusRtu::doMduHoldingRegisters(const QModbusDataUnit& unit)
{
    switch (function()) {
        case RtuReadDeviceAddr: {
            if (checkValueCount(1, unit)) {
                /* set to object instance only */
                setDeviceAddress(unit.value(0), false);
            }
            return true;
        }
        case RtuReadVersion: {
            if (checkValueCount(1, unit)) {
                m_fwVersion = unit.value(0);
            }
            return true;
        }
    }
    return false;
}

void WSModbusRtu::doModbusOpened()
{
}

void WSModbusRtu::doModbusClosed()
{
}

void WSModbusRtu::doModbusError(const int, const QString&)
{
}

void WSModbusRtu::doComplete(uint function)
{
    setFunction(function);
}

void WSModbusRtu::doFunction(uint)
{
}

/* -------------------------------------------------------
 * Private Methods
 * ------------------------------------------------------- */

inline void WSModbusRtu::setFunction(const uint function)
{
    m_function = function;
}

inline void WSModbusRtu::setDeviceUartParams( //
   const QSerialPort::BaudRate baud,
   const QSerialPort::Parity parity)
{
    CHECK_MODBUS(m_modbus);
    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Set device UART parameters";
    }

    quint8 parval = 0x00;  // default: None
    quint8 baudval = 0x01; // default: 9600

    switch (parity) {
        case QSerialPort::Parity::NoParity: {
            parval = 0x00;
            break;
        }
        case QSerialPort::Parity::EvenParity: {
            parval = 0x01;
            break;
        }
        case QSerialPort::Parity::OddParity: {
            parval = 0x02;
            break;
        }
        default: {
            qWarning() << id() << "Unsupported UART parameter. Parity:" << parity;
            return;
        }
    }

    switch (baud) {
        case QSerialPort::BaudRate::Baud4800: {
            baudval = 0x00;
            break;
        }
        case QSerialPort::BaudRate::Baud9600: {
            baudval = 0x01;
            break;
        }
        case QSerialPort::BaudRate::Baud19200: {
            baudval = 0x02;
            break;
        }
        case QSerialPort::BaudRate::Baud38400: {
            baudval = 0x03;
            break;
        }
        case QSerialPort::BaudRate::Baud57600: {
            baudval = 0x04;
            break;
        }
        case QSerialPort::BaudRate::Baud115200: {
            baudval = 0x05;
            break;
        }
        default: {
            /* 0x06 = 128000 and 0x07 = 256000 not supported by QT */
            qWarning() << id() << "Unsupported UART parameter. BaudRate:" << baud;
            return;
        }
    }

    setFunction(RtuWriteUartParams);
    m_modbus->send(
       deviceAddress(),
       QModbusRequest( //
          QModbusRequest::WriteSingleRegister,
          (quint16) 0x2000, // 16bit uart config register
          (quint8) parval,  // 16bit parity - byte HI
          (quint8) baudval) // 16bit baud rate - byte LO
    );

    m_modbus->setBaudRate(baud);
    m_modbus->setParity(parity);
}

/* -------------------------------------------------------
 * Event Methods
 * ------------------------------------------------------- */

void WSModbusRtu::onModbusOpened()
{
    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Modbus opened";
    }

    /* reset internals */
    setFunction(RtuUnspecified);
    resetFunctionQueue();

    /* schedule inital queries */
    scheduleFunction(RtuReadVersion);
    scheduleFunction(RtuReadDeviceAddr);

    /* processing by derived classes */
    doModbusOpened();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::destroyed, this, &WSModbusRtu::onTimerDestroyed);
    connect(m_timer, &QTimer::timeout, this, &WSModbusRtu::onWorkerQueue);
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setSingleShot(false);
    m_timer->setInterval(50);
    m_timer->start();

    /* notify consumer */
    emit opened();
}

void WSModbusRtu::onModbusClosed()
{
    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Modbus closed";
    }

    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        m_timer->deleteLater();
    }

    /* processing by derived classes */
    doModbusClosed();

    emit closed();
}

void WSModbusRtu::onModbusError(uint server, const int code, const QString& message)
{
    /* skip, if event context not mine */
    if (server != deviceAddress() || function() == RtuUnspecified) {
        return;
    }

    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qCritical() << id() << "Modbus error:" << code << message;
    }

    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        m_timer->deleteLater();
    }

    /* processing by derived classes */
    doModbusError(code, message);
}

void WSModbusRtu::onModbusReceived(uint server, const QModbusResponse& resp, const QModbusDataUnit& unit, bool isDataUnit)
{
    /* skip, if event context not mine */
    if (server != deviceAddress() || function() == RtuUnspecified) {
        return;
    }

    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "FUNC>" << function()           //
                 << "RSP>" << resp                          //
                 << "UNIT>" << isDataUnit                   //
                 << "RT:" << Qt::dec << unit.registerType() //
                 << "VC:" << Qt::dec << unit.valueCount()   //
                 << "VD:" << Qt::hex << unit.values();
    }

    if (isDataUnit) {
        switch (unit.registerType()) {
            case QModbusDataUnit::Coils: {
                doMduCoils(unit);
                break;
            }
            case QModbusDataUnit::DiscreteInputs: {
                doMduDiscreteInputs(unit);
                break;
            }
            case QModbusDataUnit::InputRegisters: {
                doMduInputRegisters(unit);
                break;
            }
            case QModbusDataUnit::HoldingRegisters: {
                doMduHoldingRegisters(unit);
                break;
            }
            default: {
                qWarning() << id() << "Unhandled modbus data unit:" //
                           << unit.registerType();
                break;
            }
        }
    }
}

void WSModbusRtu::onModbusComplete(uint server)
{
    /* skip, if event context not mine */
    if (server != deviceAddress() || function() == RtuUnspecified) {
        return;
    }

    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Modbus complete";
    }

    emit complete(function());

    doComplete(function());

    /* reset current progress */
    setFunction(RtuUnspecified);
}

void WSModbusRtu::onWorkerQueue()
{
    /* nothing to do or just in work */
    if (isFunctionQueueEmpty() || function() != RtuUnspecified) {
        return;
    }

    QTimer* timer;
    if (!(timer = dynamic_cast<QTimer*>(sender()))) {
        qDebug() << id() << "OOPS! Sender object not timer!";
        resetFunctionQueue();
        return;
    }

    /* kill timer if modbus closed */
    if (!isValidModbus()) {
        timer->stop();
        timer->deleteLater();
        resetFunctionQueue();
        return;
    }

    uint function;
    switch ((function = takeFirstFunction())) {
        case RtuReadVersion: {
            readVersion();
            break;
        }
        case RtuReadDeviceAddr: {
            readDeviceAddress();
            break;
        }
        default: {
            doFunction(function);
            break;
        }
    }

#if defined(QUERY_STATUS_WITH_WORKER)
    /* restart query cycle */
    if (isFunctionQueueEmpty()) {
        /* derived class schedule queries */
        startStatusWorker();
        /* remove timer if nothing to do */
        if (isFunctionQueueEmpty()) {
            timer->stop();
            timer->deleteLater();
            return;
        }
        /* update timer to query interval */
        if (timer->interval() != ((int) queryInterval())) {
            timer->setInterval(queryInterval());
        }
    }
#endif
}

void WSModbusRtu::onTimerDestroyed()
{
    if (m_modbus->isTrace(MBRtuClient::TRACE_CONTROL)) {
        qDebug() << id() << "Timer destroyed";
    }
    m_timer = nullptr;
}
