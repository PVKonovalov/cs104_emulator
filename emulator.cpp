//
//  emulator.cpp
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

#include "emulator.hpp"
#include "csv_helper.hpp"
#include <dirent.h>
#include <sys/types.h>

#include <string>
#include <iostream>
#include <set>
#include <cmath>

#include "fuzzy_map_helper.hpp"
#include "time_helper.hpp"

using namespace std;

/**
 Загружает конфигурацию из каталога с файлами csv

 @param pathToDirectory Путь на каталог
 */
void Emulator::initFromCsv(string pathToDirectory) {

  struct dirent *dp_parent;
  set<string> types_granted {"M_SP","M_ME"};

  DIR *dir_parent = opendir(pathToDirectory.c_str());

  if (dir_parent != NULL) {
    while ((dp_parent = readdir(dir_parent)) != NULL) {
      if (types_granted.find(dp_parent->d_name) != types_granted.end()) {
        DIR *dir_child = opendir(string(pathToDirectory + "/" + dp_parent->d_name).c_str());
        if (dir_child != NULL) {
          struct dirent *dp_child;
          while ((dp_child = readdir(dir_child)) != NULL) {
            size_t found = string(dp_child->d_name).find(".csv");
            if (found != string::npos) {
              if (string(dp_parent->d_name) == "M_ME") {
                std::clog << "Load " << dp_child->d_name << std::endl;
                parseCsvME(string(pathToDirectory + "/" + dp_parent->d_name + "/"), string(dp_child->d_name));
              }
              else {
                if (string(dp_parent->d_name) == "M_SP")
                {
                  std::clog << "Load " << dp_child->d_name << std::endl;
                  parseCsvSP(string(pathToDirectory + "/" + dp_parent->d_name + "/"), string(dp_child->d_name));
                }
              }
            }
          }
          closedir(dir_child);
        }
      }
    };

    closedir(dir_parent);
  }
}

/**
 Разбирает файл формата csv с архивным описанием значений параметра

 @param pathToDirectory Путь на каталог
 @param fileName Имя файла
 */
void Emulator::parseCsvME(string pathToDirectory, string fileName) {

  parameter_id parameterId = getParametrIdFromFileName(fileName);
  fuzzy_map <time_t, Parameter*> interval_map_obj{};

  if (parameterId != 0) {

    io::CSVReader<5, io::trim_chars<' '>, io::no_quote_escape<';'>> in(string(pathToDirectory + fileName));

    in.read_header(io::ignore_extra_column, "TIME1970", "VAL", "STATE", "MIN_VAL", "MAX_VAL");

    long time1970; double fVal; unsigned int state; double min_fVal; double max_fVal;

    bool theFirstRecord = true;
    tm *local_time = nullptr;

    double fPrev = 0;

    while(in.read_row(time1970,fVal,state,min_fVal,max_fVal)){

      local_time = std::localtime(&time1970);
      local_time->tm_mon = 0;
      local_time->tm_mday = 1;
      local_time->tm_year = 70;

      time_t sec_from_midnight = std::mktime(local_time) + local_time->tm_gmtoff;


      // Устанавливаем значение на начало суток равное второму значению, если значение на начало суток отсутствует
      if(theFirstRecord && sec_from_midnight != 0) {
        theFirstRecord = false;

        Parameter *parameter = new Parameter();
        parameter->sec_from_midnight = 0;
        parameter->fVal = fVal;
        parameter->state = state;
        parameter->min_fVal = min_fVal;
        parameter->max_fVal = max_fVal;
        parameter->parameterId = parameterId;
        parameter->fKoef = 1.0;

        interval_map_obj.insert( parameter->sec_from_midnight, parameter);

        fPrev = fVal;
      }

      Parameter *parameter = new Parameter();
      parameter->sec_from_midnight = sec_from_midnight;
      parameter->fVal = fVal;
      parameter->state = state;
      parameter->min_fVal = min_fVal;
      parameter->max_fVal = max_fVal;
      parameter->parameterId = parameterId;
      parameter->fKoef = (fVal - fPrev) / 600.0;

      interval_map_obj.insert( parameter->sec_from_midnight, parameter);

      
      fPrev = fVal;
    }

    mapME[parameterId] = interval_map_obj;

  }
}


/**
 Разбирает файл формата csv с архивным описанием значений параметра

 @param pathToDirectory Путь на каталог
 @param fileName Имя файла
 */
