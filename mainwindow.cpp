/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#include "ui_mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QModbusDataUnit>
#include <QSerialPortInfo>
#include <QSettings>
#include <QStandardPaths>
#include <dlgadcindatatype.h>
#include <dlgrelaylinkcontrol.h>
#include <mainwindow.h>

Q_DECLARE_METATYPE(QSerialPortInfo)
Q_DECLARE_METATYPE(QSerialPort::BaudRate)
Q_DECLARE_METATYPE(QSerialPort::DataBits)
Q_DECLARE_METATYPE(QSerialPort::StopBits)
Q_DECLARE_METATYPE(QSerialPort::Parity)

static const QString configFile()
{
    return QStringLiteral("%1%2%3") //
       .arg(
          QStandardPaths::writableLocation( //
             QStandardPaths::AppConfigLocation),
          QDir::separator(),
          "wsmodbusrtu.conf");
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings(configFile(), QSettings::IniFormat, this)
    , m_config()
    , m_modbus(this)
    , m_rly(nullptr)
    , m_adc(nullptr)
    , m_chg(nullptr)
{
    qDebug() << "APPWND: Config file:" << m_settings.fileName();

    ui->setupUi(this);

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::onAppQuit);

    m_config.mbconf = m_modbus.config();
    m_config.rlyAddr = 1;
    m_config.adcAddr = 1;
    loadConfig();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    int selected = -1;

    /* Global MODBUS */
    ui->cbComPort->clear();
    for (int i = 0; i < ports.count(); i++) {
        ui->cbComPort->addItem(ports[i].portName(), QVariant::fromValue(ports[i]));
        if (ports[i].portName().contains(m_config.mbconf.m_portName)) {
            selected = i;
        }
    }
    if (ui->cbComPort->count() > 0) {
        ui->cbComPort->setCurrentIndex(selected);
    }

    typedef struct {
        QString name;
        QSerialPort::BaudRate baud;
    } TBaudRates;

    TBaudRates baudRates[8] = {
       //{tr("1200"), QSerialPort::Baud1200},
       //{tr("2400"), QSerialPort::Baud2400},
       {tr("4800"), QSerialPort::Baud4800},
       {tr("9600"), QSerialPort::Baud9600},
       {tr("19200"), QSerialPort::Baud19200},
       {tr("38400"), QSerialPort::Baud38400},
       {tr("57600"), QSerialPort::Baud57600},
       {tr("115200"), QSerialPort::Baud115200},
    };

    selected = -1;
    ui->cbBaudRate->clear();
    for (int i = 0; i < 8; i++) {
        ui->cbBaudRate->addItem(baudRates[i].name, QVariant::fromValue(baudRates[i].baud));
        if (m_config.mbconf.m_baudRate == baudRates[i].baud) {
            selected = i;
        }
    }
    if (selected > -1) {
        ui->cbBaudRate->setCurrentIndex(selected);
    }

    typedef struct {
        QString name;
        QSerialPort::DataBits bits;
    } TDataBits;

    TDataBits dataBits[4] = {
       {tr("5 Bits"), QSerialPort::Data5},
       {tr("6 Bits"), QSerialPort::Data6},
       {tr("7 Bits"), QSerialPort::Data7},
       {tr("8 Bits"), QSerialPort::Data8},
    };

    selected = -1;
    ui->cbDataBits->clear();
    for (int i = 0; i < 4; i++) {
        ui->cbDataBits->addItem(dataBits[i].name, QVariant::fromValue(dataBits[i].bits));
        if (m_config.mbconf.m_dataBits == dataBits[i].bits) {
            selected = i;
        }
    }
    if (selected > -1) {
        ui->cbDataBits->setCurrentIndex(selected);
    }

    typedef struct {
        QString name;
        QSerialPort::StopBits bits;
    } TStopBits;

    TStopBits stopBits[3] = {
       {tr("One Stop"), QSerialPort::OneStop},
       {tr("One and Half"), QSerialPort::OneAndHalfStop},
       {tr("Two Stop"), QSerialPort::TwoStop},
    };

    selected = -1;
    ui->cbStopBits->clear();
    for (int i = 0; i < 3; i++) {
        ui->cbStopBits->addItem(stopBits[i].name, QVariant::fromValue(stopBits[i].bits));
        if (m_config.mbconf.m_stopBits == stopBits[i].bits) {
            selected = i;
        }
    }
    if (selected > -1) {
        ui->cbStopBits->setCurrentIndex(selected);
    }

    typedef struct {
        QString name;
        QSerialPort::Parity parity;
    } TParity;

    TParity parity[5] = {
       {tr("No Parity"), QSerialPort::NoParity},
       {tr("Event Parity"), QSerialPort::EvenParity},
       {tr("Odd Parity"), QSerialPort::OddParity},
       {tr("Space Parity"), QSerialPort::SpaceParity},
       {tr("Mark Parity"), QSerialPort::MarkParity},
    };

    selected = -1;
    ui->cbParity->clear();
    for (int i = 0; i < 5; i++) {
        ui->cbParity->addItem(parity[i].name, QVariant::fromValue(parity[i].parity));
        if (m_config.mbconf.m_parity == parity[i].parity) {
            selected = i;
        }
    }
    if (selected > -1) {
        ui->cbParity->setCurrentIndex(selected);
    }

    typedef struct {
        QString name;
        uint index;
    } TDeviceSelect;

    TDeviceSelect devices[3] = {
       {tr("Waveshare Modbus RTU Relay (D)"), 1},
       {tr("Waveshare Modbus RTU AnalogIn (8CH)"), 2},
       {tr("Renogy MPPT Solar Controller"), 3},
    };

    selected = -1;
    ui->cbDeviceList->clear();
    for (int i = 0; i < 3; i++) {
        ui->cbDeviceList->addItem(devices[i].name, QVariant::fromValue(devices[i].index));
        if (m_config.selDev == i) {
            selected = i;
        }
    }
    if (selected > -1) {
        ui->cbDeviceList->setCurrentIndex(selected);
    }
    else {
        ui->cbDeviceList->setCurrentIndex(0);
    }

    ui->gbxParameters->setEnabled(false);
    ui->gbxDevUpdate->setEnabled(false);
    ui->pnlSetButtons->setEnabled(false);

    ui->cbxUpdateDevice->setChecked(false);
    ui->cbxUpdateDevice->setEnabled(false);
    ui->pbSetBaudRate->setEnabled(false);
    ui->pbSetDevAddr->setEnabled(false);

    ui->pbOpenPort->setEnabled(true);
    ui->pbClosePort->setEnabled(false);

    ui->pgRelayAndDigIn->setEnabled(false);
    ui->pgAnalogInRtu->setEnabled(false);
    ui->pgRenogyRtu->setEnabled(false);

    ui->toolBox->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    onAppQuit();
    delete ui;
}

