/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.escrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QSerialPortInfo>
#include <QTimer>
#include <mbrtuclient.h>

MBRtuClient::MBRtuClient(QObject* parent)
    : QObject {parent}
    , m_config()
    , m_modbus(this)
    , m_isOpen(false)
    , m_worker(nullptr)
{
    qRegisterMetaType<QSerialPort::SerialPortError>();
    qRegisterMetaType<QModbusDevice::Error>();
    qRegisterMetaType<QModbusDevice::State>();

    /* default configuration (Firefly AIO-RK3568J IPC board) */
    m_config.m_portName = "ttysWK0"; // RS485_1 / ttysWK1 = RS485_2
    m_config.m_baudRate = QSerialPort::Baud9600;
    m_config.m_dataBits = QSerialPort::Data8;
    m_config.m_stopBits = QSerialPort::OneStop;
    m_config.m_parity = QSerialPort::NoParity;

    m_config.m_traceFlags =
       (TRACE_CONTROL |  //
        TRACE_REQUEST |  //
        TRACE_RESPONSE | //
        TRACE_DATAUNIT);

    connect(&m_modbus, &QModbusRtuSerialMaster::errorOccurred, this, &MBRtuClient::onModbusError);
    connect(&m_modbus, &QModbusRtuSerialMaster::stateChanged, this, &MBRtuClient::onModbusState);
}

MBRtuClient::~MBRtuClient()
{
    qDebug() << Q_FUNC_INFO;
    removeWorker();
}

