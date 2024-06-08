#include "ui_mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QModbusDataUnit>
#include <QSerialPortInfo>
#include <dlgadcindatatype.h>
#include <dlgrelaylinkcontrol.h>
#include <mainwindow.h>

Q_DECLARE_METATYPE(QSerialPortInfo)
Q_DECLARE_METATYPE(QSerialPort::BaudRate)
Q_DECLARE_METATYPE(QSerialPort::DataBits)
Q_DECLARE_METATYPE(QSerialPort::StopBits)
Q_DECLARE_METATYPE(QSerialPort::Parity)

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_modbus(this)
    , m_rly(nullptr)
    , m_adc(nullptr)
    , m_chg(nullptr)
{
    ui->setupUi(this);

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::onAppQuit);

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    int selected = -1;

    /* Global MODBUS */
    ui->cbComPort->clear();
    for (int i = 0; i < ports.count(); i++) {
        ui->cbComPort->addItem(ports[i].portName(), QVariant::fromValue(ports[i]));
        if (ports[i].portName().contains(m_modbus.portName())) {
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
        if (m_modbus.baudRate() == baudRates[i].baud) {
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
        if (m_modbus.dataBits() == dataBits[i].bits) {
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
        if (m_modbus.stopBits() == stopBits[i].bits) {
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
        if (m_modbus.parity() == parity[i].parity) {
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

    ui->cbDeviceType->clear();
    ui->cbDeviceList->clear();
    for (int i = 0; i < 3; i++) {
        ui->cbDeviceType->addItem(devices[i].name, QVariant::fromValue(devices[i].index));
        ui->cbDeviceList->addItem(devices[i].name, QVariant::fromValue(devices[i].index));
    }
    ui->cbDeviceType->setCurrentIndex(0);
    ui->cbDeviceList->setCurrentIndex(0);

    ui->gbxParameters->setEnabled(false);
    ui->gbxDevUpdate->setEnabled(false);
    ui->pnlSetButtons->setEnabled(false);

    ui->cbxUpdateDevice->setChecked(false);
    ui->cbxUpdateDevice->setEnabled(false);
    ui->pbSetBaudRate->setEnabled(false);
    ui->pbSetDevAddr->setEnabled(false);

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
    m_modbus.close();
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
    ui->pbOpen->setEnabled(false);
    ui->pbSetLinkControl->setEnabled(true);
    ui->cbxUpdateDevice->setEnabled(true);
    ui->pnlRelay->setEnabled(true);
    /* update page */
    on_cbDeviceList_activated(0);
    on_cbDeviceType_activated(0);
}

void MainWindow::onRelayDriverClosed()
{
    ui->pbOpen->setEnabled(true);
    ui->pbSetLinkControl->setEnabled(false);
    /* remove driver instance */
    m_rly->deleteLater();
    m_rly = nullptr;
    /* update page */
    on_cbDeviceList_activated(0);
    on_cbDeviceType_activated(0);
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
    if ((cbx = ui->pnlAnalogIn->findChild<QCheckBox*>(key))) {
        cbx->setChecked(state);
    }
}

// ADC Driver ---------------------------------------------------------------------

void MainWindow::onAdcDriverOpend()
{
    ui->pbOpenAnalog->setEnabled(false);
    ui->pbSetChannelType->setEnabled(true);
    /* update page */
    on_cbDeviceList_activated(1);
    on_cbDeviceType_activated(1);
}

void MainWindow::onAdcDriverClosed()
{
    ui->pbOpenAnalog->setEnabled(true);
    ui->pbSetChannelType->setEnabled(false);
    /* remove driver instance */
    m_adc->deleteLater();
    m_adc = nullptr;
    /* update page */
    on_cbDeviceList_activated(1);
    on_cbDeviceType_activated(1);
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
            connect(m_rly, &WSRelayDigInMbRtu::opened, this, &MainWindow::onRelayDriverOpend);
            connect(m_rly, &WSRelayDigInMbRtu::closed, this, &MainWindow::onRelayDriverClosed);
            connect(m_rly, &WSRelayDigInMbRtu::complete, this, &MainWindow::onRelayFunctionDone);
            connect(m_rly, &WSRelayDigInMbRtu::relayChanged, this, &MainWindow::onRelayChanged);
            connect(m_rly, &WSRelayDigInMbRtu::modeChanged, this, &MainWindow::onRelayModeChanged);
            connect(m_rly, &WSRelayDigInMbRtu::inputChanged, this, &MainWindow::onDigInChanged);
            onRelayFunctionDone(WSRelayDigInMbRtu::RtuReadDeviceAddr);
            onRelayFunctionDone(WSRelayDigInMbRtu::RtuReadVersion);
            on_cbDeviceList_activated(ui->cbDeviceList->currentIndex());
            on_cbDeviceType_activated(ui->cbDeviceType->currentIndex());
            ui->pnlRelay->setEnabled(m_rly->isValidModbus());
            ui->pbSetLinkControl->setEnabled(m_rly->isValidModbus());
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
            connect(m_adc, &WSAnalogInMbRtu::opened, this, &MainWindow::onAdcDriverOpend);
            connect(m_adc, &WSAnalogInMbRtu::closed, this, &MainWindow::onAdcDriverClosed);
            connect(m_adc, &WSAnalogInMbRtu::complete, this, &MainWindow::onAdcFunctionDone);
            connect(m_adc, &WSAnalogInMbRtu::valueChanged, this, &MainWindow::onAInValueChanged);
            connect(m_adc, &WSAnalogInMbRtu::channelChanged, this, &MainWindow::onAInTypeChanged);
            onAdcFunctionDone(WSRelayDigInMbRtu::RtuReadDeviceAddr);
            onAdcFunctionDone(WSRelayDigInMbRtu::RtuReadVersion);
            on_cbDeviceList_activated(ui->cbDeviceList->currentIndex());
            on_cbDeviceType_activated(ui->cbDeviceType->currentIndex());
            ui->pbSetChannelType->setEnabled(m_adc->isValidModbus());
            break;
        }
        case 3: {
            on_cbDeviceList_activated(ui->cbDeviceList->currentIndex());
            on_cbDeviceType_activated(ui->cbDeviceType->currentIndex());
            break;
        }
    }
}

void MainWindow::on_cbDeviceList_activated(int index)
{
    ui->cbDeviceType->setCurrentIndex(index);

    QVariant vd = ui->cbDeviceList->itemData(index);
    if (vd.isNull() || !vd.isValid()) {
        return;
    }
    QString pfx;
    switch (vd.value<int>()) {
        case 1: {
            pfx = (m_rly ? "Disable" : "Enable");
            ui->pgRelayAndDigIn->setEnabled(true);
            ui->gbxParameters->setEnabled(m_rly != nullptr);
            ui->gbxDevUpdate->setEnabled(m_rly != nullptr);
            ui->pnlSetButtons->setEnabled(m_rly != nullptr);
            if (m_rly) {
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
            if (m_adc) {
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
            break;
        }
    }
    ui->pbEnableDevice->setText(pfx + " Device");
}

void MainWindow::on_edDevAddr_valueChanged(int arg1)
{
    m_devAddress = arg1;
}

void MainWindow::on_cbDeviceType_activated(int index)
{
    QVariant vd = ui->cbDeviceType->itemData(index);
    if (vd.isNull() || !vd.isValid()) {
        return;
    }
    switch (vd.value<int>()) {
        case 1: {
            ui->cbxUpdateDevice->setEnabled( //
               m_rly != nullptr && m_rly->isValidModbus());
            ui->pbSetBaudRate->setEnabled(m_rly != nullptr);
            ui->pbSetDevAddr->setEnabled(m_rly != nullptr);
            break;
        }
        case 2: {
            ui->cbxUpdateDevice->setEnabled( //
               m_adc != nullptr && m_adc->isValidModbus());
            ui->pbSetBaudRate->setEnabled(m_adc != nullptr);
            ui->pbSetDevAddr->setEnabled(m_adc != nullptr);
            break;
        }
        case 3: {
            ui->cbxUpdateDevice->setEnabled(m_chg != nullptr);
            ui->pbSetBaudRate->setEnabled(m_chg != nullptr);
            ui->pbSetDevAddr->setEnabled(m_chg != nullptr);
            break;
        }
    }
}

void MainWindow::on_pbSetDevAddr_clicked()
{
    QVariant v = ui->cbDeviceType->currentData();
    if (v.isNull() || !v.isValid()) {
        return;
    }

    bool update = ui->cbxUpdateDevice->isChecked();

    switch (v.value<int>()) {
        case 1: {
            if (m_rly) {
                m_rly->setDeviceAddress(m_devAddress, update);
            }
            break;
        }
        case 2: {
            if (m_adc) {
                m_adc->setDeviceAddress(m_devAddress, update);
            }
            break;
        }
        case 3: {
            break;
        }
    }
}

void MainWindow::on_pbSetBaudRate_clicked()
{
    QVariant vd = ui->cbDeviceType->currentData();
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

    QString portName = vcp.value<QSerialPortInfo>().portName();
    QSerialPort::BaudRate rate = vb.value<QSerialPort::BaudRate>();
    QSerialPort::DataBits dataBits = vdb.value<QSerialPort::DataBits>();
    QSerialPort::StopBits stopBits = vsb.value<QSerialPort::StopBits>();
    QSerialPort::Parity parity = vp.value<QSerialPort::Parity>();

    bool update = ui->cbxUpdateDevice->isChecked();

    switch (vd.value<int>()) {
        case 1: {
            if (m_rly) {
                m_rly->setPortName(portName);
                m_rly->setDataBits(dataBits);
                m_rly->setStopBits(stopBits);
                m_rly->setBaudRate(rate, update);
                m_rly->setParity(parity, update);
            }
            break;
        }
        case 2: {
            if (m_adc) {
                m_adc->setPortName(portName);
                m_adc->setDataBits(dataBits);
                m_adc->setStopBits(stopBits);
                m_adc->setBaudRate(rate, update);
                m_adc->setParity(parity, update);
            }
            break;
        }
        case 3: {
            if (m_chg) {
            }
            break;
        }
    }
}

void MainWindow::on_pbOpen_clicked()
{
    if (m_rly) {
        m_rly->open();
    }
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

void MainWindow::on_pbOpenAnalog_clicked()
{
    if (m_adc) {
        m_adc->open();
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
