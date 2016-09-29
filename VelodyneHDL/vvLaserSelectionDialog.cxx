// Copyright 2013 Velodyne Acoustics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "vvLaserSelectionDialog.h"

#include "ui_vvLaserSelectionDialog.h"

#include <pqApplicationCore.h>
#include <pqSettings.h>

#include <QCheckBox>

#include <iostream>

#include <cmath>
#include <cassert>

//-----------------------------------------------------------------------------
class vvLaserSelectionDialog::pqInternal : public Ui::vvLaserSelectionDialog
{
public:
  pqInternal(): Settings(pqApplicationCore::instance()->settings()) {}

  void setup();
  void saveSettings();
  void restoreSettings();

  pqSettings* const Settings;

public:

  QTableWidget* Table;
};

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::pqInternal::saveSettings()
{
  for(int i = 0; i < 64; ++i)
    {
    QTableWidgetItem* item = this->Table->item(i, 0);
    QTableWidgetItem* value = this->Table->item(i, 2);
    int channel = value->data(Qt::EditRole).toInt();

    bool checked = item->checkState() == Qt::Checked;
    this->Settings->setValue(QString("VelodyneHDLPlugin/LaserSelectionDialog%1").arg(channel),
                             checked);
    }
}

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::pqInternal::restoreSettings()
{
  QVector<int> channel2index(64, 0);

  for(int i = 0; i < 64; ++i)
    {
    QTableWidgetItem* value = this->Table->item(i, 2);
    int channel = value->data(Qt::EditRole).toInt();
    channel2index[channel] = i;
    }

  for(int c = 0; c < 64; ++c)
    {
    bool checked = this->Settings->value(QString("VelodyneHDLPlugin/LaserSelectionDialog%1").arg(c),
                                         QVariant::fromValue(true)).toBool();
    int index = channel2index[c];

    QTableWidgetItem* item = this->Table->item(index, 0);
    item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
}

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::pqInternal::setup()
{
  QTableWidget* table = this->LaserTable;
  QAbstractItemModel* model = table->model();

  table->setColumnWidth(0, 32);
  table->setColumnWidth(1, 64);

  for (int i = 2; i < 13; i++)
  {
  table->setColumnWidth(i, 128);
  }

  // Set checkable header
  QTableWidgetItem* hcheckbox = new QTableWidgetItem();
  hcheckbox->setCheckState(Qt::Checked);
  hcheckbox->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

  table->setHorizontalHeaderItem(0, hcheckbox);

  for(size_t i = 0; i < 64; ++i)
    {
    table->insertRow(i);

    QTableWidgetItem* checkbox = new QTableWidgetItem();
    checkbox->setCheckState(Qt::Checked);
    checkbox->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    checkbox->setTextAlignment(Qt::AlignHCenter);

    for (int j = 2; j < 13; j++)
      {
      QTableWidgetItem* values = new QTableWidgetItem;
      values->setData(Qt::EditRole, 0.0);
      values->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

      table->setItem(i, j, values);
      }
    

    QTableWidgetItem* channel = new QTableWidgetItem;
    channel->setData(Qt::EditRole, QVariant::fromValue(i));
    channel->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    table->setItem(i, 0, checkbox);
    
    table->setItem(i, 1, channel);
    }

  QTableWidgetItem* leadBox = table->horizontalHeaderItem(0);
  leadBox->setFlags(Qt::ItemIsUserCheckable);

  this->EnableDisableAll->setTristate(false);

  this->Table = table;
}

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::onToggleSelected()
{
  // Update other selected items
  QList<QTableWidgetItem*> list = this->Internal->Table->selectedItems();
  foreach(QTableWidgetItem* sitem, list)
    {
    if(sitem->column() == 0)
      {
      sitem->setCheckState(sitem->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
      }
    }
}

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::onItemChanged(QTableWidgetItem* item)
{
  // Set enable/disable all based on all checked
  bool allChecked = true;
  bool noneChecked = true;
  //
  for(int i = 0; i < this->Internal->Table->rowCount(); ++i)
    {
    QTableWidgetItem* item = this->Internal->Table->item(i, 0);
    allChecked = allChecked && item->checkState() == Qt::Checked;
    noneChecked = noneChecked && item->checkState() == Qt::Unchecked;
    }
  Qt::CheckState state;
  if(allChecked)
    {
    state = Qt::Checked;
    }
  else if(noneChecked)
    {
    state = Qt::Unchecked;
    }
  else
    {
    state = Qt::PartiallyChecked;
    }
  this->Internal->EnableDisableAll->setCheckState(state);
  if(state != Qt::PartiallyChecked)
    {
    this->Internal->EnableDisableAll->setTristate(false);
    }
}

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::onEnableDisableAll(int state)
{
  if(state != Qt::PartiallyChecked)
    {
    // enable all
    for(int i = 0; i < this->Internal->Table->rowCount(); ++i)
      {
      QTableWidgetItem* item = this->Internal->Table->item(i, 0);
      item->setCheckState(Qt::CheckState(state));
      }
    }
}

//-----------------------------------------------------------------------------
vvLaserSelectionDialog::vvLaserSelectionDialog(QWidget *p) : QDialog(p)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->setup();
  this->Internal->restoreSettings();

  QObject::connect(this->Internal->Table, SIGNAL(itemChanged(QTableWidgetItem*)),
                   this, SLOT(onItemChanged(QTableWidgetItem*)));

  QObject::connect(this->Internal->Toggle, SIGNAL(clicked()),
                   this, SLOT(onToggleSelected()));

  QObject::connect(this->Internal->EnableDisableAll, SIGNAL(stateChanged(int)),
                   this, SLOT(onEnableDisableAll(int)));

  this->Internal->Table->setSortingEnabled(true);
}

//-----------------------------------------------------------------------------
QVector<int> vvLaserSelectionDialog::getLaserSelectionSelector()
{
  QVector<int> result(64, 1);
  for(int i = 0; i < this->Internal->Table->rowCount(); ++i)
    {
    QTableWidgetItem* value = this->Internal->Table->item(i, 1);
    int channel = value->data(Qt::EditRole).toInt();
    assert(channel < 64 && channel >= 0);
    QTableWidgetItem* item = this->Internal->Table->item(i, 0);
    result[channel] = (item->checkState() == Qt::Checked);
    }
  return result;
}

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::setLasersCorrections(const QVector<double>& verticalCorrection,
                                                const QVector<double>& rotationalCorrection,
                                                const QVector<double>& distanceCorrection,
                                                const QVector<double>& distanceCorrectionX,
                                                const QVector<double>& distanceCorrectionY,
                                                const QVector<double>& verticalOffsetCorrection,
                                                const QVector<double>& horizontalOffsetCorrection,
                                                const QVector<double>& focalDistance,
                                                const QVector<double>& focalSlope,
                                                const QVector<double>& minIntensity,
                                                const QVector<double>& maxIntensity,
                                                int nchannels)
                                                {
  for(int i = 0; i < this->Internal->Table->rowCount(); ++i)
    {
    QTableWidgetItem* value = this->Internal->Table->item(i, 1);
    int channel = value->data(Qt::EditRole).toInt();

    assert(channel < 64 && channel >= 0);
    int col=2;
    QTableWidgetItem* item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, verticalCorrection[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, rotationalCorrection[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, distanceCorrection[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, distanceCorrectionX[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, distanceCorrectionY[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, verticalOffsetCorrection[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, horizontalOffsetCorrection[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, focalDistance[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, focalSlope[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, minIntensity[channel]);

    item = this->Internal->Table->item(i, col++);
    item->setData(Qt::EditRole, maxIntensity[channel]);

    }

  if(nchannels > this->Internal->Table->rowCount())
    {
    nchannels = this->Internal->Table->rowCount();
    }

  for(int i = nchannels; i < this->Internal->Table->rowCount(); ++i)
    {
    this->Internal->Table->hideRow(i);
    }

  // Sort the table by vertical correction
  this->Internal->Table->sortItems(2);
}

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::setLaserSelectionSelector(const QVector<int>& mask)
{
  for(int i = 0; i < this->Internal->Table->rowCount(); ++i)
    {
    QTableWidgetItem* item = this->Internal->Table->item(i, 0);
    QTableWidgetItem* value = this->Internal->Table->item(i, 1);
    int channel = value->data(Qt::EditRole).toInt();
    assert(channel < 64 && channel >= 0);
    item->setCheckState(mask[channel] ? Qt::Checked : Qt::Unchecked);
    }
}

//-----------------------------------------------------------------------------
vvLaserSelectionDialog::~vvLaserSelectionDialog()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vvLaserSelectionDialog::accept()
{
  if(this->Internal->saveCheckBox->isChecked())
    {
    this->Internal->saveSettings();
    }
  QDialog::accept();
}
