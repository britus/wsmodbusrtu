/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.escrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#pragma once
#include <QMap>
#include <QObject>
#include <QWidget>
#include <mbrtuclient.h>
#include <wsmodbusrtu.h>

/**
 * @brief The Modbus RTU Analog Input 8CH driver class
 */
class WSAnalogInMbRtu: public WSModbusRtu
{
    Q_OBJECT

public:
    enum TAdcFunction {
        ReadDataValues = RtuCustomStart + 0x0201,
        ReadChannelTypes = RtuCustomStart + 0x0202,
        WriteChannelTypes = RtuCustomStart + 0x0203,
        WriteChannelType = RtuCustomStart + 0x0204,
    };
    Q_ENUM(TAdcFunction)

    enum TChannelType {
        Range0_5000mV = 0x00,
        Range1000_5000mV = 0x01,
        Range0_20000uA = 0x02,
        Range4000_20000uA = 0x03,
        RangeDirect4096 = 0x04,
    };
    Q_ENUM(TChannelType)

    explicit WSAnalogInMbRtu(MBRtuClient* modbus, QObject* parent = nullptr);

    ~WSAnalogInMbRtu();

    const char* id() const override;
    quint8 maxInputs() const override;
    quint8 maxOutputs() const override;
    QWidget* settingsWidget(QWidget* parent) override;

    TChannelType channelType(quint8 channel) const;
    void setChannelTypes(const QMap<quint8, TChannelType>& types, bool updateDevice = false);
    void setChannelType(quint8 channel, const TChannelType type, bool updateDevice = false);

    float channelValue(quint8 channel) const;

signals:
    void channelChanged(quint8 channel, WSAnalogInMbRtu::TChannelType type);
    void valueChanged(quint8 channel, float value);

protected:
    void doModbusOpened() override;
    void startStatusWorker() override;
    void doFunction(uint function) override;
    bool doMduInputRegisters(const QModbusDataUnit& unit) override;
    bool doMduHoldingRegisters(const QModbusDataUnit& unit) override;

private:
    QMap<quint8, float> m_values;
    QMap<quint8, TChannelType> m_types;

private:
    inline void readDataValues();
    inline void readChannelTypes();
};

Q_DECLARE_METATYPE(WSAnalogInMbRtu::TAdcFunction);
Q_DECLARE_METATYPE(WSAnalogInMbRtu::TChannelType);