void MainWindow::onAppQuit()
{
    m_modbus.close();

    if (m_rly) {
        m_rly->close();
        m_rly->deleteLater();
        m_rly = nullptr;
    }

    if (m_adc) {
        m_adc->close();
        m_adc->deleteLater();
        m_adc = nullptr;
    }

    saveConfig();
}

inline void MainWindow::loadConfig()
{
    QString str;
    uint value;
    bool numOk;

    m_settings.beginGroup("modbus");
    value = m_config.mbconf.m_baudRate;
    value = m_settings.value("baudRate", value).toUInt(&numOk);
    if (numOk) {
        m_config.mbconf.m_baudRate = //
           static_cast<QSerialPort::BaudRate>(value);
    }

    value = m_config.mbconf.m_dataBits;
    value = m_settings.value("dataBits", value).toUInt(&numOk);
    if (numOk) {
        m_config.mbconf.m_dataBits = //
           static_cast<QSerialPort::DataBits>(value);
    }

    value = m_config.mbconf.m_stopBits;
    value = m_settings.value("stopBits", value).toUInt(&numOk);
    if (numOk) {
        m_config.mbconf.m_stopBits = //
           static_cast<QSerialPort::StopBits>(value);
    }

    value = m_config.mbconf.m_parity;
    value = m_settings.value("parity", value).toUInt(&numOk);
    if (numOk) {
        m_config.mbconf.m_parity = //
           static_cast<QSerialPort::Parity>(value);
    }

    str = m_config.mbconf.m_portName;
    str = m_settings.value("port", value).toString();
    if (!str.isEmpty()) {
        m_config.mbconf.m_portName = str;
    }
    m_settings.endGroup();

    m_settings.beginGroup("devices");
    value = m_config.rlyAddr;
    value = m_settings.value("rlyAddr", value).toUInt(&numOk);
    if (numOk) {
        m_config.rlyAddr = value;
    }
    value = m_config.adcAddr;
    value = m_settings.value("adcAddr", value).toUInt(&numOk);
    if (numOk) {
        m_config.adcAddr = value;
    }
    value = m_config.selDev;
    value = m_settings.value("selDev", value).toUInt(&numOk);
    if (numOk) {
        m_config.selDev = value;
    }
    m_settings.endGroup();
}

