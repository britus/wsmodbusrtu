/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#pragma once
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QWidget>
#include <mbrtuclient.h>

/**
 * @brief The base class for Waveshare Modbus RTU devices
 * This class can be used in a multi threaded app. All
 * commands scheduled in a background queue and is fully
 * event driven.
 */
class WSModbusRtu: public QObject
{
    Q_OBJECT

public:
    enum TRtuFunction {
        RtuUnspecified = 0x0000,
        RtuReadVersion = 0x0001,
        RtuReadDeviceAddr = 0x0002,
        RtuWriteDeviceAddr = 0x0003,
        RtuWriteUartParams = 0x0004,
        RtuCustomStart = 0x1000,
    };
    Q_ENUM(TRtuFunction)

    explicit WSModbusRtu(MBRtuClient* modbus, QObject* parent = nullptr);

    ~WSModbusRtu();

    virtual const char* id() const = 0;
    virtual quint8 maxInputs() const = 0;
    virtual quint8 maxOutputs() const = 0;
    virtual QWidget* settingsWidget(QWidget* parent) = 0;

    void open();
    void close();

    const quint16& firmwareVersion() const;

    const quint8& deviceAddress() const;
    void setDeviceAddress(const quint8 address, const bool updateDevice = false);

    const uint& queryInterval() const;
    void setQueryInterval(uint interval);

    const uint& function() const;

    void scheduleFunction(const uint function);

    const QString& portName() const;
    void setPortName(const QString& name);

    const QSerialPort::BaudRate& baudRate() const;
    void setBaudRate(const QSerialPort::BaudRate rate, const bool updateDevice = false);

    const QSerialPort::Parity& parity() const;
    void setParity(const QSerialPort::Parity parity, const bool updateDevice = false);

    const QSerialPort::DataBits& dataBits() const;
    void setDataBits(const QSerialPort::DataBits bits);

    const QSerialPort::StopBits& stopBits() const;
    void setStopBits(const QSerialPort::StopBits bits);

    bool isValidModbus() const;

signals:
    void opened();
    void closed();
    void addressChanged(quint8 address);
    void intervalChanged(uint interval);
    void complete(uint function);

protected:
    bool isTrace(uint mask) const;
    void send(uint function, quint8 device, const QModbusRequest& mr);
    void read(uint function, quint8 device, const QModbusDataUnit& du);
    void write(uint function, quint8 device, const QModbusDataUnit& du);
    bool checkValueCount(const uint count, const QModbusDataUnit& unit);
    bool isFunctionQueueEmpty() const;
    void resetFunctionQueue();
    uint takeFirstFunction();
    virtual void readVersion();
    virtual void readDeviceAddress();
    virtual void startStatusWorker();
    virtual bool doMduCoils(const QModbusDataUnit& unit);
    virtual bool doMduDiscreteInputs(const QModbusDataUnit& unit);
    virtual bool doMduInputRegisters(const QModbusDataUnit& unit);
    virtual bool doMduHoldingRegisters(const QModbusDataUnit& unit);
    virtual void doModbusOpened();
    virtual void doModbusClosed();
    virtual void doModbusError(const int, const QString&);
    virtual void doComplete(uint function = RtuUnspecified);
    virtual void doFunction(uint function = RtuUnspecified);

private slots:
    void onModbusOpened();
    void onModbusClosed();
    void onModbusError(uint server, const int, const QString&);
    void onModbusReceived(uint server, const QModbusResponse&, const QModbusDataUnit&, bool);
    void onModbusComplete(uint server);
    void onWorkerQueue();
    void onTimerDestroyed();

private:
    /* modbus RTU client */
    MBRtuClient* m_modbus;
    /* RTU firmware version */
    quint16 m_fwVersion;
    /* RTU device address */
    quint8 m_address;
    /* status query interval */
    uint m_interval;
    /* holds current command function */
    uint m_function;
    /* function queue */
    QList<uint> m_funcQueue;
    /* Query timer */
    QTimer* m_timer;

private:
    inline void setDeviceUartParams(const QSerialPort::BaudRate baud, const QSerialPort::Parity parity);
    inline void setFunction(const uint function);
};

Q_DECLARE_METATYPE(WSModbusRtu::TRtuFunction)
