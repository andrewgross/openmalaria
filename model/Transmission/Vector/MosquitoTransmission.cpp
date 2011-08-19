/* This file is part of OpenMalaria.
 *
 * Copyright (C) 2005-2011 Swiss Tropical Institute and Liverpool School Of Tropical Medicine
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

//TODO: trim includes
#include "Transmission/Vector/MosquitoTransmission.h"
#include "schema/entomology.h"
#include "util/vectors.h"
#include "util/errors.h"
#include "util/CommandLine.h"
#include "util/MultidimSolver.h"
#include <cmath>
#include <sstream>

namespace OM {
namespace Transmission {
namespace Vector {
using namespace OM::util;

void MosquitoTransmission::initialise ( const scnXml::AnophelesParams& anoph ) {
    // -----  Set model variables  -----
    const scnXml::Mosq& mosq = anoph.getMosq();
    
    mosqRestDuration = mosq.getMosqRestDuration().getValue();
    EIPDuration = mosq.getExtrinsicIncubationPeriod().getValue();
    if (1 > mosqRestDuration || mosqRestDuration > EIPDuration) {
        throw util::xml_scenario_error ("Code expects EIPDuration >= mosqRestDuration >= 1");
    }
    N_v_length = EIPDuration + mosqRestDuration;
    
    minInfectedThreshold = mosq.getMinInfectedThreshold();
    
    lcParams.initMosqLifeCycle( anoph.getLifeCycle() );
    lifeCycle.init( lcParams );
    
    // -----  allocate memory  -----
    // Set up fArray and ftauArray. Each step, all elements not set here are
    // calculated, even if they aren't directly used in the end;
    // however all calculated values are used in calculating the next value.
    fArray.resize(EIPDuration-mosqRestDuration+1);
    fArray[0] = 1.0;
    ftauArray.resize(EIPDuration);
    for (int i = 0; i < mosqRestDuration; ++i)
        ftauArray[i] = 0.0;
    ftauArray[mosqRestDuration] = 1.0;

    N_v  .resize (N_v_length);
    O_v  .resize (N_v_length);
    S_v  .resize (N_v_length);
    P_A  .resize (N_v_length);
    P_df .resize (N_v_length);
    P_dif.resize (N_v_length);
}

void MosquitoTransmission::initState ( double tsP_A, double tsP_df,
                                       double initNvFromSv, double initOvFromSv,
                                       vector<double>& forcedS_v ){
    // Initialize per-day variables; S_v, N_v and O_v are only estimated
    for (int t = 0; t < N_v_length; ++t) {
        P_A[t] = tsP_A;
        P_df[t] = tsP_df;
        P_dif[t] = 0.0;     // humans start off with no infectiousness.. so just wait
        S_v[t] = forcedS_v[t];  // assume N_v_length ≤ 365
        N_v[t] = S_v[t] * initNvFromSv;
        O_v[t] = S_v[t] * initOvFromSv;
    }
}

double MosquitoTransmission::update( size_t d, double tsP_A, double tsP_df, double tsP_dif ){
    // Warning: with x<0, x%y can be negative (depending on compiler); avoid x<0.
    // We add N_v_length so that ((dMod - x) >= 0) for (x <= N_v_length).
    size_t dMod = d + N_v_length;
    assert (dMod >= (size_t)N_v_length);
    // Indecies for today, yesterday and mosqRestDuration days back:
    size_t t    = dMod % N_v_length;
    size_t t1   = (dMod - 1) % N_v_length;
    size_t ttau = (dMod - mosqRestDuration) % N_v_length;
    // Day of year and of 5-year cycles. Note that emergence during day 1
    // comes from mosqEmergeRate[0], hence subtraction by 1.
    size_t dYear1 = (d - 1) % TimeStep::DAYS_IN_YEAR;
    size_t d5Year = d % (TimeStep::DAYS_IN_YEAR * 5);
    
    
    // These only need to be calculated once per timestep, but should be
    // present in each of the previous N_v_length - 1 positions of arrays.
    P_A[t] = tsP_A;
    P_df[t] = tsP_df;
    P_dif[t] = tsP_dif;
    
    
    // update life-cycle model
    double newAdults = lifeCycle.updateEmergence( lcParams,
                                                    P_df[ttau] * N_v[ttau],
                                                    d, dYear1
                                                );
    
    
    // num seeking mosquitos is: new adults + those which didn't find a host
    // yesterday + those who found a host tau days ago and survived cycle:
    N_v[t] = newAdults
                + P_A[t1]  * N_v[t1]
                + P_df[ttau] * N_v[ttau];
    // similar for O_v, except new mosquitoes are those who were uninfected
    // tau days ago, started a feeding cycle then, survived and got infected:
    O_v[t] = P_dif[ttau] * (N_v[ttau] - O_v[ttau])
                + P_A[t1]  * O_v[t1]
                + P_df[ttau] * O_v[ttau];


    //BEGIN S_v
    // Set up array with n in 1..θ_s−1 for f_τ(dMod-n) (NDEMD eq. 1.7)
    size_t fProdEnd = 2*mosqRestDuration;
    for (size_t n = mosqRestDuration+1; n <= fProdEnd; ++n) {
        size_t tn = (dMod-n)%N_v_length;
        ftauArray[n] = ftauArray[n-1] * P_A[tn];
    }
    ftauArray[fProdEnd] += P_df[(dMod-fProdEnd)%N_v_length];

    for (int n = fProdEnd+1; n < EIPDuration; ++n) {
        size_t tn = (dMod-n)%N_v_length;
        ftauArray[n] =
            P_df[tn] * ftauArray[n - mosqRestDuration]
            + P_A[tn] * ftauArray[n-1];
    }

    double sum = 0.0;
    size_t ts = dMod - EIPDuration;
    for (int l = 1; l < mosqRestDuration; ++l) {
        size_t tsl = (ts - l) % N_v_length;       // index dMod - theta_s - l
        sum += P_dif[tsl] * P_df[ttau] * (N_v[tsl] - O_v[tsl]) * ftauArray[EIPDuration+l-mosqRestDuration];
    }


    // Set up array with n in 1..θ_s−τ for f(dMod-n) (NDEMD eq. 1.6)
    for (int n = 1; n <= mosqRestDuration; ++n) {
        size_t tn = (dMod-n)%N_v_length;
        fArray[n] = fArray[n-1] * P_A[tn];
    }
    fArray[mosqRestDuration] += P_df[ttau];

    fProdEnd = EIPDuration-mosqRestDuration;
    for (size_t n = mosqRestDuration+1; n <= fProdEnd; ++n) {
        size_t tn = (dMod-n)%N_v_length;
        fArray[n] =
            P_df[tn] * fArray[n - mosqRestDuration]
            + P_A[tn] * fArray[n-1];
    }


    ts = ts % N_v_length;       // index dMod - theta_s
    S_v[t] = P_dif[ts] * fArray[EIPDuration-mosqRestDuration] * (N_v[ts] - O_v[ts])
                + sum
                + P_A[t1]*S_v[t1]
                + P_df[ttau]*S_v[ttau];


    // We cut-off transmission when no more than X mosquitos are infected to
    // allow true elimination in simulations. Unfortunately, it may cause problems with
    // trying to simulate extremely low transmission, such as an R_0 case.
    if ( S_v[t] <= minInfectedThreshold ) { // infectious mosquito cut-off
        S_v[t] = 0.0;
        /* Note: could report; these reports often occur too frequently, however
        if( S_v[t] != 0.0 ){        // potentially reduce reporting
            cerr << TimeStep::simulation <<":\t S_v cut-off"<<endl;
        } */
    }
    //END S_v


    //FIXME quinquennialS_v[d5Year] = S_v[t];
    
    timestep_N_v0 += newAdults;

    return S_v[t];
}


