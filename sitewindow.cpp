/******************************************************/
/*                                                    */
/* sitewindow.cpp - window for checking site          */
/*                                                    */
/******************************************************/
/* Copyright 2017-2019 Pierre Abbat.
 * This file is part of Bezitopo.
 *
 * Bezitopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Bezitopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License and Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and Lesser General Public License along with Bezitopo. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <cmath>
#include "except.h"
#include "sitewindow.h"
#include "zoom.h"
#include "test.h"
#include "ldecimal.h"
#include "color.h"
#include "penwidth.h"
#include "dxf.h"

#define CACHEDRAW

using namespace std;

string baseName(string fileName)
{
  long long slashPos;
  slashPos=fileName.rfind('/');
  return fileName.substr(slashPos+1);
}

SiteWindow::SiteWindow(QWidget *parent):QMainWindow(parent)
{
  resize(707,500);
  showFileLoaded("");
  show();
  toolbar=new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,toolbar);
  //toolbar->setIconSize(QSize(40,40));
  canvas=new TopoCanvas(this);
  setCentralWidget(canvas);
  llDialog=new LatlongFactorDialog(this);
  grDialog=new GridFactorDialog(this);
  canvas->setShowDelaunay(false);
  canvas->setAllowFlip(false);
  canvas->setTipXyz(true);
  canvas->show();
  makeActions();
  canvas->setMeter();
  connect(this,SIGNAL(zoomCanvas(int)),canvas,SLOT(zoom(int)));
  connect(canvas,SIGNAL(fileChanged(std::string)),this,SLOT(showFileLoaded(std::string)));
}

SiteWindow::~SiteWindow()
{
  unmakeActions();
  delete canvas;
}

void SiteWindow::makeActions()
{
  int i;
  fileMenu=menuBar()->addMenu(tr("&File"));
  editMenu=menuBar()->addMenu(tr("&Edit"));
  viewMenu=menuBar()->addMenu(tr("&View"));
  unitsMenu=menuBar()->addMenu(tr("&Units"));
  contourMenu=menuBar()->addMenu(tr("&Contour"));
  coordMenu=menuBar()->addMenu(tr("&Coordinates"));
  helpMenu=menuBar()->addMenu(tr("&Help"));
  // View menu
  zoomButtons.push_back(new ZoomButton(this,-10));
  zoomButtons.back()->setIcon(QIcon(":/tenth.png"));
  zoomButtons.back()->setText(tr("Zoom out 10"));
  connect(zoomButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(zoomm10()));
  zoomButtons.push_back(new ZoomButton(this,-3));
  zoomButtons.back()->setIcon(QIcon(":/half.png"));
  zoomButtons.back()->setText(tr("Zoom out 2"));
  connect(zoomButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(zoomm3()));
  zoomButtons.push_back(new ZoomButton(this,-1));
  zoomButtons.back()->setIcon(QIcon(":/four-fifths.png"));
  zoomButtons.back()->setText(tr("Zoom out"));
  connect(zoomButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(zoomm1()));
  zoomButtons.push_back(new ZoomButton(this,1));
  zoomButtons.back()->setIcon(QIcon(":/five-fourths.png"));
  zoomButtons.back()->setText(tr("Zoom in"));
  connect(zoomButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(zoomp1()));
  zoomButtons.push_back(new ZoomButton(this,3));
  zoomButtons.back()->setIcon(QIcon(":/two.png"));
  zoomButtons.back()->setText(tr("Zoom in 2"));
  connect(zoomButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(zoomp3()));
  zoomButtons.push_back(new ZoomButton(this,10));
  zoomButtons.back()->setIcon(QIcon(":/ten.png"));
  zoomButtons.back()->setText(tr("Zoom in 10"));
  connect(zoomButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(zoomp10()));
  zoomButtons.push_back(new ZoomButton(this,0,-DEG45/4));
  zoomButtons.back()->setIcon(QIcon(":/cw.png"));
  zoomButtons.back()->setText(tr("Rotate right"));
  connect(zoomButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(rotatecw()));
  zoomButtons.push_back(new ZoomButton(this,0,DEG45/4));
  zoomButtons.back()->setIcon(QIcon(":/ccw.png"));
  zoomButtons.back()->setText(tr("Rotate left"));
  connect(zoomButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(rotateccw()));
  for (i=0;i<zoomButtons.size();i++)
  {
    toolbar->addAction(zoomButtons[i]);
    viewMenu->addAction(zoomButtons[i]);
  }
  sizeToFitAction=new QAction(this);
  //sizeToFitAction->setIcon(QIcon(":/sizetofit.png"));
  sizeToFitAction->setText(tr("Size to Fit"));
  viewMenu->addAction(sizeToFitAction);
  connect(sizeToFitAction,SIGNAL(triggered(bool)),canvas,SLOT(sizeToFit()));
  // File menu
  openAction=new QAction(this);
  openAction->setIcon(QIcon::fromTheme("document-open"));
  openAction->setText(tr("Open"));
  fileMenu->addAction(openAction);
  connect(openAction,SIGNAL(triggered(bool)),canvas,SLOT(open()));
  saveAction=new QAction(this);
  saveAction->setIcon(QIcon::fromTheme("document-save"));
  saveAction->setText(tr("Save"));
  fileMenu->addAction(saveAction);
  connect(saveAction,SIGNAL(triggered(bool)),canvas,SLOT(save()));
  saveAsAction=new QAction(this);
  saveAsAction->setIcon(QIcon::fromTheme("document-save-as"));
  saveAsAction->setText(tr("Save As"));
  fileMenu->addAction(saveAsAction);
  connect(saveAsAction,SIGNAL(triggered(bool)),canvas,SLOT(saveAs()));
  exitAction=new QAction(this);
  exitAction->setIcon(QIcon::fromTheme("application-exit"));
  exitAction->setText(tr("Exit"));
  fileMenu->addAction(exitAction);
  connect(exitAction,SIGNAL(triggered(bool)),this,SLOT(close()));
  // Contour menu
  selectContourIntervalAction=new QAction(this);
  //makeTinAction->setIcon(QIcon(":/selectci.png"));
  selectContourIntervalAction->setText(tr("Select contour interval"));
  contourMenu->addAction(selectContourIntervalAction);
  connect(selectContourIntervalAction,SIGNAL(triggered(bool)),canvas,SLOT(selectContourInterval()));
  roughContoursAction=new QAction(this);
  //makeTinAction->setIcon(QIcon(":/roughcon.png"));
  roughContoursAction->setText(tr("Draw rough contours"));
  contourMenu->addAction(roughContoursAction);
  connect(roughContoursAction,SIGNAL(triggered(bool)),canvas,SLOT(roughContours()));
  smoothContoursAction=new QAction(this);
  //smoothContoursAction->setIcon(QIcon(":/smoothcon.png"));
  smoothContoursAction->setText(tr("Draw smooth contours"));
  contourMenu->addAction(smoothContoursAction);
  connect(smoothContoursAction,SIGNAL(triggered(bool)),canvas,SLOT(smoothContours()));
  curvyContourAction=new QAction(this);
  //curvyContourAction->setIcon(QIcon(":/curvycon.png"));
  curvyContourAction->setText(tr("Draw smooth contours with curves"));
  curvyContourAction->setCheckable(true);
  contourMenu->addAction(curvyContourAction);
  connect(curvyContourAction,SIGNAL(triggered(bool)),this,SLOT(changeButtonBits()));
  curvyContourAction->setChecked(true);
#ifndef FLATTRIANGLE
  curvyTriangleAction=new QAction(this);
  //curvyTriangleAction->setIcon(QIcon(":/curvytri.png"));
  curvyTriangleAction->setText(tr("Use curved triangular surfaces"));
  curvyTriangleAction->setCheckable(true);
  contourMenu->addAction(curvyTriangleAction);
  connect(curvyTriangleAction,SIGNAL(triggered(bool)),this,SLOT(changeButtonBits()));
  curvyTriangleAction->setChecked(true);
  connect(this,SIGNAL(buttonBitsChanged(int)),canvas,SLOT(setButtonBits(int)));
#endif
  // Coordinate menu
  loadGeoidAction=new QAction(this);
  //loadGeoidAction->setIcon(QIcon(":/loadgeoid.png"));
  loadGeoidAction->setText(tr("Load geoid file"));
  coordMenu->addAction(loadGeoidAction);
  connect(loadGeoidAction,SIGNAL(triggered(bool)),canvas,SLOT(loadGeoid()));
  gridToLatlongAction=new QAction(this);
  //gridToLatlongAction->setIcon(QIcon(":/gridtoll.png"));
  gridToLatlongAction->setText(tr("Grid to lat/long"));
  coordMenu->addAction(gridToLatlongAction);
  connect(gridToLatlongAction,SIGNAL(triggered(bool)),this,SLOT(gridToLatlong()));
  latlongToGridAction=new QAction(this);
  //latlongToGridAction->setIcon(QIcon(":/lltogrid.png"));
  latlongToGridAction->setText(tr("Lat/long to grid"));
  coordMenu->addAction(latlongToGridAction);
  connect(latlongToGridAction,SIGNAL(triggered(bool)),this,SLOT(latlongToGrid()));
  // Help menu
  aboutProgramAction=new QAction(this);
  //aboutProgramAction->setIcon(QIcon(":/.png"));
  aboutProgramAction->setText(tr("About SiteCheck"));
  helpMenu->addAction(aboutProgramAction);
  connect(aboutProgramAction,SIGNAL(triggered(bool)),this,SLOT(aboutProgram()));
  aboutQtAction=new QAction(this);
  //aboutQtAction->setIcon(QIcon(":/.png"));
  aboutQtAction->setText(tr("About Qt"));
  helpMenu->addAction(aboutQtAction);
  connect(aboutQtAction,SIGNAL(triggered(bool)),this,SLOT(aboutQt()));
  dumpAction=new QAction(this);
  //dumpAction->setIcon(QIcon(":/.png"));
  dumpAction->setText(tr("Dump")); // Dump is for debugging.
  //helpMenu->addAction(dumpAction); // In released versions, it is off the menu.
  connect(dumpAction,SIGNAL(triggered(bool)),canvas,SLOT(dump()));
  measureButtons.push_back(new MeasureButton(this,METER,0));
  measureButtons.back()->setIcon(QIcon(":/meter.png"));
  measureButtons.back()->setText(tr("Meter"));
  toolbar->addAction(measureButtons.back());
  unitsMenu->addAction(measureButtons.back());
  connect(measureButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(setMeter()));
  measureButtons.push_back(new MeasureButton(this,FOOT,0));
  measureButtons.back()->setIcon(QIcon(":/foot.png"));
  measureButtons.back()->setText(tr("Foot"));
  toolbar->addAction(measureButtons.back());
  unitsMenu->addAction(measureButtons.back());
  connect(measureButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(setFoot()));
  measureButtons.push_back(new MeasureButton(this,0,INTERNATIONAL));
  measureButtons.back()->setIcon(QIcon(":/international-foot.png"));
  measureButtons.back()->setText(tr("International foot"));
  toolbar->addAction(measureButtons.back());
  unitsMenu->addAction(measureButtons.back());
  connect(measureButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(setInternationalFoot()));
  measureButtons.push_back(new MeasureButton(this,0,USSURVEY));
  measureButtons.back()->setIcon(QIcon(":/us-foot.png"));
  measureButtons.back()->setText(tr("US survey foot"));
  toolbar->addAction(measureButtons.back());
  unitsMenu->addAction(measureButtons.back());
  connect(measureButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(setUSFoot()));
  measureButtons.push_back(new MeasureButton(this,0,INSURVEY));
  measureButtons.back()->setIcon(QIcon(":/indian-foot.png"));
  measureButtons.back()->setText(tr("Indian survey foot"));
  toolbar->addAction(measureButtons.back());
  unitsMenu->addAction(measureButtons.back());
  connect(measureButtons.back(),SIGNAL(triggered(bool)),canvas,SLOT(setIndianFoot()));
  for (i=0;i<measureButtons.size();i++)
    connect(canvas,SIGNAL(measureChanged(Measure)),measureButtons[i],SLOT(setMeasure(Measure)));
}

void SiteWindow::unmakeActions()
{
  int i;
  for (i=0;i<zoomButtons.size();i++)
  {
    toolbar->removeAction(zoomButtons[i]);
    viewMenu->removeAction(zoomButtons[i]);
    delete zoomButtons[i];
  }
  zoomButtons.clear();
  for (i=0;i<measureButtons.size();i++)
  {
    toolbar->removeAction(measureButtons[i]);
    unitsMenu->removeAction(measureButtons[i]);
    delete measureButtons[i];
  }
  measureButtons.clear();
}

void SiteWindow::showFileLoaded(string fileName)
{
  if (fileName.length())
    fileName=baseName(fileName)+" — ";
  setWindowTitle(QString::fromStdString(fileName)+tr("SiteCheck"));
}

void SiteWindow::prepareZoomSteps(int steps)
{
  cout<<"prepareZoomSteps "<<steps<<endl;
  preZoomStep=steps;
}

void SiteWindow::zoomSteps(bool checked)
{
  zoomCanvas(preZoomStep);
}

void SiteWindow::changeButtonBits()
{
  buttonBitsChanged((curvyTriangleAction->isChecked()<<0)|
                    (curvyContourAction->isChecked()<<1));
}

void SiteWindow::gridToLatlong()
{
  grDialog->setDoc(canvas->getDoc());
  grDialog->show();
  grDialog->raise();
  grDialog->activateWindow();
}

void SiteWindow::latlongToGrid()
{
  llDialog->setDoc(canvas->getDoc());
  llDialog->show();
  llDialog->raise();
  llDialog->activateWindow();
}

void SiteWindow::aboutProgram()
{
  QString progName=tr("SiteCheck, a Bezitopo program");
  QMessageBox::about(this,tr("SiteCheck"),
		     tr("%1\nVersion %2\nCopyright %3 Pierre Abbat\nLicense LGPL 3 or later")
		     .arg(progName).arg(QString(VERSION)).arg(COPY_YEAR));
}

void SiteWindow::aboutQt()
{
  QMessageBox::aboutQt(this,tr("SiteCheck"));
}
