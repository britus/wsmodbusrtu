#include "ui_dlgrelaylinkcontrol.h"
#include <dlgrelaylinkcontrol.h>

DlgRelayLinkControl::DlgRelayLinkControl(WSRelayDigInMbRtu* rtu, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DlgRelayLinkControl)
    , m_rtu(rtu)
    , m_channel(0)
    , m_control()
    , m_btnSave(nullptr)
{
    ui->setupUi(this);

    ui->cbChannel->clear();
    for (quint8 i = 0; i < m_rtu->maxInputs(); i++) {
        QString name = tr("Channel %1").arg(i + 1);
        ui->cbChannel->addItem(name, QVariant());
        m_control[i] = rtu->controlMode(i);
    }

    m_btnSave = ui->buttonBox->button(QDialogButtonBox::Save);
    m_btnSave->setEnabled(false);

    /* initial selection */
    ui->cbChannel->setCurrentIndex(0);

    /* initial active radio button */
    on_cbChannel_activated(0);
}

uint DlgRelayLinkControl::controlMode(quint8 channel)
{
    return m_control[channel];
}

void DlgRelayLinkControl::setControlModes(const QMap<quint8, WSRelayDigInMbRtu::TControlMode>& modes)
{
    m_control = modes;
}

const QMap<quint8, WSRelayDigInMbRtu::TControlMode>& DlgRelayLinkControl::controlModes() const
{
    return m_control;
}

inline void DlgRelayLinkControl::setModeOption(quint8 channel, WSRelayDigInMbRtu::TControlMode mode)
{
    m_control[channel] = mode;
    m_channel = channel;

    switch (mode) {
        case WSRelayDigInMbRtu::NormalMode: {
            ui->rbMode0->setChecked(true);
            ui->rbMode1->setChecked(false);
            ui->rbMode2->setChecked(false);
            break;
        }
        case WSRelayDigInMbRtu::LinkageMode: {
            ui->rbMode0->setChecked(false);
            ui->rbMode1->setChecked(true);
            ui->rbMode2->setChecked(false);
            break;
        }
        case WSRelayDigInMbRtu::ToggleMode: {
            ui->rbMode0->setChecked(false);
            ui->rbMode1->setChecked(false);
            ui->rbMode2->setChecked(true);
            break;
        }
    }
}

DlgRelayLinkControl::~DlgRelayLinkControl()
{
    delete ui;
}

void DlgRelayLinkControl::on_cbChannel_activated(int index)
{
    quint8 channel = static_cast<quint8>(index);
    setModeOption(channel, m_control[channel]);
}

void DlgRelayLinkControl::on_rbMode0_clicked(bool checked)
{
    if (checked) {
        m_control[m_channel] = WSRelayDigInMbRtu::NormalMode;
        m_btnSave->setEnabled(true);
    }
}

void DlgRelayLinkControl::on_rbMode1_clicked(bool checked)
{
    if (checked) {
        m_control[m_channel] = WSRelayDigInMbRtu::LinkageMode;
        m_btnSave->setEnabled(true);
    }
}

void DlgRelayLinkControl::on_rbMode2_clicked(bool checked)
{
    if (checked) {
        m_control[m_channel] = WSRelayDigInMbRtu::ToggleMode;
        m_btnSave->setEnabled(true);
    }
}

void DlgRelayLinkControl::on_buttonBox_accepted()
{
    m_rtu->setControlModes(m_control, true);
}
