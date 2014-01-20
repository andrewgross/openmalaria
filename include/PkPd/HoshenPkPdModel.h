/* This file is part of OpenMalaria.
 * 
 * Copyright (C) 2005-2014 Swiss Tropical and Public Health Institute
 * Copyright (C) 2005-2014 Liverpool School Of Tropical Medicine
 * 
 * OpenMalaria is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef Hmod_HoshenPkPdModel
#define Hmod_HoshenPkPdModel

#include "PkPd/PkPdModel.h"
#include "PkPd/Drug/HoshenDrug.h"

/* WARNING: The Hoshen PK/PD model is no longer maintained.
 * The LSTM model is partially a replacement, and the Hoshen model has so
 * far had no use. */

namespace OM { namespace PkPd {
    
/** Pharmacokinetic and pharmacodynamics drug model, using the Hoshen
 * model.
 *
 * Holds per-human data for Tiago / the Liverpool school of medicine's
 * Hoshen PKPD model.
 * 
 * Some of the implementation is contained in the drug.h/drug.cpp files. */
class HoshenPkPdModel : public PkPdModel {
public:
  HoshenPkPdModel ();
  virtual ~HoshenPkPdModel ();
  
  virtual void checkpoint (istream& stream);
  virtual void checkpoint (ostream& stream);
  
  virtual void medicate(string drugAbbrev, double qty, double time, const AgeGroupData ageGroupData, double age);
  virtual void medicateIV(string drugAbbrev, double qty, double duration, double endTime);
  virtual void decayDrugs ();
  virtual double getDrugFactor (uint32_t proteome_ID);
  
  virtual uint32_t new_proteome_ID () {
      return ProteomeInstance::newInfection();
  }
  
private:
  list<HoshenDrug> _drugs;
};

} }
#endif