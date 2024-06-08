/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: GPL v3
 **********************************************************************/
#include "ui_dlgadcindatatype.h"
#include <dlgadcindatatype.h>

DlgAdcInDataType::DlgAdcInDataType(WSAnalogInMbRtu* rtu, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DlgAdcInDataType)
    , m_rtu(rtu)
    , m_channelTypes()
    , m_channel(0)
{
    ui->setupUi(this);

    ui->cbChannel->clear();
    for (quint8 i = 0; i < m_rtu->maxInputs(); i++) {
        QString name = tr("Channel %1").arg(i + 1);
        ui->cbChannel->addItem(name, QVariant());
        m_channelTypes[i] = rtu->channelType(i);
    }

    m_btnSave = ui->buttonBox->button(QDialogButtonBox::Save);
    m_btnSave->setEnabled(false);

    /* initial selection */
    ui->cbChannel->setCurrentIndex(0);

    /* initial active radio button */
    on_cbChannel_activated(0);
}

DlgAdcInDataType::~DlgAdcInDataType()
{
    delete ui;
}

uint DlgAdcInDataType::channelType(quint8 channel)
{
    return m_channelTypes[channel];
}

void DlgAdcInDataType::setChannelTypes(const QMap<quint8, WSAnalogInMbRtu::TChannelType>& types)
{
    m_channelTypes = types;
}

const QMap<quint8, WSAnalogInMbRtu::TChannelType>& DlgAdcInDataType::channelTypes() const
{
    return m_channelTypes;
}

inline void DlgAdcInDataType::setTypeOption(quint8 channel, WSAnalogInMbRtu::TChannelType type)
{
    m_channelTypes[channel] = type;
    m_channel = channel;

    switch (type) {
        case WSAnalogInMbRtu::Range0_5000mV: {
            ui->rbMode0->setChecked(true);
            ui->rbMode1->setChecked(false);
            ui->rbMode2->setChecked(false);
            ui->rbMode3->setChecked(false);
            ui->rbMode4->setChecked(false);
            break;
        }
        case WSAnalogInMbRtu::Range1000_5000mV: {
            ui->rbMode0->setChecked(false);
            ui->rbMode1->setChecked(true);
            ui->rbMode2->setChecked(false);
            ui->rbMode3->setChecked(false);
            ui->rbMode4->setChecked(false);
            break;
        }
        case WSAnalogInMbRtu::Range0_20000uA: {
            ui->rbMode0->setChecked(false);
            ui->rbMode1->setChecked(false);
            ui->rbMode2->setChecked(true);
            ui->rbMode3->setChecked(false);
            ui->rbMode4->setChecked(false);
            break;
        }
        case WSAnalogInMbRtu::Range4000_20000uA: {
            ui->rbMode0->setChecked(false);
            ui->rbMode1->setChecked(false);
            ui->rbMode2->setChecked(false);
            ui->rbMode3->setChecked(true);
            ui->rbMode4->setChecked(false);
            break;
        }
        case WSAnalogInMbRtu::RangeDirect4096: {
            ui->rbMode0->setChecked(false);
            ui->rbMode1->setChecked(false);
            ui->rbMode2->setChecked(false);
            ui->rbMode3->setChecked(false);
            ui->rbMode4->setChecked(true);
            break;
        }
    }
}

void DlgAdcInDataType::on_cbChannel_activated(int index)
{
    quint8 channel = static_cast<quint8>(index);
    setTypeOption(channel, m_channelTypes[channel]);
}

void DlgAdcInDataType::on_rbMode0_clicked(bool checked)
{
    if (checked) {
        m_channelTypes[m_channel] = WSAnalogInMbRtu::Range0_5000mV;
        m_btnSave->setEnabled(true);
    }
}

void DlgAdcInDataType::on_rbMode1_clicked(bool checked)
{
    if (checked) {
        m_channelTypes[m_channel] = WSAnalogInMbRtu::Range1000_5000mV;
        m_btnSave->setEnabled(true);
    }
}

void DlgAdcInDataType::on_rbMode2_clicked(bool checked)
{
    if (checked) {
        m_channelTypes[m_channel] = WSAnalogInMbRtu::Range0_20000uA;
        m_btnSave->setEnabled(true);
    }
}

void DlgAdcInDataType::on_rbMode3_clicked(bool checked)
{
    if (checked) {
        m_channelTypes[m_channel] = WSAnalogInMbRtu::Range4000_20000uA;
        m_btnSave->setEnabled(true);
    }
}

void DlgAdcInDataType::on_rbMode4_clicked(bool checked)
{
    if (checked) {
        m_channelTypes[m_channel] = WSAnalogInMbRtu::RangeDirect4096;
        m_btnSave->setEnabled(true);
    }
}

void DlgAdcInDataType::on_buttonBox_accepted()
{
    m_rtu->setChannelTypes(m_channelTypes, true);
}
