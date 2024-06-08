/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.escrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#pragma once
#include <QEvent>
#include <QModbusDataUnit>
#include <QModbusDataUnitMap>
#include <QModbusDevice>
#include <QModbusExceptionResponse>
#include <QModbusRtuSerialMaster>
#include <QMutex>
#include <QObject>
#include <QSerialPort>
#include <QSettings>
#include <QThread>
#include <QWaitCondition>

#define CS_EVENT(id)   ((QEvent::Type)(QEvent::User + id))
#define CS_EVENT_ID(t) ((int) t)

class ModbusQueueWorker;

/**
 * @brief The Modbus Serial RS232/RS485 RTU client class
 * This class can be used in a multi threaded app. All
 * commands scheduled in a background queue and is fully
 * event driven.
 */
class MBRtuClient: public QObject
{
    Q_OBJECT

public:
    static const uint TRACE_CONTROL = 0x01;
    static const uint TRACE_REQUEST = 0x02;
    static const uint TRACE_RESPONSE = 0x04;
    static const uint TRACE_DATAUNIT = 0x08;
    static const uint TRACE_INTERNAL = 0x1000;

    typedef struct Config {
        QString m_portName;
        QSerialPort::BaudRate m_baudRate;
        QSerialPort::DataBits m_dataBits;
        QSerialPort::StopBits m_stopBits;
        QSerialPort::Parity m_parity;
        // QSerialPort::FlowControl m_flow;
        uint m_traceFlags;
    } TConfig;

    /**
     * @brief Default constructor
     * @param parent
     */
    explicit MBRtuClient(QObject* parent = nullptr);
    /**
     * Destructor kills worker thread
     */
    ~MBRtuClient();
    /**
     * @brief customEvent
     * @param event
     */
    void customEvent(QEvent* event) override;
    /**
     * @brief isOpen
     * @return
     */
    bool isOpen() const;
    /**
     * @brief open
     * @return
     */
    void open();
    /**
     * @brief close
     */
    void close();
    /**
     * @brief read
     * @param server
     * @param unit
     */
    void read(const uint server, const QModbusDataUnit& unit);
    /**
     * @brief write
     * @param server
     * @param unit
     */
    void write(const uint server, const QModbusDataUnit& unit);
    /**
     * @brief send
     * @param server
     * @param mr
     */
    void send(const uint server, const QModbusRequest& mr);
    /**
     * @brief portName
     * @return
     */
    const QString& portName() const;
    /**
     * @brief setPortName
     * @param name
     */
    void setPortName(const QString& name);
    /**
     * @brief baudRate
     * @return
     */
    const QSerialPort::BaudRate& baudRate() const;
    /**
     * @brief setBaudRate
     * @param rate
     */
    void setBaudRate(const QSerialPort::BaudRate rate);
    /**
     * @brief parity
     * @return
     */
    const QSerialPort::Parity& parity() const;
    /**
     * @brief setParity
     * @param parity
     */
    void setParity(const QSerialPort::Parity parity);
    /**
     * @brief dataBits
     * @return
     */
    const QSerialPort::DataBits& dataBits() const;
    /**
     * @brief setDataBits
     * @param bits
     */
    void setDataBits(const QSerialPort::DataBits bits);
    /**
     * @brief stopBits
     * @return
     */
    const QSerialPort::StopBits& stopBits() const;
    /**
     * @brief setStopBits
     * @param bits
     */
    void setStopBits(const QSerialPort::StopBits bits);
    /**
     * @brief setTraceFlags
     * @param flags
     */
    void setTraceFlags(const uint flags);
    /**
     * @brief traceFlags
     * @return
     */
    uint traceFlags() const;
    /**
     * @brief isTrace
     * @param mask
     * @return true or false
     */
    bool isTrace(uint mask) const;

signals:
    /**
     * @brief opened
     */
    void opened();
    /**
     * @brief closed
     */
    void closed();
    /**
     * @brief error
     * @param code
     * @param message
     */
    void error(uint server, const int code, const QString& message);
    /**
     * @brief received
     */
    void received(uint server, const QModbusResponse&, const QModbusDataUnit&, bool);
    /**
     * @brief complete
     */
    void complete(uint server);

private slots:
    void onModbusError(QModbusDevice::Error);
    void onModbusState(QModbusDevice::State);
    void onModbusReply();
    void onReplyDestroyed();
    /* -- */
    void onWorkerStarted();
    void onWorkerFinished();
    void onWorkerDestroyed();

private:
    friend class ModbusQueueWorker;
    static const uint ID_EVENT_OPEN = 601;
    static const uint ID_EVENT_CLOSE = 602;
    static const uint ID_EVENT_READ = 603;
    static const uint ID_EVENT_WRITE = 604;
    static const uint ID_EVENT_REQUEST = 605;

    class IOEvent: public QEvent
    {
    public:
        explicit IOEvent(QEvent::Type type, const int server, const QModbusDataUnit& unit)
            : QEvent(type)
            , m_server(server)
            , m_unit(unit) {};
        explicit IOEvent(QEvent::Type type, const int server, const QModbusRequest& mr)
            : QEvent(type)
            , m_server(server)
            , m_request(mr) {};

        inline const QModbusDataUnit& unit() const
        {
            return m_unit;
        }

        inline const QModbusRequest& request() const
        {
            return m_request;
        }

        inline const int& server() const
        {
            return m_server;
        }

    private:
        int m_server;
        QModbusDataUnit m_unit;
        QModbusRequest m_request;
    };

    TConfig m_config;
    QModbusRtuSerialMaster m_modbus;
    bool m_isOpen;

private:
    ModbusQueueWorker* m_worker;
    inline void createWorker();
    inline void removeWorker();
    inline bool connectDevice();
    inline void disconnectDevice();
    inline void request(IOEvent* event);
    inline void request(uint server, const QModbusRequest& mr);
    inline void request(uint action, uint server, const QModbusDataUnit& unit);
};

class ModbusQueueWorker: public QThread
{
    Q_OBJECT

public:
    enum TRequestType {
        RequestSend,
        DataUnitRead,
        DataUnitWrite,
    };

    typedef struct {
        TRequestType type;
        uint server;
        QModbusRequest request;
        QModbusDataUnit unit;
    } TRequest;

    ModbusQueueWorker(MBRtuClient* client, QObject* parent = nullptr);
    ~ModbusQueueWorker();

    void run() override;

    void clearQueue();
    void scheduleRequest(const TRequest& request);
    void notifyRequest();
    void notifyResponse();
    uint activeServer();

private:
    MBRtuClient* m_client;
    QList<TRequest> m_queue;
    QMutex m_queueLock;
    QMutex m_queueWaitLock;
    QMutex m_responseWaitLock;
    QWaitCondition m_queueWait;
    QWaitCondition m_responseWait;
    TRequest m_pendingRequest;

private:
    inline bool isQueueEmpty();
    inline void sendRequest();
    inline bool waitForRequests();
    inline bool waitForResponse();
};