inline void MainWindow::saveConfig()
{
    m_settings.beginGroup("modbus");
    m_settings.setValue("baudRate", m_config.mbconf.m_baudRate);
    m_settings.setValue("dataBits", m_config.mbconf.m_dataBits);
    m_settings.setValue("stopBits", m_config.mbconf.m_stopBits);
    m_settings.setValue("parity", m_config.mbconf.m_parity);
    m_settings.setValue("port", m_config.mbconf.m_portName);
    m_settings.endGroup();

    m_settings.beginGroup("devices");
    m_settings.setValue("rlyAddr", m_config.rlyAddr);
    m_settings.setValue("adcAddr", m_config.adcAddr);
    m_settings.setValue("selDev", m_config.selDev);
    m_settings.endGroup();
    m_settings.sync();
}

// Relay Driver ---------------------------------------------------

inline void MainWindow::setRelay(quint8 relay)
{
    if (m_rly) {
        /* toggle on / off */
        bool state = !m_rly->relayStatus(relay);
        /* update */
        m_rly->setRelayStatus(relay, state);
    }
}

void MainWindow::onRelayDriverOpend()
{
    ui->pbOpenPort->setEnabled(false);
    ui->pbClosePort->setEnabled(true);
    ui->pbSetLinkControl->setEnabled(true);
    ui->cbxUpdateDevice->setEnabled(true);
    ui->pnlRelay->setEnabled(true);
    on_cbDeviceList_activated(0);
}

void MainWindow::onRelayDriverClosed()
{
    ui->pbOpenPort->setEnabled(true);
    ui->pbClosePort->setEnabled(false);
    ui->pbSetLinkControl->setEnabled(false);
    on_cbDeviceList_activated(0);
}

void MainWindow::onRelayFunctionDone(uint function)
{
    // qDebug() << "APPWND:onFunctionDone():" << function;
    if (function == WSModbusRtu::RtuReadVersion)
        ui->lbFwVersion->setText(tr("FW Version: %1").arg(m_rly->firmwareVersion()));
    if (function == WSModbusRtu::RtuReadDeviceAddr) {
        ui->lbDevAddress->setText(tr("Dev.Address: %1").arg(m_rly->deviceAddress()));
        ui->edDevAddr->setValue(m_rly->deviceAddress());
    }
}

