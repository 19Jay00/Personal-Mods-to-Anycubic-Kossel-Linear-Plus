/************************
 * move_axis_screen.cpp *
 ************************/

/****************************************************************************
 *   Written By Mark Pelletier  2017 - Aleph Objects, Inc.                  *
 *   Written By Marcio Teixeira 2018 - Aleph Objects, Inc.                  *
 *                                                                          *
 *   This program is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation, either version 3 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   To view a copy of the GNU General Public License, go to the following  *
 *   location: <http://www.gnu.org/licenses/>.                              *
 ****************************************************************************/

#include "../config.h"

#if ENABLED(LULZBOT_TOUCH_UI)

#include "screens.h"
#include "screen_data.h"

using namespace FTDI;
using namespace ExtUI;

void MoveAxisScreen::onEntry() {
  // Since Marlin keeps only one absolute position for all the extruders,
  // we have to keep track of the relative motion of individual extruders
  // ourselves. The relative distances are reset to zero whenever this
  // screen is entered.

  for (uint8_t i = 0; i < ExtUI::extruderCount; i++) {
    screen_data.MoveAxisScreen.e_rel[i] = 0;
  }
  BaseNumericAdjustmentScreen::onEntry();
}

void MoveAxisScreen::onRedraw(draw_mode_t what) {
  widgets_t w(what);
  w.precision(1);
  w.units(GET_TEXTF(UNITS_MM));
  w.heading(                                GET_TEXTF(MOVE_AXIS));
  w.home_buttons(20);
  w.color(Theme::x_axis  )   .adjuster(  2, GET_TEXTF(AXIS_X),  getAxisPosition_mm(X), canMove(X));
  w.color(Theme::y_axis  )   .adjuster(  4, GET_TEXTF(AXIS_Y),  getAxisPosition_mm(Y), canMove(Y));
  w.color(Theme::z_axis  )   .adjuster(  6, GET_TEXTF(AXIS_Z),  getAxisPosition_mm(Z), canMove(Z));

  #if EXTRUDERS == 1
    w.color(Theme::e_axis)   .adjuster(  8, GET_TEXTF(AXIS_E),  screen_data.MoveAxisScreen.e_rel[0], canMove(E0));
  #elif EXTRUDERS > 1
    w.color(Theme::e_axis)   .adjuster(  8, GET_TEXTF(AXIS_E1), screen_data.MoveAxisScreen.e_rel[0], canMove(E0));
    w.color(Theme::e_axis)   .adjuster( 10, GET_TEXTF(AXIS_E2), screen_data.MoveAxisScreen.e_rel[1], canMove(E1));
    #if EXTRUDERS > 2
      w.color(Theme::e_axis) .adjuster( 12, GET_TEXTF(AXIS_E3), screen_data.MoveAxisScreen.e_rel[2], canMove(E2));
    #endif
    #if EXTRUDERS > 3
      w.color(Theme::e_axis) .adjuster( 14, GET_TEXTF(AXIS_E4), screen_data.MoveAxisScreen.e_rel[3], canMove(E3));
    #endif
  #endif
  w.increments();
}

bool MoveAxisScreen::onTouchHeld(uint8_t tag) {
  #define UI_INCREMENT_AXIS(axis) setManualFeedrate(axis, increment); UI_INCREMENT(AxisPosition_mm, axis);
  #define UI_DECREMENT_AXIS(axis) setManualFeedrate(axis, increment); UI_DECREMENT(AxisPosition_mm, axis);
  const float increment = getIncrement();
  switch (tag) {
    case  2: UI_DECREMENT_AXIS(X); break;
    case  3: UI_INCREMENT_AXIS(X); break;
    case  4: UI_DECREMENT_AXIS(Y); break;
    case  5: UI_INCREMENT_AXIS(Y); break;
    case  6: UI_DECREMENT_AXIS(Z); break;
    case  7: UI_INCREMENT_AXIS(Z); break;
    // For extruders, also update relative distances.
    case  8: UI_DECREMENT_AXIS(E0); screen_data.MoveAxisScreen.e_rel[0] -= increment; break;
    case  9: UI_INCREMENT_AXIS(E0); screen_data.MoveAxisScreen.e_rel[0] += increment; break;
    #if EXTRUDERS > 1
    case 10: UI_DECREMENT_AXIS(E1); screen_data.MoveAxisScreen.e_rel[1] -= increment; break;
    case 11: UI_INCREMENT_AXIS(E1); screen_data.MoveAxisScreen.e_rel[1] += increment; break;
    #endif
    #if EXTRUDERS > 2
    case 12: UI_DECREMENT_AXIS(E2); screen_data.MoveAxisScreen.e_rel[2] -= increment; break;
    case 13: UI_INCREMENT_AXIS(E2); screen_data.MoveAxisScreen.e_rel[2] += increment; break;
    #endif
    #if EXTRUDERS > 3
    case 14: UI_DECREMENT_AXIS(E3); screen_data.MoveAxisScreen.e_rel[3] -= increment; break;
    case 15: UI_INCREMENT_AXIS(E3); screen_data.MoveAxisScreen.e_rel[3] += increment; break;
    #endif
    case 20: injectCommands_P(PSTR("G28 X")); break;
    case 21: injectCommands_P(PSTR("G28 Y")); break;
    case 22: injectCommands_P(PSTR("G28 Z")); break;
    case 23: injectCommands_P(PSTR("G28"));   break;
    default:
      return false;
  }
  #undef UI_DECREMENT_AXIS
  #undef UI_INCREMENT_AXIS
  return true;
}

float MoveAxisScreen::getManualFeedrate(uint8_t axis, float increment_mm) {
  // Compute feedrate so that the tool lags the adjuster when it is
  // being held down, this allows enough margin for the planner to
  // connect segments and even out the motion.
  constexpr float max_manual_feedrate[XYZE] = MAX_MANUAL_FEEDRATE;
  return min(max_manual_feedrate[axis]/60, abs(increment_mm * TOUCH_REPEATS_PER_SECOND * 0.80));
  return min(max_manual_feedrate[axis] / 60, abs(increment_mm * TOUCH_REPEATS_PER_SECOND * 0.80));
}

void MoveAxisScreen::setManualFeedrate(ExtUI::axis_t axis, float increment_mm) {
  ExtUI::setFeedrate_mm_s(getManualFeedrate(X_AXIS + (axis - ExtUI::X), increment_mm));
}

void MoveAxisScreen::setManualFeedrate(ExtUI::extruder_t, float increment_mm) {
  ExtUI::setFeedrate_mm_s(getManualFeedrate(E_AXIS, increment_mm));
}

void MoveAxisScreen::onIdle() {
  if (refresh_timer.elapsed(STATUS_UPDATE_INTERVAL)) {
    onRefresh();
    refresh_timer.start();
  }
  BaseScreen::onIdle();
}

#endif // LULZBOT_TOUCH_UI
