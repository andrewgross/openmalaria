/* This file is part of OpenMalaria.
 * 
 * Copyright (C) 2005-2015 Swiss Tropical and Public Health Institute
 * Copyright (C) 2005-2015 Liverpool School Of Tropical Medicine
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

#ifndef Hmod_LSTMDrugType
#define Hmod_LSTMDrugType

#include "Global.h"
#include "util/sampler.h"

#include <string>
#include <deque>
#include <vector>
#include <set>
#include <cassert>
#include <memory>
#include <map>
#include <boost/unordered_set.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

using namespace std;

namespace scnXml{
    class Drugs;
    class PKPDDrug;
    class Phenotype;
}
namespace OM {
namespace PkPd {
using util::LognormalSampler;

class LSTMDrugType;
class LSTMDrug;

/** Drug PD parameters (specified per phenotype), as well as applicable
 * functions to calculate drug factors and concentrations.
 * 
 * Parasite genetics is specified elsewhere; we simply map genotype to
 * phenotype as well as describing phenotypes here. */
class LSTMDrugPD {
public:
    /// Construct
    LSTMDrugPD( const scnXml::Phenotype& phenotype );
    
    /** Calculate a survival factor induced by a drug already in the blood.
     * It is expected that no drug doses are taken over the period for which
     * this function calculates a drug factor.
     * 
     * @param neg_elim_rate -k (sampled)
     * @param C0 Concentration of drug in blood at start of period. Will be
     *  updated to correct concentration at end of period. Units: mg/l
     * @param duration Length of IV in days.
     * @return survival factor (unitless)
     */
    double calcFactor( double neg_elim_rate, double& C0, double duration ) const;
    
    inline double slope() const{ return n; }
    inline double IC50_pow_slope() const{ return Kn; }
    inline double max_killing_rate() const{ return V; }
    
private:
    struct Cache {
        Cache( double c, double d, double r );
        
        // hash, used as a way of organizing in a map
        size_t hash;
        // inputs:
        double C0, duration, rate;
        // cached outputs:
        double C1, drugFactor;
        
        // check inputs are equal (used to assert relation):
        bool operator== (const Cache& rhs) const{
            return C0 == rhs.C0 && duration == rhs.duration && rate == rhs.rate;
        }
    };
    struct Cache_hash : std::unary_function<Cache, std::size_t> {
        std::size_t operator()(Cache const& c) const {
            return c.hash;
        }
    };

    typedef boost::unordered_set<Cache,Cache_hash> CachedIV;
    mutable CachedIV cachedIV;
    
    /// Slope of the dose response curve (no unit)
    double n;   // slope
    /// Concentration with 50% of the maximal parasite killing to-the-power-of slope ((mg/l)^slope)
    double Kn;  // IC50_pow_slope
    /// Maximal drug killing rate per day (units: 1/days)
    double V;   // max_killing_rate;
};
    
    
/** Drug PK & PD parameters (one instance per drug type).
 * 
 * Static data contains a list of all available drug types.
 * 
 * None of this data is checkpointed, because it is loaded by init() from XML
 * data. (Although if it cannot be reproduced by reloading it should be
 * checkpointed.) */
class LSTMDrugType {
public:
    ///@brief Static functions
    //@{
    /** Initialise the drug model. Called at start of simulation. */
    static void init (const scnXml::Drugs& data);
    /** Clear previous data. Only needed for testing. */
    static void clear();
    
    /** Get the number of drug types. */
    static size_t numDrugTypes();
    
    /** Find a DrugType by its abbreviation, and returns its index.
     *
     * Throws if the drug isn't found, so you can rely on it returning a valid
     * index if it doesn't throw. */
    static size_t findDrug(string abbreviation);
    
    /** Create a per-human drug module for a given drug index. */
    static LSTMDrug* createInstance( size_t index );
    //@}
    
    
    ///@brief Non static (per instance) functions
    //@{
    /** Create a new DrugType.
     *
     * @param index     Index of drug in internal list
     * @param drugData Scenario data for this drug (PK params, PD params per phenotype)
     */
    LSTMDrugType (size_t index, const scnXml::PKPDDrug& drugData);
    ~LSTMDrugType ();
    
    inline size_t getIndex() const {
        return index;
    }
    inline double getNegligibleConcentration() const{
        return negligible_concentration;
    }
    inline double neg_m_exponent() const{ return neg_m_exp; }
    inline double sample_Vd() const{
        return vol_dist.sample();
    }
    inline double sample_elim_rate() const{
        return elimination_rate.sample();
    }
    inline double sample_a12() const{ return a12.sample(); }
    inline double sample_a21() const{ return a21.sample(); }
    inline double sample_a13() const{ return a13.sample(); }
    inline double sample_a31() const{ return a31.sample(); }
    inline double sample_ka() const{ return absorbtion_rate.sample(); }
    
    /** Return reference to correct drug-phenotype data. */
    const LSTMDrugPD& getPD( uint32_t genotype ) const;
    //@}
  
private:
    // non-copyable (due to allocation of members of drugPhenotype)
    LSTMDrugType( const LSTMDrugType& );
    
    /** The drug index. Stored here since LTSMDrug stores a pointer to this
     * struct object, not the index. */
    size_t index;
    
    /** A mapping from genotype codes to phenotypes for this drug. */
    vector<uint32_t> genotype_mapping;
    
    // TODO: at the moment we're storing all PK & PD data regardless of which
    // model is used. Evaluate whether this is sensible or not.
    
    /* PD parameters (may vary based on infection genotype). */
    boost::ptr_vector<LSTMDrugPD> PD;
    
    /*PK parameters required - varies with humans age and severity of disease*/
    /** Concentration, below which drug is deemed not to have an effect and is
     * removed for performance reasons. (mg/l) */
    double negligible_concentration;
    // Used to calculate elimination rate
    double neg_m_exp;
    /// Volume of distribution (l/kg)
    LognormalSampler vol_dist;
    /// Absorbtion rate
    LognormalSampler absorbtion_rate;
    /** Terminal elimination rate constant (k). Equals ln(2)/half_life.
     * Units: (1 / days) */
    LognormalSampler elimination_rate;
    /// Parameters for absorbtion rates in two- and three-compartment models.
    LognormalSampler a12, a21, a13, a31;
    
    // Allow LSTMDrug to access private members
    friend class LSTMDrugPD;
    friend inline double drugEffect (const LSTMDrugType& drugType, double& concentration, double duration, double weight_kg, double dose_mg);
};

} }
#endif