void MainWindow::onRelayChanged(quint8 relay, bool state)
{
    qDebug() << "APPWND:onRelayChanged(): relay:" << relay << state;

    QPushButton* btn;
    QString key = tr("pbR%1").arg(relay + 1);
    if ((btn = ui->pnlRelay->findChild<QPushButton*>(key))) {
        if (state) {
            btn->setStyleSheet("background-color: green;");
        }
        else {
            btn->setStyleSheet(QString());
        }
    }
}

void MainWindow::onRelayModeChanged(quint8 relay, WSRelayDigInMbRtu::TControlMode mode)
{
    qDebug() << "APPWND:onRelayModeChanged(): relay:" << relay << mode;

    QPushButton* btn;
    QString key = tr("pbR%1").arg(relay + 1);
    if ((btn = ui->pnlRelay->findChild<QPushButton*>(key))) {
        btn->setEnabled(mode == WSRelayDigInMbRtu::NormalMode);
        if (mode != WSRelayDigInMbRtu::NormalMode) {
            btn->setStyleSheet(QString());
        }
    }
}

void MainWindow::onDigInChanged(quint8 channel, bool state)
{
    qDebug() << "APPWND:onDigInChanged(): channel:" << channel << state;

    QCheckBox* cbx;
    QString key = tr("cbxChannel%1").arg(channel + 1);
    if ((cbx = ui->pnlDigitalIn->findChild<QCheckBox*>(key))) {
        cbx->setChecked(state);
    }
}

// ADC Driver ---------------------------------------------------------------------

void MainWindow::onAdcDriverOpend()
{
    ui->pbOpenPort->setEnabled(false);
    ui->pbClosePort->setEnabled(true);
    ui->pbSetChannelType->setEnabled(true);
    on_cbDeviceList_activated(1);
}

void MainWindow::onAdcDriverClosed()
{
    ui->pbOpenPort->setEnabled(true);
    ui->pbClosePort->setEnabled(false);
    ui->pbSetChannelType->setEnabled(false);
    on_cbDeviceList_activated(1);
}

void MainWindow::onAdcFunctionDone(uint function)
{
    // qDebug() << "APPWND:onFunctionDone():" << function;
    if (function == WSModbusRtu::RtuReadVersion)
        ui->txAFwVersion->setText(tr("FW Version: %1").arg(m_adc->firmwareVersion()));
    if (function == WSModbusRtu::RtuReadDeviceAddr) {
        ui->txADeviceAddr->setText(tr("Dev.Address: %1").arg(m_adc->deviceAddress()));
        ui->edDevAddr->setValue(m_adc->deviceAddress());
    }
}

void MainWindow::onAInTypeChanged(quint8 channel, WSAnalogInMbRtu::TChannelType type)
{
    qDebug() << "APPWND:onAInTypeChanged(): channel:" << channel << type;
}

void MainWindow::onAInValueChanged(quint8 channel, float value)
{
    qDebug() << "APPWND:onAInValueChanged(): channel:" << channel << value;
    QLCDNumber* lcd;
    QString key = tr("lcdChannel%1").arg(channel + 1);
    if ((lcd = ui->pnlAnalogIn->findChild<QLCDNumber*>(key))) {
        lcd->display(value);
    }
}

// UI ----------------------------------------------------------------------------