void Emulator::parseCsvSP(string pathToDirectory, string fileName) {

  parameter_id parameterId = getParametrIdFromFileName(fileName);
  fuzzy_map <time_t, Parameter*> interval_map_obj{};

  if (parameterId != 0) {

    io::CSVReader<4, io::trim_chars<' '>, io::no_quote_escape<';'>> in(string(pathToDirectory + fileName));

    in.read_header(io::ignore_extra_column, "TIME1970", "VAL", "STATE", "DT_MKS");

    long time1970; int iVal; unsigned int state; long dtMks;

    bool theFirstRecord = true;

    while(in.read_row(time1970,iVal,state,dtMks)){

      tm *local_time = std::localtime(&time1970);
      local_time->tm_mon = 0;
      local_time->tm_mday = 1;
      local_time->tm_year = 70;

      time_t sec_from_midnight = std::mktime(local_time) + local_time->tm_gmtoff;

      // Устанавливаем значение на начало суток равное второму значению, если значение на начало суток отсутствует
      if(theFirstRecord && sec_from_midnight != 0) {
        theFirstRecord = false;
        
        Parameter *parameter = new Parameter();
        parameter->sec_from_midnight = 0;
        parameter->iVal = iVal;
        parameter->state = state;
        parameter->dtMks = dtMks;
        parameter->parameterId = parameterId;

        interval_map_obj.insert( parameter->sec_from_midnight, parameter);
      }

      Parameter *parameter = new Parameter();
      parameter->sec_from_midnight = sec_from_midnight;
      parameter->iVal = iVal;
      parameter->state = state;
      parameter->dtMks = dtMks;
      parameter->parameterId = parameterId;

      interval_map_obj.insert( parameter->sec_from_midnight, parameter);
    }

    mapSP[parameterId] = interval_map_obj;
  }
}

/**
 Возвращает из имени файла формата XXX_NNNN.csv идентификатор параметра NNNN

 @param fileName Имя файлв
 @return Идентификатор параметра или 0 если не сформирован
 */
parameter_id Emulator::getParametrIdFromFileName(std::string fileName) {

  parameter_id parameterId = 0;

  size_t pos_begin = fileName.find("_");
  size_t pos_end = fileName.find(".csv");

  if(pos_begin != string::npos && pos_end != string::npos) {
    parameterId = (parameter_id) stoi(fileName.substr(pos_begin+1, pos_end - pos_begin - 1));
  }
  return parameterId;
}

Parameter *Emulator::getParameterME(parameter_id parameterId, time_t sec_from_midnight) {
  return mapME[parameterId][sec_from_midnight];
}

Parameter *Emulator::getApproximationParameterME(parameter_id parameterId, time_t sec_from_midnight) {

  Parameter *parameter = mapME[parameterId][sec_from_midnight];

  time_t diff = abs(sec_from_midnight - parameter->sec_from_midnight);

  parameter->fApproximationVal = parameter->fVal + diff * parameter->fKoef;

  return parameter;
}

Parameter *Emulator::getParameterSP(parameter_id parameterId, time_t sec_from_midnight) {
  return mapSP[parameterId][sec_from_midnight];
}

void Emulator::moveIteratorBeginME() {
  iteratorME = mapME.begin();
}

Parameter *Emulator::getNextParameterME(time_t sec_from_midnight) {

  if(iteratorME != mapME.end()) {
    Parameter *parameter = iteratorME->second[sec_from_midnight];
    iteratorME++;
    return parameter;
  }
  else {
    return NULL;
  }
}

void Emulator::moveIteratorBeginSP() {
  iteratorSP = mapSP.begin();
}

Parameter *Emulator::getNextParameterSP(time_t sec_from_midnight) {

  if(iteratorSP != mapSP.end()) {
    Parameter *parameter = iteratorSP->second[sec_from_midnight];
    iteratorSP++;
    return parameter;
  }
  else {
    return NULL;
  }
}

void Emulator::moveIteratorChangedBeginSP() {
  iteratorChangedSP = mapSP.begin();
}

void Emulator::moveIteratorChangedBeginME() {
  iteratorChangedME = mapME.begin();
}

Parameter *Emulator::getNextChangedParameterSP(time_t sec_from_midnight) {

  while(! (iteratorChangedSP == mapSP.end())) {

    Parameter *parameter = iteratorChangedSP->second[sec_from_midnight];

    iteratorChangedSP++;

    if( parameterSPHasChanged(parameter->parameterId, sec_from_midnight)) {
      return parameter;
    }


  }
  return NULL;
}

Parameter *Emulator::getNextChangedParameterME(time_t sec_from_midnight) {

  while(! (iteratorChangedME == mapME.end())) {

    Parameter *parameter = iteratorChangedME->second[sec_from_midnight];

    iteratorChangedME++;

    if( parameterMEHasChanged(parameter->parameterId, sec_from_midnight)) {
      return parameter;
    }


  }
  return NULL;
}


bool  Emulator::parameterMEHasChanged(parameter_id parameterId, time_t sec_from_midnight, double aperture) {
 
  if (sec_from_midnight == 0) {
    return true;
  }

  Parameter *parameter_current = getApproximationParameterME(parameterId, sec_from_midnight);
  Parameter *parameter_prev = getParameterME(parameterId, sec_from_midnight - 1 );

  if (abs((parameter_current->fApproximationVal - parameter_prev->fVal)) > aperture) {
    return true;
  }

  return false;
}

bool  Emulator::parameterSPHasChanged(parameter_id parameterId, time_t sec_from_midnight) {

  if (sec_from_midnight == 0) {
    return true;
  }

  Parameter *parameter_current = getParameterSP(parameterId, sec_from_midnight);
  Parameter *parameter_prev = getParameterSP(parameterId, sec_from_midnight - 1 );

  if (parameter_current->iVal != parameter_prev->iVal) {
    return true;
  }

  return false;
}