void MBRtuClient::customEvent(QEvent* event)
{
    if (isTrace(TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Custom event:" << event->type();
    }

    switch (CS_EVENT_ID(event->type())) {
        case CS_EVENT(ID_EVENT_OPEN): {
            connectDevice();
            return;
        }
        case CS_EVENT(ID_EVENT_CLOSE): {
            disconnectDevice();
            return;
        }
        case CS_EVENT(ID_EVENT_READ):
        case CS_EVENT(ID_EVENT_WRITE):
        case CS_EVENT(ID_EVENT_REQUEST): {
            IOEvent* ev;
            if ((ev = dynamic_cast<IOEvent*>(event))) {
                request(ev);
            }
            return;
        }
    }

    QObject::customEvent(event);
}

/* --------------------------------------------------------------------
 * API Methods
 * -------------------------------------------------------------------- */

bool MBRtuClient::isOpen() const
{
    return m_isOpen;
}

void MBRtuClient::open()
{
    qApp->postEvent(this, new QEvent(CS_EVENT(ID_EVENT_OPEN)));
}

void MBRtuClient::close()
{
    qApp->postEvent(this, new QEvent(CS_EVENT(ID_EVENT_CLOSE)));
}

void MBRtuClient::read(const uint server, const QModbusDataUnit& unit)
{
    if (!m_worker) {
        qApp->postEvent(this, new IOEvent(CS_EVENT(ID_EVENT_READ), server, unit));
    }
    else {
        m_worker->scheduleRequest({
           .type = ModbusQueueWorker::DataUnitRead,
           .server = server,
           .request = {},
           // .unit = QModbusDataUnit(unit.registerType(), unit.startAddress(), unit.values()),
           .unit = unit,
        });
        m_worker->notifyRequest();
    }
}

void MBRtuClient::write(const uint server, const QModbusDataUnit& unit)
{
    if (!m_worker) {
        qApp->postEvent(this, new IOEvent(CS_EVENT(ID_EVENT_WRITE), server, unit));
    }
    else {
        m_worker->scheduleRequest({
           .type = ModbusQueueWorker::DataUnitWrite,
           .server = server,
           .request = {},
           //.unit = QModbusDataUnit(unit.registerType(), unit.startAddress(), unit.values()),
           .unit = unit,
        });
        m_worker->notifyRequest();
    }
}

void MBRtuClient::send(const uint server, const QModbusRequest& mr)
{
    if (!m_worker) {
        qApp->postEvent(this, new IOEvent(CS_EVENT(ID_EVENT_REQUEST), server, mr));
    }
    else {
        m_worker->scheduleRequest({
           .type = ModbusQueueWorker::RequestSend,
           .server = server,
           //.request = QModbusRequest(mr),
           .request = mr,
           .unit = {},
        });
        m_worker->notifyRequest();
    }
}

const QString& MBRtuClient::portName() const
{
    return m_config.m_portName;
}

void MBRtuClient::setPortName(const QString& name)
{
    if (m_config.m_portName != name) {
        m_config.m_portName = name;
        m_modbus.setConnectionParameter( //
           QModbusRtuSerialMaster::SerialPortNameParameter,
           QVariant::fromValue(name));
    }
}

const QSerialPort::BaudRate& MBRtuClient::baudRate() const
{
    return m_config.m_baudRate;
}

void MBRtuClient::setBaudRate(const QSerialPort::BaudRate rate)
{
    if (m_config.m_baudRate != rate) {
        m_config.m_baudRate = rate;
        m_modbus.setConnectionParameter( //
           QModbusRtuSerialMaster::SerialBaudRateParameter,
           QVariant::fromValue(rate));
    }
}

const QSerialPort::Parity& MBRtuClient::parity() const
{
    return m_config.m_parity;
}

void MBRtuClient::setParity(const QSerialPort::Parity parity)
{
    if (m_config.m_parity != parity) {
        m_config.m_parity = parity;
        m_modbus.setConnectionParameter( //
           QModbusRtuSerialMaster::SerialParityParameter,
           QVariant::fromValue(parity));
    }
}

const QSerialPort::DataBits& MBRtuClient::dataBits() const
{
    return m_config.m_dataBits;
}

void MBRtuClient::setDataBits(const QSerialPort::DataBits bits)
{
    if (m_config.m_dataBits != bits) {
        m_config.m_dataBits = bits;
        m_modbus.setConnectionParameter( //
           QModbusRtuSerialMaster::SerialDataBitsParameter,
           QVariant::fromValue(bits));
    }
}

const QSerialPort::StopBits& MBRtuClient::stopBits() const
{
    return m_config.m_stopBits;
}

void MBRtuClient::setStopBits(const QSerialPort::StopBits bits)
{
    if (m_config.m_stopBits != bits) {
        m_config.m_stopBits = bits;
        m_modbus.setConnectionParameter( //
           QModbusRtuSerialMaster::SerialStopBitsParameter,
           QVariant::fromValue(bits));
    }
}

void MBRtuClient::setTraceFlags(const uint flags)
{
    m_config.m_traceFlags = flags;
}

uint MBRtuClient::traceFlags() const
{
    return m_config.m_traceFlags;
}

/* --------------------------------------------------------------------
 * Private Methods
 * -------------------------------------------------------------------- */

bool MBRtuClient::isTrace(uint mask) const
{
    return ((traceFlags() & mask) == mask);
}

inline void MBRtuClient::createWorker()
{
    if (!m_worker) {
        m_worker = new ModbusQueueWorker(this, this);
        connect(m_worker, &ModbusQueueWorker::started, this, &MBRtuClient::onWorkerStarted);
        connect(m_worker, &ModbusQueueWorker::finished, this, &MBRtuClient::onWorkerFinished);
        connect(m_worker, &ModbusQueueWorker::destroyed, this, &MBRtuClient::onWorkerDestroyed);
        m_worker->start(QThread::HighPriority);
    }
}

inline void MBRtuClient::removeWorker()
{
    if (m_worker) {
        m_worker->requestInterruption();
        m_worker->notifyRequest();
        m_worker->notifyResponse();
        m_worker->wait(5000);
        m_worker->deleteLater();
        m_worker = nullptr;
    }
}

inline bool MBRtuClient::connectDevice()
{
    if (m_isOpen) {
        return true;
    }

    /* find symbolic link to real port name */
    QSerialPortInfo spi;
    QDir devPath("/dev");
    QString portName = m_config.m_portName;
    QFileInfoList fil = devPath.entryInfoList(QStringList() << "ttyMB*");
    if (!fil.isEmpty()) {
        qInfo() << "MODBUS: Found ttyMBUS symlink to serial device.";
        foreach (auto fi, fil) {
            if (fi.isSymLink()) {
                QString sym = fi.absoluteFilePath();
                QString tgt = fi.symLinkTarget();
                qInfo() << "MODBUS: " << sym << " -> " << tgt;
                if (sym.contains(portName)) {
                    QString dev;
                    dev = tgt.replace(devPath.absolutePath(), "");
                    dev = dev.replace("/", "");
                    qInfo() << "MODBUS: Using device:" << dev << "for" << portName;
                    portName = dev;
                    break;
                }
            }
        }
    }

    /* lookup known serial ports */
    foreach (auto pi, spi.availablePorts()) {
        if (pi.portName().contains(portName)) {
            qInfo() << "MODBUS: Using device:" << pi.systemLocation();
            spi = pi;
            break;
        }
    }

    if (spi.isNull()) {
        qCritical() << "MODBUS: Can't find serial port:" << portName.toUtf8().constData();
        return false;
    }

    /* create worker thread */
    createWorker();

    m_modbus.setConnectionParameter( //
       QModbusDevice::SerialPortNameParameter,
       QVariant::fromValue(m_config.m_portName));
    m_modbus.setConnectionParameter( //
       QModbusDevice::SerialBaudRateParameter,
       QVariant::fromValue(m_config.m_baudRate));
    m_modbus.setConnectionParameter( //
       QModbusDevice::SerialDataBitsParameter,
       QVariant::fromValue(m_config.m_dataBits));
    m_modbus.setConnectionParameter( //
       QModbusDevice::SerialStopBitsParameter,
       QVariant::fromValue(m_config.m_stopBits));
    m_modbus.setConnectionParameter( //
       QModbusDevice::SerialParityParameter,
       QVariant::fromValue(m_config.m_parity));

    return m_modbus.connectDevice();
}

inline void MBRtuClient::disconnectDevice()
{
    removeWorker();

    if (m_isOpen) {
        m_isOpen = false;
        m_modbus.disconnectDevice();
    }
}

inline void MBRtuClient::request(IOEvent* event)
{
    switch (CS_EVENT_ID(event->type())) {
        case CS_EVENT(ID_EVENT_READ):
        case CS_EVENT(ID_EVENT_WRITE): {
            request(event->type(), event->server(), event->unit());
            break;
        }
        case CS_EVENT(ID_EVENT_REQUEST): {
            request(event->server(), event->request());
            break;
        }
        default: {
            qCritical() << "MODBUS: Invalid event type:" << event->type();
            if (m_worker) {
                m_worker->notifyResponse(); // unlock waiting worker
            }
            break;
        }
    }
}

inline void MBRtuClient::request(uint server, const QModbusRequest& mr)
{
    if (server > 248) {
        qCritical() << "MODBUS: Invalid server address:" << server;
        if (m_worker) {
            m_worker->notifyResponse(); // unlock waiting worker
        }
        return;
    }

    if (!mr.isValid()) {
        qCritical() << "MODBUS: Invalid request object.";
        if (m_worker) {
            m_worker->notifyResponse(); // unlock waiting worker
        }
        return;
    }

    if (isTrace(TRACE_REQUEST | TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Request"              //
                 << "Device:" << Qt::dec << server //
                 << "Data:" << Qt::hex << mr;
    }

    QModbusReply* reply;
    if ((reply = m_modbus.sendRawRequest(mr, server))) {
        connect(reply, &QModbusReply::finished, this, &MBRtuClient::onModbusReply);
        connect(reply, &QModbusReply::destroyed, this, &MBRtuClient::onReplyDestroyed);
        connect(reply, &QModbusReply::errorOccurred, this, &MBRtuClient::onModbusError);
    }
    else if (m_worker) {
        m_worker->notifyResponse(); // unlock waiting worker
    }
}

inline void MBRtuClient::request(uint action, uint server, const QModbusDataUnit& unit)
{
    if (server > 248) {
        qCritical() << "MODBUS: Invalid server address:" << server;
        if (m_worker) {
            m_worker->notifyResponse(); // unlock waiting worker
        }
        return;
    }

    if (!unit.isValid()) {
        qCritical() << "MODBUS: Invalid data unit.";
        if (m_worker) {
            m_worker->notifyResponse(); // unlock waiting worker
        }
        return;
    }

    if (isTrace(TRACE_REQUEST | TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Request"                          //
                 << "Event:" << Qt::dec << action              //
                 << "DAddr:" << Qt::dec << server              //
                 << "RType:" << Qt::hex << unit.registerType() //
                 << "RAddr:" << Qt::hex << unit.startAddress() //
                 << "Count:" << Qt::dec << unit.valueCount()   //
                 << "Value:" << Qt::hex << unit.values();
    }

    QModbusReply* reply = 0L;
    switch (CS_EVENT_ID(action)) {
        case CS_EVENT(ID_EVENT_READ): {
            reply = m_modbus.sendReadRequest(unit, server);
            break;
        }
        case CS_EVENT(ID_EVENT_WRITE): {
            reply = m_modbus.sendWriteRequest(unit, server);
            break;
        }
        default: {
            qCritical() << "MODBUS: Invalid request type:" << action;
            if (m_worker) {
                m_worker->notifyResponse(); // unlock waiting worker
            }
            return;
        }
    }
    if (reply) {
        connect(reply, &QModbusReply::finished, this, &MBRtuClient::onModbusReply);
        connect(reply, &QModbusReply::destroyed, this, &MBRtuClient::onReplyDestroyed);
        connect(reply, &QModbusReply::errorOccurred, this, &MBRtuClient::onModbusError);
    }
    else if (m_worker) {
        m_worker->notifyResponse(); // unlock waiting worker
    }
}

/* --------------------------------------------------------------------
 * Event Methods
 * -------------------------------------------------------------------- */

void MBRtuClient::onModbusError(QModbusDevice::Error code)
{
    if (code == QModbusDevice::NoError) {
        return;
    }

    QString msg;
    switch (code) {
        case QModbusDevice::ReadError: {
            msg = tr("Read error");
            break;
        }
        case QModbusDevice::WriteError: {
            msg = tr("Write error");
            break;
        }
        case QModbusDevice::ConnectionError: {
            msg = tr("Connection error");
            break;
        }
        case QModbusDevice::ConfigurationError: {
            msg = tr("Configuration error");
            break;
        }
        case QModbusDevice::TimeoutError: {
            msg = tr("Timeout error");
            break;
        }
        case QModbusDevice::ProtocolError: {
            msg = tr("Protocol error");
            break;
        }
        case QModbusDevice::ReplyAbortedError: {
            msg = tr("Reply aborted error");
            break;
        }
        default: {
            msg = tr("Unknown error %1").arg(code);
            break;
        }
    }

    qCritical() << "MODBUS:" << msg.toUtf8().constData();

    if (m_worker) {
        emit error(m_worker->activeServer(), code, msg);
    }
    else {
        emit error(0, code, msg);
    }

    /* force port close if open, but not on timeout. May
     * be one of the devices in chain is offline. */
    if (m_isOpen && code != QModbusDevice::TimeoutError) {
        disconnectDevice();
    }
}

void MBRtuClient::onModbusState(QModbusDevice::State state)
{
    if (isTrace(TRACE_INTERNAL)) {
        qDebug() << "MODBUS: State changed:" << state;
    }

    switch (state) {
        case QModbusDevice::UnconnectedState: {
            m_isOpen = false;
            emit closed();
            break;
        }
        case QModbusDevice::ConnectingState: {
            break;
        }
        case QModbusDevice::ConnectedState: {
            m_isOpen = true;
            emit opened();
            break;
        }
        case QModbusDevice::ClosingState: {
            break;
        }
    }
}

void MBRtuClient::onModbusReply()
{
    QModbusReply* reply;
    if (!(reply = dynamic_cast<QModbusReply*>(sender()))) {
        qCritical() << "MODBUS: Invalid reply object";
        return;
    }

    /* error event already emitted by modbus */
    if (reply->error() != QModbusDevice::NoError) {
        reply->deleteLater();
        return;
    }

    /* get raw result */
    QModbusResponse resp = reply->rawResult();
    if (!resp.isValid() || resp.isException()) {
        qCritical() << "MODBUS: Got invalid response. Exception:" //
                    << resp.exceptionCode();
        reply->deleteLater();
        return;
    }

    QStringList dump;
    for (qint16 i = 0; i < resp.dataSize(); i++) {
        dump << tr("0x%1").arg(resp.data().at(i), 2, 16, QChar('0'));
    }

    if (isTrace(TRACE_RESPONSE | TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Response"                        //
                 << "Func:" << Qt::hex << resp.functionCode() //
                 << "Size:" << Qt::dec << resp.dataSize()     //
                 << "Dump:" << dump;
    }

    /* raw result translator */
    bool isUnit;
    QModbusDataUnit unit = reply->result();
    auto translate = [resp](QModbusDataUnit& unit) {
        /* nothing to do if empty */
        if (resp.dataSize() <= 0) {
            return false;
        }

        QVector<quint16> values;
        QModbusDataUnit::RegisterType type;
        QByteArray data = resp.data();
        int count = data.count();

        /* setup data unit type */
        switch (resp.functionCode()) {
            case QModbusResponse::ReadDiscreteInputs: {
                type = QModbusDataUnit::DiscreteInputs;
                break;
            }
            case QModbusResponse::ReadCoils: {
                type = QModbusDataUnit::Coils;
                break;
            }
            case QModbusResponse::ReadHoldingRegisters: {
                type = QModbusDataUnit::HoldingRegisters;
                // first byte is number of bytes in data
                count = data.at(0);
                data.remove(0, 1);
                break;
            }
            case QModbusResponse::ReadInputRegisters: {
                type = QModbusDataUnit::InputRegisters;
                // first byte is number of bytes in data
                count = data.at(0);
                data.remove(0, 1);
                break;
            }
            default: {
                type = QModbusDataUnit::InputRegisters;
                break;
            }
        }

        /* digital input read */
        if (type == QModbusDataUnit::Coils || type == QModbusDataUnit::DiscreteInputs) {
            /* first byte 0 should be number of bit mask bytes */
            for (int i = 1; i < count; i++) {
                char mask = data.at(i);
                for (quint8 b = 0; b < 8; b++) {
                    if ((mask & (1 << b)) != 0) {
                        values.append(1);
                    }
                    else {
                        values.append(0);
                    }
                }
            }
        }
        /* others as analog input */
        else {
            do {
                quint16 value = 0;

                /* hi byte */
                if (!data.isEmpty()) {
                    value = static_cast<quint16>(data.at(0));
                    data.remove(0, 1);
                }

                /* add lo byte */
                if (!data.isEmpty()) {
                    value <<= 8; // shift to hi byte
                    value |= (data.at(0) & 0x00ff);
                    data.remove(0, 1);
                }

                /* save result */
                values.append(value);
            } while (!data.isEmpty());
        }

        /* return translated */
        unit = QModbusDataUnit(type);
        unit.setStartAddress(count);
        unit.setValueCount(values.count());
        unit.setValues(values);
        return unit.isValid();
    };

    /* translate to data unit manually */
    if (!(isUnit = unit.isValid())) {
        isUnit = translate(unit);
    }

    if (isTrace(TRACE_DATAUNIT | TRACE_INTERNAL)) {
        qDebug() << "MODBUS: DataUnit"                      //
                 << "RT:" << Qt::hex << unit.registerType() //
                 << "SA:" << Qt::hex << unit.startAddress() //
                 << "VC:" << Qt::dec << unit.valueCount()   //
                 << "VD:" << Qt::hex << unit.values();
    }

    /* notfiy consumer */
    if (m_worker) {
        emit received(m_worker->activeServer(), resp, unit, isUnit);
    }
    else {
        emit received(0, resp, unit, isUnit);
    }

    /* remove reply object */
    reply->deleteLater();
    return;
}

void MBRtuClient::onReplyDestroyed()
{
    if (isTrace(TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Reply object destoyed.";
    }

    /* notfiy consumer */
    if (m_worker) {
        emit complete(m_worker->activeServer());
        /* run next request */
        m_worker->notifyResponse();
    }
    else {
        emit complete(0);
    }
}

void MBRtuClient::onWorkerStarted()
{
    if (isTrace(TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Queue worker started.";
    }
}

void MBRtuClient::onWorkerFinished()
{
    if (isTrace(TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Queue worker finished.";
    }
}

void MBRtuClient::onWorkerDestroyed()
{
    if (isTrace(TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Queue worker destoyed.";
    }
    m_worker = nullptr;
}

/* --------------------------------------------------------------------
 * WorkerThread
 * -------------------------------------------------------------------- */

ModbusQueueWorker::ModbusQueueWorker(MBRtuClient* client, QObject* parent)
    : QThread(parent)
    , m_client(client)
    , m_queue()
    , m_queueLock()
    , m_queueWaitLock()
    , m_responseWaitLock()
    , m_queueWait()
    , m_responseWait()
    , m_pendingRequest()
{
}

ModbusQueueWorker::~ModbusQueueWorker()
{
    qDebug() << Q_FUNC_INFO;

    clearQueue();

    if (isRunning()) {
        requestInterruption();
        notifyRequest();
        notifyResponse();
    }
}

void ModbusQueueWorker::clearQueue()
{
    QMutexLocker lock(&m_queueLock);
    m_queue.clear();
}

void ModbusQueueWorker::scheduleRequest(const TRequest& request)
{
    QMutexLocker lock(&m_queueLock);
    m_queue.append(request);
}

void ModbusQueueWorker::notifyRequest()
{
    m_queueWait.wakeOne();
}

void ModbusQueueWorker::notifyResponse()
{
    m_responseWait.wakeOne();
}

uint ModbusQueueWorker::activeServer()
{
    return m_pendingRequest.server;
}

inline bool ModbusQueueWorker::isQueueEmpty()
{
    QMutexLocker lock(&m_queueLock);
    return m_queue.isEmpty();
}

inline void ModbusQueueWorker::sendRequest()
{
    QMutexLocker lock(&m_queueLock);

    m_pendingRequest = m_queue.takeFirst();
    switch (m_pendingRequest.type) {
        case RequestSend: {
            qApp->postEvent(
               m_client,                 //
               new MBRtuClient::IOEvent( //
                  CS_EVENT(MBRtuClient::ID_EVENT_REQUEST),
                  m_pendingRequest.server,
                  m_pendingRequest.request));
            return;
        }
        case DataUnitRead: {
            qApp->postEvent(
               m_client,                 //
               new MBRtuClient::IOEvent( //
                  CS_EVENT(MBRtuClient::ID_EVENT_READ),
                  m_pendingRequest.server,
                  m_pendingRequest.unit));
            return;
        }
        case DataUnitWrite: {
            qApp->postEvent(
               m_client,                 //
               new MBRtuClient::IOEvent( //
                  CS_EVENT(MBRtuClient::ID_EVENT_WRITE),
                  m_pendingRequest.server,
                  m_pendingRequest.unit));
            return;
        }
    }
}

inline bool ModbusQueueWorker::waitForRequests()
{
    QMutexLocker lock(&m_queueWaitLock);
    m_queueWait.wait(&m_queueWaitLock);

    /* stop thread if interrupted */
    if (isInterruptionRequested()) {
        return false;
    }

    return true;
}

inline bool ModbusQueueWorker::waitForResponse()
{
    QMutexLocker lock(&m_responseWaitLock);

    if (!m_responseWait.wait(&m_responseWaitLock, 30000)) {
        qCritical() << "MODBUS: Request timed out!";
        return false;
    }

    /* stop thread if interrupted */
    if (isInterruptionRequested()) {
        return false;
    }

    return true;
}

void ModbusQueueWorker::run()
{
    if (m_client->isTrace(MBRtuClient::TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Worker run enter.";
    }

    while (!isInterruptionRequested()) {

        if (isQueueEmpty()) {
            /* wait for new requests */
            if (!waitForRequests()) {
                break;
            }
        }

        /* send pending request via UI thread. Events
         * handled by modbus client object instance */
        sendRequest();

        /* wait for response ack */
        if (!waitForResponse()) {
            break;
        }
    }

    if (isInterruptionRequested()) {
        if (m_client->isTrace(MBRtuClient::TRACE_INTERNAL)) {
            qDebug() << "MODBUS: Queue worker interrupted.";
        }
        exit(-1);
        return;
    }

    if (m_client->isTrace(MBRtuClient::TRACE_INTERNAL)) {
        qDebug() << "MODBUS: Worker run leave.";
    }
    exit(0);
}
