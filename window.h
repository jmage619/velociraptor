/****************************************************************************
    Copyright (C) 2019  jmage619

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#ifndef WINDOW
#define WINDOW

#include <atomic>
#include <mutex>
//#include <iostream>
#include <QWidget>

#include <jack/jack.h>

//int process(jack_nframes_t nframes, void* data);

//class QPushButton;
class QSlider;
class QLabel;
//class QSpinBox;

class Window: public QWidget {
  Q_OBJECT

  private:
    jack_client_t* jack_client;
    QLabel* note_label;
    QLabel* in_vel_label;
    QSlider* gain_slider;
    QLabel* gain_label;
    QSlider* thresh_slider;
    QLabel* thresh_label;

    QTimer* timer;

  public:
    //jack_port_t * out_l;
    //jack_port_t * out_r;
    jack_port_t* input_port;
    jack_port_t* output_port;
    std::atomic<int> note;
    std::atomic<int> in_vel;
    Window();
    ~Window();

  public slots:
    void updateDisplay();
    void updateGainLabel(int val);
    void updateThreshLabel(int val);
};

#endif