void MainWindow::on_pbEnableDevice_clicked()
{
    QVariant vd = ui->cbDeviceList->currentData();
    if (vd.isNull() || !vd.isValid()) {
        return;
    }
    switch (vd.value<int>()) {
        /* Relay driver */
        case 1: {
            if (m_rly) {
                m_rly->close();
                m_rly->deleteLater();
                m_rly = nullptr;
                on_cbDeviceList_activated(ui->cbDeviceList->currentIndex());
                return;
            }
            m_rly = new WSRelayDigInMbRtu(&m_modbus, this);
            m_rly->setDeviceAddress(m_config.rlyAddr, false);
            connect(m_rly, &WSRelayDigInMbRtu::opened, this, &MainWindow::onRelayDriverOpend);
            connect(m_rly, &WSRelayDigInMbRtu::closed, this, &MainWindow::onRelayDriverClosed);
            connect(m_rly, &WSRelayDigInMbRtu::complete, this, &MainWindow::onRelayFunctionDone);
            connect(m_rly, &WSRelayDigInMbRtu::relayChanged, this, &MainWindow::onRelayChanged);
            connect(m_rly, &WSRelayDigInMbRtu::modeChanged, this, &MainWindow::onRelayModeChanged);
            connect(m_rly, &WSRelayDigInMbRtu::inputChanged, this, &MainWindow::onDigInChanged);
            onRelayFunctionDone(WSRelayDigInMbRtu::RtuReadDeviceAddr);
            onRelayFunctionDone(WSRelayDigInMbRtu::RtuReadVersion);
            on_cbDeviceList_activated(ui->cbDeviceList->currentIndex());
            ui->pnlRelay->setEnabled(m_rly->isValidModbus());
            ui->pbSetLinkControl->setEnabled(m_rly->isValidModbus());
            if (m_modbus.isOpen()) {
                m_rly->open();
            }
            break;
        }
        case 2: {
            if (m_adc) {
                m_adc->close();
                m_adc->deleteLater();
                m_adc = nullptr;
                on_cbDeviceList_activated(ui->cbDeviceList->currentIndex());
                return;
            }
            /* AnalogIn driver */
            m_adc = new WSAnalogInMbRtu(&m_modbus, this);
            m_adc->setDeviceAddress(m_config.adcAddr, false);
            connect(m_adc, &WSAnalogInMbRtu::opened, this, &MainWindow::onAdcDriverOpend);
            connect(m_adc, &WSAnalogInMbRtu::closed, this, &MainWindow::onAdcDriverClosed);
            connect(m_adc, &WSAnalogInMbRtu::complete, this, &MainWindow::onAdcFunctionDone);
            connect(m_adc, &WSAnalogInMbRtu::valueChanged, this, &MainWindow::onAInValueChanged);
            connect(m_adc, &WSAnalogInMbRtu::channelChanged, this, &MainWindow::onAInTypeChanged);
            onAdcFunctionDone(WSRelayDigInMbRtu::RtuReadDeviceAddr);
            onAdcFunctionDone(WSRelayDigInMbRtu::RtuReadVersion);
            on_cbDeviceList_activated(ui->cbDeviceList->currentIndex());
            ui->pbSetChannelType->setEnabled(m_adc->isValidModbus());
            if (m_modbus.isOpen()) {
                m_adc->open();
            }
            break;
        }
        case 3: {
            on_cbDeviceList_activated(ui->cbDeviceList->currentIndex());
            break;
        }
    }
}