// -----  Summary and intervention functions  -----

/*FIXME
void MosquitoTransmission::intervLarviciding (const scnXml::LarvicidingAnopheles& elt) {
    cerr << "This larviciding implementation isn't valid (according to NC)." << endl;
    larvicidingIneffectiveness = 1 - elt.getEffectiveness();
    larvicidingEndStep = TimeStep::simulation + TimeStep::fromDays(elt.getDuration());
}
*/
void MosquitoTransmission::uninfectVectors() {
    O_v.assign( O_v.size(), 0.0 );
    S_v.assign( S_v.size(), 0.0 );
    P_dif.assign( P_dif.size(), 0.0 );
}

double MosquitoTransmission::getLastVecStat ( VecStat vs ) const{
    //Note: implementation isn't performance optimal but rather intended to
    //keep code size low and have no overhead if not used.
    const vector<double> *array;
    switch( vs ){
        case PA: array = &P_A; break;
        case PDF: array = &P_df; break;
        case PDIF: array = &P_dif; break;
        case NV: array = &N_v; break;
        case OV: array = &O_v; break;
        case SV: array = &S_v; break;
        default: assert( false );
    }
    double val = 0.0;
    int firstDay = TimeStep::simulation.inDays() - TimeStep::interval + 1;
    for (size_t i = 0; i < (size_t)TimeStep::interval; ++i) {
        size_t t = (i + firstDay) % N_v_length;
        val += (*array)[t];
    }
    return val;
}
void MosquitoTransmission::summarize (const string speciesName, Monitoring::Survey& survey) const{
    survey.set_Vector_Nv0 (speciesName, timestep_N_v0/TimeStep::interval);
    survey.set_Vector_Nv (speciesName, getLastVecStat(NV)/TimeStep::interval);
    survey.set_Vector_Ov (speciesName, getLastVecStat(OV)/TimeStep::interval);
    survey.set_Vector_Sv (speciesName, getLastVecStat(SV)/TimeStep::interval);
}

}
}
}