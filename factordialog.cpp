/******************************************************/
/*                                                    */
/* factordialog.cpp - scale factor dialog             */
/*                                                    */
/******************************************************/
/* Copyright 2018 Pierre Abbat.
 * This file is part of Bezitopo.
 * 
 * Bezitopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bezitopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bezitopo. If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include "factordialog.h"
using namespace std;

LatlongFactorDialog::LatlongFactorDialog(QWidget *parent):QDialog(parent)
{
  latlongLabel=new QLabel(tr("Lat/Long"),this);
  latlongInput=new QLineEdit(this);
  elevationLabel=new QLabel(tr("Elevation"),this);
  elevationInput=new QLineEdit(this);
  plWidget=new ProjListWidget(this);
  okButton=new QPushButton(tr("OK"),this);
  cancelButton=new QPushButton(tr("Cancel"),this);
  gridLayout=new QGridLayout(this);
  setLayout(gridLayout);
  gridLayout->addWidget(latlongLabel,0,0);
  gridLayout->addWidget(latlongInput,0,1);
  gridLayout->addWidget(elevationLabel,1,0);
  gridLayout->addWidget(elevationInput,1,1);
  gridLayout->addWidget(plWidget,2,0,1,2);
  gridLayout->addWidget(okButton,3,0);
  gridLayout->addWidget(cancelButton,3,1);
  okButton->setEnabled(false);
  okButton->setDefault(true);
  connect(okButton,SIGNAL(clicked()),this,SLOT(accept()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
}

void LatlongFactorDialog::accept()
{
  QDialog::accept();
}