void MainWindow::on_cbDeviceList_activated(int index)
{
    QVariant vd = ui->cbDeviceList->itemData(index);
    if (vd.isNull() || !vd.isValid()) {
        return;
    }

    m_config.selDev = index;
    saveConfig();

    QString pfx;
    switch (vd.value<int>()) {
        case 1: {
            pfx = (m_rly ? "Disable" : "Enable");
            ui->pgRelayAndDigIn->setEnabled(true);
            ui->gbxParameters->setEnabled(m_rly != nullptr);
            ui->gbxDevUpdate->setEnabled(m_rly != nullptr);
            ui->pnlSetButtons->setEnabled(m_rly != nullptr);
            ui->pbSetBaudRate->setEnabled(m_rly != nullptr);
            ui->pbSetDevAddr->setEnabled(m_rly != nullptr);
            if (m_rly) {
                ui->cbxUpdateDevice->setEnabled( //
                   m_rly != nullptr && m_rly->isValidModbus());
                onRelayFunctionDone(WSRelayDigInMbRtu::RtuReadDeviceAddr);
                onRelayFunctionDone(WSRelayDigInMbRtu::RtuReadVersion);
            }
            break;
        }
        case 2: {
            pfx = (m_adc ? "Disable" : "Enable");
            ui->pgAnalogInRtu->setEnabled(m_adc != nullptr);
            ui->gbxParameters->setEnabled(m_adc != nullptr);
            ui->gbxDevUpdate->setEnabled(m_adc != nullptr);
            ui->pnlSetButtons->setEnabled(m_adc != nullptr);
            ui->pbSetBaudRate->setEnabled(m_adc != nullptr);
            ui->pbSetDevAddr->setEnabled(m_adc != nullptr);
            if (m_adc) {
                ui->cbxUpdateDevice->setEnabled( //
                   m_adc != nullptr && m_adc->isValidModbus());
                onAdcFunctionDone(WSRelayDigInMbRtu::RtuReadDeviceAddr);
                onAdcFunctionDone(WSRelayDigInMbRtu::RtuReadVersion);
            }
            break;
        }
        case 3: {
            pfx = (m_chg ? "Disable" : "Enable");
            ui->pgRenogyRtu->setEnabled(m_chg != nullptr);
            ui->gbxParameters->setEnabled(m_chg != nullptr);
            ui->gbxDevUpdate->setEnabled(m_chg != nullptr);
            ui->pnlSetButtons->setEnabled(m_chg != nullptr);
            ui->cbxUpdateDevice->setEnabled(m_chg != nullptr);
            ui->pbSetBaudRate->setEnabled(m_chg != nullptr);
            ui->pbSetDevAddr->setEnabled(m_chg != nullptr);
            break;
        }
    }
    ui->pbEnableDevice->setText(pfx + " Device");
}

void MainWindow::on_edDevAddr_valueChanged(int arg1)
{
    m_devAddress = arg1;
}

void MainWindow::on_pbSetDevAddr_clicked()
{
    QVariant v = ui->cbDeviceList->currentData();
    if (v.isNull() || !v.isValid()) {
        return;
    }

    bool update = ui->cbxUpdateDevice->isChecked();

    switch (v.value<int>()) {
        case 1: {
            m_config.rlyAddr = m_devAddress;
            if (m_rly) {
                m_rly->setDeviceAddress(m_devAddress, update);
            }
            break;
        }
        case 2: {
            m_config.adcAddr = m_devAddress;
            if (m_adc) {
                m_adc->setDeviceAddress(m_devAddress, update);
            }
            break;
        }
        case 3: {
            break;
        }
    }

    saveConfig();
}

void MainWindow::on_pbSetBaudRate_clicked()
{
    QVariant vd = ui->cbDeviceList->currentData();
    if (vd.isNull() || !vd.isValid()) {
        return;
    }

    QVariant vb = ui->cbBaudRate->currentData();
    if (vb.isNull() || !vb.isValid()) {
        return;
    }

    QVariant vp = ui->cbParity->currentData();
    if (vp.isNull() || !vp.isValid()) {
        return;
    }

    QVariant vdb = ui->cbDataBits->currentData();
    if (vdb.isNull() || !vdb.isValid()) {
        return;
    }

    QVariant vsb = ui->cbStopBits->currentData();
    if (vsb.isNull() || !vsb.isValid()) {
        return;
    }

    QVariant vcp = ui->cbComPort->currentData();
    if (vcp.isNull() || !vcp.isValid()) {
        return;
    }

    bool update = ui->cbxUpdateDevice->isChecked();
    m_config.mbconf.m_portName = vcp.value<QSerialPortInfo>().portName();
    m_config.mbconf.m_baudRate = vb.value<QSerialPort::BaudRate>();
    m_config.mbconf.m_dataBits = vdb.value<QSerialPort::DataBits>();
    m_config.mbconf.m_stopBits = vsb.value<QSerialPort::StopBits>();
    m_config.mbconf.m_parity = vp.value<QSerialPort::Parity>();

    switch (vd.value<int>()) {
        case 1: {
            if (m_rly) {
                m_rly->setPortName(m_config.mbconf.m_portName);
                m_rly->setDataBits(m_config.mbconf.m_dataBits);
                m_rly->setStopBits(m_config.mbconf.m_stopBits);
                m_rly->setBaudRate(m_config.mbconf.m_baudRate, update);
                m_rly->setParity(m_config.mbconf.m_parity, update);
            }
            break;
        }
        case 2: {
            if (m_adc) {
                m_adc->setPortName(m_config.mbconf.m_portName);
                m_adc->setDataBits(m_config.mbconf.m_dataBits);
                m_adc->setStopBits(m_config.mbconf.m_stopBits);
                m_adc->setBaudRate(m_config.mbconf.m_baudRate, update);
                m_adc->setParity(m_config.mbconf.m_parity, update);
            }
            break;
        }
        case 3: {
            m_modbus.setPortName(m_config.mbconf.m_portName);
            m_modbus.setDataBits(m_config.mbconf.m_dataBits);
            m_modbus.setStopBits(m_config.mbconf.m_stopBits);
            m_modbus.setBaudRate(m_config.mbconf.m_baudRate);
            m_modbus.setParity(m_config.mbconf.m_parity);
            if (m_chg) {
            }
            break;
        }
    }

    saveConfig();
}

