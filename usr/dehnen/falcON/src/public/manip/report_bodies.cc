// -*- C++ -*-                                                                  
////////////////////////////////////////////////////////////////////////////////
///                                                                             
/// \file   src/public/manip/report_bodies.cc                                   
///                                                                             
/// \brief  provided manipulator report_bodies                                  
///                                                                             
/// \author Walter Dehnen                                                       
/// \date   2007                                                                
///                                                                             
////////////////////////////////////////////////////////////////////////////////
//                                                                              
// Copyright (C) 2007 Walter Dehnen                                             
//                                                                              
// This program is free software; you can redistribute it and/or modify         
// it under the terms of the GNU General Public License as published by         
// the Free Software Foundation; either version 2 of the License, or (at        
// your option) any later version.                                              
//                                                                              
// This program is distributed in the hope that it will be useful, but          
// WITHOUT ANY WARRANTY; without even the implied warranty of                   
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
// General Public License for more details.                                     
//                                                                              
// You should have received a copy of the GNU General Public License            
// along with this program; if not, write to the Free Software                  
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                    
//                                                                              
////////////////////////////////////////////////////////////////////////////////
//                                                                              
// history:                                                                     
//                                                                              
// v 0.0    06/09/2006  WD created                                              
////////////////////////////////////////////////////////////////////////////////
#include <public/defman.h>
#include <fstream>                                 // C++ file I/O              
#include <cstring>                                 // C strings                 

