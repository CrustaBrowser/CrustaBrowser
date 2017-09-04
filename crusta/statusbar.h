/* ============================================================
* Crusta - Qt5 webengine browser
* Copyright (C) 2017 Anmol Gautam <anmol@crustabrowser.com>
*
* THIS FILE IS A PART OF CRUSTA
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QWebEngineView>
#include <QLabel>

class StatusBar: public QWidget{
public:
    QLabel* zoomindicator=new QLabel();
    QWebEngineView* view;
    QPushButton* resetzoombtn=new QPushButton(tr("Reset"));
    QSlider* zoomslider=new QSlider();
    QProgressBar* pbar=new QProgressBar();
    QHBoxLayout* hbox=new QHBoxLayout();
    StatusBar();
    void getWebview(QWebEngineView* mview);
    void changeZoom(int);
};

#endif // STATUSBAR_H
