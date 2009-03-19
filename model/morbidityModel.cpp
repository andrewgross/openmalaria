/*

This file is part of OpenMalaria.

Copyright (C) 2005,2006,2007,2008 Swiss Tropical Institute and Liverpool School Of Tropical Medicine

OpenMalaria is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#include "morbidityModel.h"
#include "pyrogenMorbidityModel.h"
#include "predetMorbidityModel.h"
#include "muellerMorbidityModel.h"
#include "global.h"

using namespace std;


MorbidityModel* MorbidityModel::createMorbidityModel(){
  if (Global::modelVersion & PREDETERMINED_EPISODES) {
    PredetMorbidityModel::init();
    return new PredetMorbidityModel();
  }
  else {
    if (Global::modelVersion & MUELLER_MORBIDITY_MODEL) {
      MuellerMorbidityModel::init();
      return new MuellerMorbidityModel();

    }
    else {
      PyrogenMorbidityModel::init();
      return new PyrogenMorbidityModel();
    }
  }
}

double MorbidityModel::getPyrogenThres(){
  return 0;
}

