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

#include "WithinHost/Infection/DescriptiveIPTInfection.h"
#include "util/random.h"
#include "util/errors.h"

namespace OM { namespace WithinHost {
     using namespace OM::util;
   // static IPT variables
vector<DescriptiveIPTInfection::GenotypeData> DescriptiveIPTInfection::genotypes;


// -----  static init/clear -----

// Only called if IPT is present
void DescriptiveIPTInfection::initParameters (const scnXml::IPTDescription& xmlIPTI){
  const scnXml::IPTDescription::InfGenotypeSequence& genotypesData = xmlIPTI.getInfGenotype();
  genotypes.reserve (genotypesData.size());
  
  double genotypeCumFreq = 0.0;
  for (scnXml::IPTDescription::InfGenotypeConstIterator it = genotypesData.begin(); it != genotypesData.end(); ++it) {
    genotypes.push_back( GenotypeData(it, genotypeCumFreq) );
  }
  assert( genotypes.size() == genotypesData.size() );
  // Note: this assertion is new; problem wouldn't have been detected
  // previously. Arguably frequencies should be scaled but the old
  // implementation didn't do this; don't change behaviour now.
  XML_ASSERT(0.99 <= genotypeCumFreq && genotypeCumFreq <= 1.01,
             "IPT.description.infGenotype.freq: sum across genotypes not equal to 1");
  genotypes[genotypes.size()-1].cumFreq = 1.0;	// make sure.. for random draws
  // (otherwise we rely on specification with XML and floating-point arithmatic)
}

DescriptiveIPTInfection::GenotypeData::GenotypeData (
    scnXml::IPTDescription::InfGenotypeConstIterator iter, double& cumFreq) :
    tolPeriod(TimeStep(iter->getTolPeriod())),
    proph(TimeStep(iter->getProph())),
    ACR(iter->getACR()), atten(iter->getAtten())
{
    cumFreq += iter->getFreq();
    this->cumFreq = cumFreq;
    XML_ASSERT(0.0 <= iter->getFreq() && iter->getFreq() <= 1.0, "IPT.description.infGenotype.freq: not in range [0,1]");
    XML_ASSERT(0.0 <= ACR && ACR <= 1.0, "IPT.description.infGenotype.ACR: not in range [0,1]");
    XML_ASSERT(1.0 <= atten, "IPT.description.infGenotype.atten: not in range [1,inf)");
}


// -----  non-static init/destruction  -----

DescriptiveIPTInfection::DescriptiveIPTInfection(TimeStep lastSPdose) :
  DescriptiveInfection(), _SPattenuate(false)
{
    // proteome_ID is initialized to 0xFFFFFFFF
    
    double rndSample=(random::uniform_01());
    double lowerBound = 0.0;
    //This Loop assigns the infection a genotype according to its frequency
    for (size_t genotypeCounter=0; genotypeCounter < genotypes.size(); genotypeCounter++){
      if (rndSample >= lowerBound && rndSample < genotypes[genotypeCounter].cumFreq){
	proteome_ID=genotypeCounter;
	break;
      }
      lowerBound = genotypes[genotypeCounter].cumFreq;
    }
    assert (proteome_ID < genotypes.size());
    /*
    The attenuation effect of SP is only effective during a certain time-window for certain IPTi models
    If t(=now) lies within this time window, SPattenuate is true, false otherwise.
    The time window starts after the prophylactic period ended (during the prophylactic
    period infections are cleared) and ends genotypeTolPeriod(iTemp%iData%gType%ID) time steps later.
    */
    if (TimeStep::simulation-lastSPdose > genotypes[proteome_ID].proph &&
	TimeStep::simulation-lastSPdose <= genotypes[proteome_ID].proph + genotypes[proteome_ID].tolPeriod){
      _SPattenuate=true;
    }
}

bool DescriptiveIPTInfection::eventSPClears (TimeStep _lastSPDose) {
    if(TimeStep::simulation - _startdate < latentp)
	return false;	// don't consider pre-patent infections
    
    return
	(TimeStep::simulation - _lastSPDose <= DescriptiveIPTInfection::genotypes[proteome_ID].proph)
	&& (random::uniform_01() <= DescriptiveIPTInfection::genotypes[proteome_ID].ACR);
}

double DescriptiveIPTInfection::asexualAttenuation () {
  double attFact = 1.0 / genotypes[proteome_ID].atten;
  _density *= attFact;
  return attFact;
}


DescriptiveIPTInfection::DescriptiveIPTInfection (istream& stream) :
    DescriptiveInfection(stream)
{
  _SPattenuate & stream; 
}
void DescriptiveIPTInfection::checkpoint (ostream& stream) {
    DescriptiveInfection::checkpoint (stream);
    _SPattenuate & stream; 
}

} }