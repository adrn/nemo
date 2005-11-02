// -*- C++ -*-                                                                 |
//-----------------------------------------------------------------------------+
//                                                                             |
// sample.h                                                                    |
//                                                                             |
// Copyright (C) 2004  Walter Dehnen                                           |
//                                                                             |
// This program is free software; you can redistribute it and/or modify        |
// it under the terms of the GNU General Public License as published by        |
// the Free Software Foundation; either version 2 of the License, or (at       |
// your option) any later version.                                             |
//                                                                             |
// This program is distributed in the hope that it will be useful, but         |
// WITHOUT ANY WARRANTY; without even the implied warranty of                  |
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           |
// General Public License for more details.                                    |
//                                                                             |
// You should have received a copy of the GNU General Public License           |
// along with this program; if not, write to the Free Software                 |
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                   |
//                                                                             |
//-----------------------------------------------------------------------------+
// some history                                                                |
// 27/10/2004 WD  added support for DF evaluation                              |
//-----------------------------------------------------------------------------+
#ifndef falcON_included_sample_h
#define falcON_included_sample_h

#ifndef falcON_included_body_h
#  include <body.h>
#endif
#ifndef falcON_included_rand_h
#  include <public/random.h>
#endif
////////////////////////////////////////////////////////////////////////////////
namespace falcON {
  //////////////////////////////////////////////////////////////////////////////
  //                                                                          //
  // class falcON::SphericalSampler                                           //
  //                                                                          //
  // PUBLIC version:                                                          //
  //                                                                          //
  // We consider isotropic DFs, i.e. f=f(E)                                   //
  //                                                                          //
  //                                                                          //
  // PROPRIETARY version:                                                     //
  //                                                                          //
  // We consider DFs of the general form (Eps := -E = Psi-v^2/2)              //
  //                                                                          //
  //     f = L^(-2b) g(Q)                                                     //
  //                                                                          //
  // with Q := Eps - L^2/(2a^2)                                               //
  //                                                                          //
  // The Binney anisotropy parameter beta for these models is                 //
  //                                                                          //
  //     beta = (r^2 + b*a^2) / (r^2 + a^2),                                  //
  //                                                                          //
  // which is beta=b at r=0 and beta=1 at r=oo.                               //
  // For b=0, we obtain the Ossipkov-Merritt model. For a=oo, we obtain a     //
  // model with constant anisotropy. For b=0 and a=oo, we get the isotropic   //
  // DF.                                                                      //
  //                                                                          //
  // NOTE  1. Since a=0 makes no sense, we interprete a=0 as a=oo.            //
  //       2. If the model has a different type of DF, one may not use these  //
  //          methods but superseed the sampling routines below.              //
  //                                                                          //
  //////////////////////////////////////////////////////////////////////////////
  class SphericalSampler {
  private:
    const double Mt;
    const double ibt;
    typedef tupel<2,double> pair_d;
    //--------------------------------------------------------------------------
#ifdef falcON_PROPER
    const bool   adapt_masses, Peri, OM, beta;
    const double ra,iraq;
    const double b0;
    const int    nR;
    const double*Rad, fac;
    double      *num;
    pair_d      *Xe;
    double      *Is;
    int          Ne;
    //--------------------------------------------------------------------------
    // private methods                                                          
    //--------------------------------------------------------------------------
    inline void setis();
#endif
  public:
    //--------------------------------------------------------------------------
    // construction & destruction                                               
    //--------------------------------------------------------------------------
    explicit 
    SphericalSampler(double const& mt              // I: total mass             
#ifdef falcON_PROPER
		    ,double const& =0.,            //[I: a: anisotropy radius]  
		     double const& =0.,            //[I: b0]                    
		     const double* =0,             //[I: mass adaption: radii]  
		     int    const& =0,             //[I: mass adaption: # -- ]  
		     double const& =1.2,           //[I: mass adaption: factor] 
		     bool   const& =0              //[I: mass adaption: R_-/Re] 
#endif
		     )
#ifdef falcON_PROPER
      ;
#else
      : 
    Mt(mt), ibt(1./3.) {}
#endif
    //--------------------------------------------------------------------------
    ~SphericalSampler() { 
#ifdef falcON_PROPER
      if(num) delete[] num;
      if(Xe)  delete[] Xe;
      if(Is)  delete[] Is;
#endif
    }
    //--------------------------------------------------------------------------
    // 1. Methods required for sampling radius and velocity: abstract           
    //--------------------------------------------------------------------------
  protected:
    virtual double DF(double)         const=0;     // f(E) or g(Q)              
#ifdef falcON_PROPER
    virtual double Re(double)         const=0;     // Rcirc(Eps)                
    virtual double Rp(double, double) const=0;     // R_peri(Eps, L)            
#endif
    virtual double Ps(double)         const=0;     // Psi(R)                    
    virtual double rM(double)         const=0;     // r(M), M in [0,M_total]    
    //--------------------------------------------------------------------------
    // 2. Sampling of phase space: virtual                                      
    //                                                                          
    // We implement two routines for sampling radius, radial and tangential     
    // velocity from the model. They differ in the treatment of the random      
    // numbers. The first accepts any type falcON::PseudoRandom random number   
    // generator and the second a falcON::Random, of which it uses the first    
    // two quasi-random number generators.                                      
    //                                                                          
    // NOTE  These methods are virtual but not abstract. That is you may super- 
    //       seed them if you want to provide your own.                         
    //--------------------------------------------------------------------------
    double pseudo_random(                          // R: Psi(r)                 
			 PseudoRandom const&,      // I: pseudo RNG             
			 double&,                  // O: radius                 
			 double&,                  // O: radial velocity        
			 double&,                  // O: tangential velocity    
			 double&) const;           // O: f(E) or g(Q)           
    //--------------------------------------------------------------------------
    double quasi_random (                          // R: Psi(r)                 
			 Random const&,            // I: pseudo & quasi RNG     
			 double&,                  // O: radius                 
			 double&,                  // O: radial velocity        
			 double&,                  // O: tangential velocity    
			 double&) const;           // O: f(E) or g(Q)           
    //--------------------------------------------------------------------------
    // 3. Sampling of full phase-space: non-virtual                             
    //                                                                          
    // NOTE on scales                                                           
    // We assume that the randomly generated (r,vr,vt) are already properly     
    // scaled as well as the Psi returnd. Also the routines for pericentre and  
    // Rc(E) are assummed to take arguments and return values that are both     
    // scaled.                                                                  
    //--------------------------------------------------------------------------
  public:
    void sample(bodies const&,                     // I/O: bodies to sample     
		bool   const&,                     // I: quasi random?          
		Random const&,                     // I: pseudo & quasi RNG     
		double const& =0.5,                //[I: fraction with vphi>0]  
#ifdef falcON_PROPER
	        double const& =0.0,                //[I: factor: setting eps_i] 
#endif
		bool          =false               //[I: write DF into aux?]    
		) const;          
    //--------------------------------------------------------------------------
  };
} // namespace falcON {
////////////////////////////////////////////////////////////////////////////////
#endif// falcON_included_sample_h
////////////////////////////////////////////////////////////////////////////////