namespace falcON { namespace Manipulate {
  // ///////////////////////////////////////////////////////////////////////////
  //                                                                            
  // class report_bodies                                                        
  //                                                                            
  /// manipulator: writes ASCII file(s): m,x,v,a,p[,R,U,C] of bodies in_subset()
  ///                                                                           
  /// This manipulator writes for each body in_subset() (default: all, see      
  /// set_subset) a separat file with mass, position, velocity, potential,      
  /// acceleration, and for SPH particles gas density, internal energy and      
  /// sound speed.\n                                                            
  /// The if more than one body is in_subset(), the file name provided MUST     
  /// in fact be a format string taking an integer, eg. "FILE%05d.dat".         
  /// \note                                                                     
  /// The number and order of bodies in_subset() must not change (later versions
  /// may overcome this problem).\n                                             
  ///                                                                           
  /// Meaning of the parameters:\n                                              
  /// file:   file name, format string if more than one body in_subset().       
  /// par[0]: # digits to print out real numbers (default: 8)                   
  ///                                                                           
  /// Usage of pointers: none\n                                                 
  /// Usage of flags:    uses in_subset()\n                                     
  ///                                                                           
  // ///////////////////////////////////////////////////////////////////////////
  class report_bodies : public manipulator {
  private:
    char                    FNAME[256];   ///< file name / format string.
    const int               PREC;         ///< precision for output of scalars
    mutable int             NSET;         ///< # files open
    mutable std::ofstream  *OUT;          ///< array: files to write to
    mutable unsigned       *KEY;          ///< array: keys of bodies in subset.
    //--------------------------------------------------------------------------
    static int N_set(const bodies*B) {
      int n(0);
      LoopAllBodies(B,b) if(in_subset(b)) ++n;
      return n;
    }
    static unsigned bodykey(const body&b) {
      return has_key(b)? key(b) : bodyindex(b);
    }
    static real ppp(const body&b) { return pot(b) + pex(b); }
    //--------------------------------------------------------------------------
    void print_scal(std::ostream&out) const
    {
      out << "-------";
      for(int i=0; i!=PREC; ++i) out << '-';
    }
    //--------------------------------------------------------------------------
    void print_vect(std::ostream&out) const
    {
      print_scal(out);
      print_scal(out);
      print_scal(out);
    }
    //--------------------------------------------------------------------------
    void print_line(std::ostream&out, const body&b) const
    {
      print_scal( out << '#' );
      if(has_mass(b)) print_scal(out);
      if(has_pos(b))  print_vect(out);
      if(has_vel(b))  print_vect(out);
      if(has_acc(b))  print_vect(out);
      if(has_pot(b) || has_pex(b)) print_scal(out);
      if(has_srho(b)) print_scal(out);
      if(has_uin (b)) print_scal(out);
      if(has_csnd(b)) print_scal(out);
      out<<std::endl;
    }
    //--------------------------------------------------------------------------
    void print_head(std::ostream&out, const body&b) const
    {
      out << "# " << std::setw(PREC+6) << "time";
      if(has_mass(b)) out << ' ' << std::setw(PREC+6) << "mass";
      if(has_pos (b)) out << ' ' << std::setw(PREC+6) << "x"
			  << ' ' << std::setw(PREC+6) << "y"
			  << ' ' << std::setw(PREC+6) << "z";
      if(has_vel (b)) out << ' ' << std::setw(PREC+6) << "v_x"
			  << ' ' << std::setw(PREC+6) << "v_y"
			  << ' ' << std::setw(PREC+6) << "v_z";
      if(has_acc (b)) out << ' ' << std::setw(PREC+6) << "a_x"
			  << ' ' << std::setw(PREC+6) << "a_y"
			  << ' ' << std::setw(PREC+6) << "a_z";
      if(has_pot(b) || has_pex(b)) out << ' ' << std::setw(PREC+6) << "Phi";
      if(has_srho(b)) out << ' ' << std::setw(PREC+6) << "gas_density";
      if(has_uin (b)) out << ' ' << std::setw(PREC+6) << "U_internal";
      if(has_csnd(b)) out << ' ' << std::setw(PREC+6) << "sound speed";
      out << std::endl;
    }
    //--------------------------------------------------------------------------
    void print_data(std::ostream&out, const body&b, double time) const
    {
      out << "  " << std::setw(PREC+6) << std::setprecision(PREC) << time;
      if(has_mass(b))
	out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << mass(b);
      if(has_pos(b))
	out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << pos(b)[0]
	    << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << pos(b)[1]
	    << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << pos(b)[2];
      if(has_vel(b))
	out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << vel(b)[0]
	    << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << vel(b)[1]
	    << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << vel(b)[2];
      if(has_acc(b))
	out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << acc(b)[0]
	    << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << acc(b)[1]
	    << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << acc(b)[2];
      if(has_pot(b))
	if(has_pex(b))
	  out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << ppp(b);
	else
	  out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << pot(b);
	else if(has_pex(b))
	  out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << pex(b);

      if(has_srho(b))
	out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << srho(b);
      if(has_uin (b))
	out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << uin(b);
      if(has_csnd(b))
	out << ' ' << std::setw(PREC+6) << std::setprecision(PREC) << csnd(b);
      out << std::endl;
    }
    //--------------------------------------------------------------------------
  public:
    const char* name    () const { return "report_bodies"; }
    const char* describe() const {
      return message("for selected bodies: writes basic data to file(s)");
    }
    //--------------------------------------------------------------------------
    fieldset need   () const { return fieldset::basic; }
    fieldset provide() const { return fieldset::empty; }
    fieldset change () const { return fieldset::empty; }
    //--------------------------------------------------------------------------
    bool manipulate(const snapshot*) const;
    //--------------------------------------------------------------------------
    report_bodies(const double*pars, int npar, const char*file) falcON_THROWING
    : PREC ( npar>0? int(pars[0]) : 8 ), NSET (0), OUT(0), KEY(0)
    {
      if(npar==0 && debug(1) || debug(2))
	std::cerr<<
	  " Manipulator \""<<name()<<"\":\n"
	  " writes basic data for file(s) for bodies in_subset()"
	  " (default: all)\n";
      strcpy(FNAME,file);
    }
    //--------------------------------------------------------------------------
    ~report_bodies();
  };
  //////////////////////////////////////////////////////////////////////////////
  report_bodies::~report_bodies() {
    if(OUT) falcON_DEL_A(OUT);
    if(KEY) falcON_DEL_A(KEY);
    NSET = 0;
    OUT = 0;
    KEY = 0;
  }
  //----------------------------------------------------------------------------
  bool report_bodies::manipulate(const snapshot*S) const
  {
    // 0 first call ever:
    if(OUT == 0) {
      NSET = N_set(S);
      OUT = falcON_NEW(std::ofstream,NSET);
      KEY = falcON_NEW(unsigned,NSET);
      char file[256];
      int f = 0;
      LoopAllBodies(S,b) if(in_subset(b)) {
	KEY[f] = bodykey(b);
	sprintf(file,FNAME,f);
	if(NSET > 1 && 0==strcmp(file,FNAME))
	  error("Manipulator \"%s\": %d bodies in set but "
		"file name \"%s\" not format string\n",
		name(),NSET,FNAME);
	OUT[f].open(file);
	if(!OUT[f].is_open())
	  error("Manipulator \"%s\": cannot open file \"%s\"\n",file);
 	print_line(OUT[f],b);
	OUT[f]   << "#\n# output from Manipulator \"" << name() << "\"\n";
	if(RunInfo::cmd_known())
	  OUT[f] << "#\n# command: \"" << RunInfo::cmd() << "\"\n";
	OUT[f]   << "#\n# run at "<< RunInfo::time() << '\n';
	if(RunInfo::user_known())
	  OUT[f] << "#     by \"" << RunInfo::user() << "\"\n";
	if(RunInfo::host_known())
	  OUT[f] << "#     on \"" << RunInfo::host() << "\"\n";
	if(RunInfo::pid_known())
	  OUT[f] << "#     pid " << RunInfo::pid() << '\n';
	OUT[f]   << "#\n# data for body " << KEY[f] << "\n#\n";
 	print_line(OUT[f],b);
	print_head(OUT[f],b);
	print_line(OUT[f],b);
	f++;
      }
    }
    // 1 every call: write out body data
    int f = N_set(S);
    if(NSET != f)
      error("Manipulator \"%s\": # bodies in set changed from %d to %d\n",
	    name(),NSET,f);
    f = 0;
    LoopAllBodies(S,b) if(in_subset(b)) {
      if(KEY[f] != bodykey(b))
	error("Manipulator \"%s\": %dth body changed key form %d to %d\n",
	      name(),KEY[f],bodykey(b));
      print_data(OUT[f],b,S->time());
      f++;
    }
    return false;
  }
  //////////////////////////////////////////////////////////////////////////////
} }

__DEF__MAN(falcON::Manipulate::report_bodies)