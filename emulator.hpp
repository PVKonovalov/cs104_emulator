//
//  emulator.hpp
//  cs104_emulator
//
//  Created by Pavel Konovalov on 15/11/2018.
//  Copyright © 2018 Pavel Konovalov. All rights reserved.
//
//
// License: BSD-3
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef emulator_hpp
#define emulator_hpp

#include <string>
#include "fuzzy_map_helper.hpp"

typedef unsigned int parameter_id;

typedef struct {
  parameter_id parameterId;

  time_t sec_from_midnight;
  unsigned int state;

  int   iVal;
  time_t dtMks;
  
  double fVal;
  double fApproximationVal;

  double min_fVal;
  double max_fVal;

  double fKoef;

} Parameter;

class Emulator {

private:
  std::map <parameter_id, fuzzy_map <time_t, Parameter*>> mapME;
  std::map <parameter_id, fuzzy_map <time_t, Parameter*>> mapSP;
  std::map <parameter_id, fuzzy_map <time_t, Parameter*>>::iterator iteratorME;
  std::map <parameter_id, fuzzy_map <time_t, Parameter*>>::iterator iteratorSP;
  std::map <parameter_id, fuzzy_map <time_t, Parameter*>>::iterator iteratorChangedME;
  std::map <parameter_id, fuzzy_map <time_t, Parameter*>>::iterator iteratorChangedSP;

public:
  void initFromCsv(std::string pathToDirectory);

  Parameter *getParameterSP(parameter_id parameterId, time_t sec_from_midnight);
  Parameter *getParameterME(parameter_id parameterId, time_t sec_from_midnight);
  Parameter *getApproximationParameterME(parameter_id parameterId, time_t sec_from_midnight);


  void moveIteratorBeginSP();
  void moveIteratorBeginME();

  Parameter *getNextParameterSP(time_t sec_from_midnight);
  Parameter *getNextParameterME(time_t sec_from_midnight);

  void moveIteratorChangedBeginSP();
  void moveIteratorChangedBeginME();

  Parameter *getNextChangedParameterSP(time_t sec_from_midnight);
  Parameter *getNextChangedParameterME(time_t sec_from_midnight);

  bool  parameterSPHasChanged(parameter_id parameterId, time_t sec_from_midnight);
  bool  parameterMEHasChanged(parameter_id parameterId, time_t sec_from_midnight, double aperture = 0.0001);

private:
  void parseCsvSP(std::string pathToDirectory, std::string fileName);
  void parseCsvME(std::string pathToDirectory, std::string fileName);

  parameter_id getParametrIdFromFileName(std::string fileName);
};


#endif /* emulator_hpp */
