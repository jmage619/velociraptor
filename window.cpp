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

#include <iostream>
using std::cout;
using std::endl;
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <QtWidgets>
#include "window.h"

#include <jack/jack.h>
#include <jack/midiport.h>

typedef jack_default_audio_sample_t sample_t;

int process(jack_nframes_t nframes, void* data) {
  Window* window = static_cast<Window*>(data);
  void* midi_in_buf = jack_port_get_buffer(window->input_port, nframes);
  void* midi_out_buf = jack_port_get_buffer(window->output_port, nframes);

  jack_midi_clear_buffer(midi_out_buf);

  jack_nframes_t event_count = jack_midi_get_event_count(midi_in_buf);
  jack_midi_event_t event;

  for (jack_nframes_t i = 0; i < event_count; ++i) {
    jack_midi_event_get(&event, midi_in_buf, i);

    if ((event.buffer[0] & 0xf0) == 0x90) {
      int in_vel = event.buffer[2];
      //cout << "note on; " << "note=" << (int) event.buffer[1] << "; vel=" << (int) event.buffer[2] << endl;
      float in_db = 20.f * log10f(event.buffer[2] / 127.f);
			float out_db;
      if (in_db > window->thresh)
         out_db = in_db - (1.f - 1.f / window->ratio) * (in_db - window->thresh);
			else
				out_db = in_db;

			out_db += window->gain;
			int out = roundf(127.f* powf(10.f, out_db / 20.f));
			event.buffer[2] = out <= 127 ? out: 127;

      window->note = event.buffer[1];
      window->in_vel = in_vel;
      window->out_vel = event.buffer[2];
    }
    jack_midi_event_write(midi_out_buf, event.time, event.buffer, event.size);
  }

  return 0;
}

Window::Window(): gain(0.f), thresh(0.f), ratio(1.f), note(0), in_vel(0), out_vel(0) {
  QVBoxLayout* v_layout = new QVBoxLayout;

  // note display
  QHBoxLayout* h_layout = new QHBoxLayout;

  QLabel* label = new QLabel;
  label->setText("note:");
  h_layout->addWidget(label);

  note_label = new QLabel;
  note_label->setMinimumWidth(30);
  note_label->setAlignment(Qt::AlignRight);
  h_layout->addWidget(note_label);

  label = new QLabel;
  label->setText("in_vel:");
  h_layout->addWidget(label);

  in_vel_label = new QLabel;
  in_vel_label->setMinimumWidth(30);
  in_vel_label->setAlignment(Qt::AlignRight);
  h_layout->addWidget(in_vel_label);

  label = new QLabel;
  label->setText("out_vel:");
  h_layout->addWidget(label);

  out_vel_label = new QLabel;
  out_vel_label->setMinimumWidth(30);
  out_vel_label->setAlignment(Qt::AlignRight);
  h_layout->addWidget(out_vel_label);

  h_layout->addStretch();

  v_layout->addLayout(h_layout);

  // gain slider
  h_layout = new QHBoxLayout;

  label = new QLabel;
  label->setText("gain:");
  label->setMinimumWidth(50);

  h_layout->addWidget(label);

  gain_slider = new QSlider(Qt::Horizontal);
  gain_slider->setMaximum(20);
  gain_slider->setMinimum(-20);
  h_layout->addWidget(gain_slider);

  gain_label = new QLabel;
  gain_label->setText("0 (db)");
  gain_label->setAlignment(Qt::AlignRight);
  gain_label->setMinimumWidth(60);
  h_layout->addWidget(gain_label);

  v_layout->addLayout(h_layout);

  // threshold slider
  h_layout = new QHBoxLayout;

  label = new QLabel;
  label->setText("thresh:");
  label->setMinimumWidth(50);

  h_layout->addWidget(label);

  thresh_slider = new QSlider(Qt::Horizontal);
  thresh_slider->setMaximum(0);
  thresh_slider->setMinimum(-40);
  thresh_slider->setValue(0);
  h_layout->addWidget(thresh_slider);

  thresh_label = new QLabel;
  thresh_label->setText("0 (db)");
  thresh_label->setAlignment(Qt::AlignRight);
  thresh_label->setMinimumWidth(60);
  h_layout->addWidget(thresh_label);

  v_layout->addLayout(h_layout);

  // ratio slider
  h_layout = new QHBoxLayout;

  label = new QLabel;
  label->setText("ratio:");
  label->setMinimumWidth(50);

  h_layout->addWidget(label);

  ratio_slider = new QSlider(Qt::Horizontal);
  ratio_slider->setMaximum(60);
  ratio_slider->setMinimum(1);
  ratio_slider->setValue(1);
  h_layout->addWidget(ratio_slider);

  ratio_label = new QLabel;
  ratio_label->setText("1:1");
  ratio_label->setAlignment(Qt::AlignRight);
  ratio_label->setMinimumWidth(60);
  h_layout->addWidget(ratio_label);

  v_layout->addLayout(h_layout);

  setLayout(v_layout);

  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &Window::updateDisplay);
  connect(gain_slider, &QSlider::valueChanged, this, &Window::updateGain);
  connect(thresh_slider, &QSlider::valueChanged, this, &Window::updateThresh);
  connect(ratio_slider, &QSlider::valueChanged, this, &Window::updateRatio);
  timer->start(20);

  // init jack garbage
  jack_client = jack_client_open("velociraptor", JackNullOption, nullptr);

  if (jack_client == nullptr)
    throw std::runtime_error("failed to initialize jack!");

  jack_set_process_callback(jack_client, &process, this);

  input_port = jack_port_register(jack_client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  if (input_port == nullptr) {
    jack_client_close(jack_client);
    throw std::runtime_error("failed to open jack input port!");
  }

  output_port = jack_port_register(jack_client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
  if (output_port == nullptr) {
    jack_port_unregister(jack_client, input_port);
    jack_client_close(jack_client);
    throw std::runtime_error("failed to open jack output port!");
  }

  // if all went well, activate it!
  if (jack_activate(jack_client)) {
    jack_port_unregister(jack_client, input_port);
    jack_port_unregister(jack_client, output_port);
    jack_client_close(jack_client);
    throw std::runtime_error("could not activate jack client!");
  }
}

Window::~Window() {
  jack_deactivate(jack_client);
  jack_port_unregister(jack_client, input_port);
  jack_port_unregister(jack_client, output_port);
  jack_client_close(jack_client);
}

void Window::updateDisplay() {
  note_label->setText(QString::number(note));
  in_vel_label->setText(QString::number(in_vel));
  out_vel_label->setText(QString::number(out_vel));
}

void Window::updateGain(int val) {
  gain = val;
  gain_label->setText(QString::number(val) + " (db)");
}

void Window::updateThresh(int val) {
  thresh = val;
  thresh_label->setText(QString::number(val) + " (db)");
}

void Window::updateRatio(int val) {
  ratio = val;
  ratio_label->setText(QString::number(val) + ":1");
}