void MainWindow::on_pbR1_clicked()
{
    setRelay(0);
}

void MainWindow::on_pbR2_clicked()
{
    setRelay(1);
}

void MainWindow::on_pbR3_clicked()
{
    setRelay(2);
}

void MainWindow::on_pbR4_clicked()
{
    setRelay(3);
}

void MainWindow::on_pbR5_clicked()
{
    setRelay(4);
}

void MainWindow::on_pbR6_clicked()
{
    setRelay(5);
}

void MainWindow::on_pbR7_clicked()
{
    setRelay(6);
}

void MainWindow::on_pbR8_clicked()
{
    setRelay(7);
}

void MainWindow::on_pbRAll_clicked()
{
    // 0xff is a coil address for all relays
    setRelay(0xff);
}

void MainWindow::on_pbSetLinkControl_clicked()
{
    if (!m_rly)
        return;

    DlgRelayLinkControl* dlg;
    if ((dlg = dynamic_cast<DlgRelayLinkControl*>(m_rly->settingsWidget(this)))) {
        if (dlg->exec() == QDialog::Accepted) {
            //
        }
        dlg->deleteLater();
    }
}

void MainWindow::on_pbToggleRelays_clicked()
{
    quint8 mask = 0xff;
    if (m_rly) {
        // TEST: invert current relay state
        for (quint8 b = 0; b < 8; b++) {
            if (m_rly->relayStatus(b)) {
                mask &= ~(1 << b);
            }
            else {
                mask |= (1 << b);
            }
        }

        m_rly->setAllRelays(mask);
    }
}

void MainWindow::on_pbSetChannelType_clicked()
{
    if (!m_adc)
        return;

    DlgAdcInDataType* dlg;
    if ((dlg = dynamic_cast<DlgAdcInDataType*>(m_adc->settingsWidget(this)))) {
        if (dlg->exec() == QDialog::Accepted) {
            //
        }
        dlg->deleteLater();
    }
}

void MainWindow::on_pbOpenPort_clicked()
{
    if (m_rly) {
        if (!m_rly->isValidModbus()) {
            m_rly->open();
        }
    }
    if (m_adc) {
        if (!m_adc->isValidModbus()) {
            m_adc->open();
        }
    }
}

void MainWindow::on_pbClosePort_clicked()
{
    if (m_rly) {
        if (m_rly->isValidModbus()) {
            m_rly->close();
        }
    }
    if (m_adc) {
        if (m_adc->isValidModbus()) {
            m_adc->close();
        }
    }
}
