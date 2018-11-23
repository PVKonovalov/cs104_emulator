//
//  cs104_emulator.cpp
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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include "iec60870_slave.h"
#include "cs104_slave.h"

#include "hal_thread.h"
#include "hal_time.h"

#include "emulator.hpp"
#include "timer_helper.hpp"
#include "time_helper.hpp"
#include "log_helper.hpp"

#include <vector>

#define MAX_NUMBER_OF_INFORMATION_OBJECT_IN_ONE_ASDU 20

static bool running = true;

static CS101_AppLayerParameters appLayerParameters;
static CS104_Slave slave;

static Emulator emulator;
static Timer timer;


//  Created by Brad Folkens https://github.com/bfolkens
//  Copyright © 2018 Brad Folkens. All rights reserved.
static void prepare_for_daemon()
{
  pid_t pid;

  /* Fork off the parent process */
  pid = fork();

  /* An error occurred */
  if (pid < 0)
    exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */

  if (pid > 0)
    exit(EXIT_SUCCESS);

  /* On success: The child process becomes session leader */
  if (setsid() < 0)
    exit(EXIT_FAILURE);

  /* Catch, ignore and handle signals */
  //TODO: Implement a working signal handler */
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if (pid < 0)
    exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0)
    exit(EXIT_SUCCESS);

  /* Set new file permissions */
  umask(0);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  chdir("/");

  /* Close all open file descriptors */
  
  for (long x = sysconf(_SC_OPEN_MAX); x>=0; x--)
  {
    close ((int)x);
  }
}

void sigint_handler(int signalId)
{
  std::clog << "SIGINT has received: " << std::endl;

  timer.stop();
  running = false;
}

static bool interrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
{
  std::clog << "Received interrogation for group " << (unsigned) qoi << std::endl;

  if (qoi == 20) { /* only handle station interrogation */\

    time_t sec_from_midnight = secondsFromCurrentMidnight();

    int numberOfIoInOneAsdu = 1;

    IMasterConnection_sendACT_CON(connection, asdu, false);
    CS101_ASDU newAsdu = nullptr;

    std::vector < InformationObject > io;
    Parameter *informationParameter = nullptr;

    emulator.moveIteratorBeginSP();

    while((informationParameter = emulator.getNextParameterSP(sec_from_midnight)) != NULL) {

      if (numberOfIoInOneAsdu == 1) {
        newAsdu = CS101_ASDU_create(appLayerParameters, false, CS101_COT_INTERROGATED_BY_STATION, 0, 1, false, false);
      }

      io.push_back((InformationObject) SinglePointInformation_create(NULL, informationParameter->parameterId, informationParameter->iVal, IEC60870_QUALITY_GOOD));
      CS101_ASDU_addInformationObject(newAsdu,io.back());

      if (numberOfIoInOneAsdu == MAX_NUMBER_OF_INFORMATION_OBJECT_IN_ONE_ASDU) {

        numberOfIoInOneAsdu = 0;

        for(auto iobj : io) {
          InformationObject_destroy(iobj);
        }

        io.clear();

        if (newAsdu != nullptr) {
          IMasterConnection_sendASDU(connection, newAsdu);
          CS101_ASDU_destroy(newAsdu);
          newAsdu = nullptr;
        }
      }
      numberOfIoInOneAsdu += 1;
    }

    for(auto iobj : io) {
      InformationObject_destroy(iobj);
    }

    io.clear();

    if (newAsdu != nullptr) {
      IMasterConnection_sendASDU(connection, newAsdu);
      CS101_ASDU_destroy(newAsdu);
      newAsdu = nullptr;
    }

    numberOfIoInOneAsdu = 1;

    emulator.moveIteratorBeginME();

    while((informationParameter = emulator.getNextParameterME(sec_from_midnight)) != NULL) {

      time_t diff = abs(sec_from_midnight - informationParameter->sec_from_midnight);
      informationParameter->fApproximationVal = informationParameter->fVal + diff * informationParameter->fKoef;

      if (numberOfIoInOneAsdu == 1) {
        newAsdu = CS101_ASDU_create(appLayerParameters, false, CS101_COT_INTERROGATED_BY_STATION, 0, 1, false, false);
      }

      io.push_back((InformationObject) MeasuredValueShort_create(NULL, informationParameter->parameterId, informationParameter->fApproximationVal, IEC60870_QUALITY_GOOD));
      CS101_ASDU_addInformationObject(newAsdu,io.back());

      if (numberOfIoInOneAsdu == MAX_NUMBER_OF_INFORMATION_OBJECT_IN_ONE_ASDU) {

        numberOfIoInOneAsdu = 0;

        for(auto iobj : io) {
          InformationObject_destroy(iobj);
        }

        io.clear();

        if (newAsdu != nullptr) {
          IMasterConnection_sendASDU(connection, newAsdu);
          CS101_ASDU_destroy(newAsdu);
          newAsdu = nullptr;
        }
      }
      numberOfIoInOneAsdu += 1;
    }

    for(auto iobj : io) {
      InformationObject_destroy(iobj);
    }

    io.clear();

    if (newAsdu != nullptr) {
      IMasterConnection_sendASDU(connection, newAsdu);
      CS101_ASDU_destroy(newAsdu);
      newAsdu = nullptr;
    }

    IMasterConnection_sendACT_TERM(connection, asdu);

  }
  else {
    IMasterConnection_sendACT_CON(connection, asdu, true);
  }

  return true;
}

static bool asduHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu)
{
  if (CS101_ASDU_getTypeID(asdu) == C_SC_NA_1) {

    std::clog << "Received single command" << std::endl;

    if  (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION) {
      InformationObject io = CS101_ASDU_getElement(asdu, 0);

      if (InformationObject_getObjectAddress(io) == 5000) {
        SingleCommand sc = (SingleCommand) io;

        std::clog << "IOA: " << InformationObject_getObjectAddress(io) << " switch to " << SingleCommand_getState(sc) << std::endl;

        CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);
      }
      else
        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_IOA);

      InformationObject_destroy(io);
    }
    else
      CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);

    IMasterConnection_sendASDU(connection, asdu);

    return true;
  }

  return false;
}

static bool connectionRequestHandler(void* parameter, const char* ipAddress)
{
  std::clog << "New connection from " << ipAddress << std::endl;
  return true;
}

static void sendSpontaneousSinglePoint(CS104_Slave self) {

  int numberOfIoInOneAsdu = 1;

  CS101_ASDU newAsdu = nullptr;
  Parameter *informationParameter = nullptr;

  std::vector < InformationObject > io;

  time_t sec_from_midnight = secondsFromCurrentMidnight();
  struct sCP56Time2a currentTime;
  CP56Time2a_createFromMsTimestamp(&currentTime, Hal_getTimeInMs());

  emulator.moveIteratorChangedBeginSP();

  while((informationParameter = emulator.getNextChangedParameterSP(sec_from_midnight)) != NULL) {

    if (numberOfIoInOneAsdu == 1) {
      newAsdu = CS101_ASDU_create(appLayerParameters, false, CS101_COT_SPONTANEOUS, 0, 1, false, false);
    }

    io.push_back((InformationObject) SinglePointWithCP56Time2a_create(NULL, informationParameter->parameterId, informationParameter->iVal, IEC60870_QUALITY_GOOD, &currentTime));
    CS101_ASDU_addInformationObject(newAsdu, io.back());

    if (numberOfIoInOneAsdu == MAX_NUMBER_OF_INFORMATION_OBJECT_IN_ONE_ASDU) {

      numberOfIoInOneAsdu = 0;

      for(auto iobj : io) {
        InformationObject_destroy(iobj);
      }

      io.clear();

      if (newAsdu != nullptr) {
        CS104_Slave_enqueueASDU(self, newAsdu);
        CS101_ASDU_destroy(newAsdu);
        newAsdu = nullptr;
      }
    }
    numberOfIoInOneAsdu += 1;
  }

  for(auto iobj : io) {
    InformationObject_destroy(iobj);
  }

  io.clear();

  if (newAsdu != nullptr) {
    CS104_Slave_enqueueASDU(self, newAsdu);
    CS101_ASDU_destroy(newAsdu);
    newAsdu = nullptr;
  }
};

static void sendSpontaneousMeasuredValue(CS104_Slave self) {

  int numberOfIoInOneAsdu = 1;

  CS101_ASDU newAsdu = nullptr;
  Parameter *informationParameter = nullptr;

  std::vector < InformationObject > io;

  time_t sec_from_midnight = secondsFromCurrentMidnight();
  struct sCP56Time2a currentTime;
  CP56Time2a_createFromMsTimestamp(&currentTime, Hal_getTimeInMs());

  emulator.moveIteratorChangedBeginME();

  while((informationParameter = emulator.getNextChangedParameterME(sec_from_midnight)) != NULL) {
    
    if (numberOfIoInOneAsdu == 1) {
      newAsdu = CS101_ASDU_create(appLayerParameters, false, CS101_COT_SPONTANEOUS, 0, 1, false, false);
    }

    io.push_back((InformationObject) MeasuredValueShortWithCP56Time2a_create(NULL, informationParameter->parameterId, informationParameter->fApproximationVal, IEC60870_QUALITY_GOOD, &currentTime));
    CS101_ASDU_addInformationObject(newAsdu, io.back());

    if (numberOfIoInOneAsdu == MAX_NUMBER_OF_INFORMATION_OBJECT_IN_ONE_ASDU) {

        numberOfIoInOneAsdu = 0;

        for(auto iobj : io) {
          InformationObject_destroy(iobj);
        }

        io.clear();

        if (newAsdu != nullptr) {
          CS104_Slave_enqueueASDU(self, newAsdu);
          CS101_ASDU_destroy(newAsdu);
          newAsdu = nullptr;
        }
      }
    numberOfIoInOneAsdu += 1;
  }

  for(auto iobj : io) {
    InformationObject_destroy(iobj);
  }

  io.clear();

  if (newAsdu != nullptr) {
    CS104_Slave_enqueueASDU(self, newAsdu);
    CS101_ASDU_destroy(newAsdu);
    newAsdu = nullptr;
  }
};


void checkChangeValuesAndSendThey() {
  sendSpontaneousSinglePoint(slave);
  sendSpontaneousMeasuredValue(slave);
}

int main(int argc, char** argv)
{

  if(argc < 2) {
    std::clog << "cs104_emulator <path to config> [-p port] [-d]" << std::endl;
    exit(1);
  }

  std::string path_to_csv = argv[1];
  bool is_daemon = false;
  int port = 2404;
  bool next_param = false;

  for (int i = 2; i < argc; i++) {

    std::string param = argv[i];

    if(param == "-d") {
      is_daemon = true;
    }

    if(next_param && stoi(param) > 0) {
      port = stoi(param);
      next_param = false;
    }

    if(param == "-p") {
      next_param = true;
    }
  }

  if(is_daemon) {
    prepare_for_daemon();
    std::clog.rdbuf(new Log("cs104_emulator", LOG_LOCAL0));
  } else {
    signal(SIGINT, sigint_handler);
  }

  std::clog << "Load parameters from " << path_to_csv << std::endl;

  emulator.initFromCsv(path_to_csv);

  std::clog << "Trying to start with port " << port << std::endl;

  int openConnections = 0;

  slave = CS104_Slave_create(100, 100);

  CS104_Slave_setLocalPort(slave, port);
  CS104_Slave_setLocalAddress(slave, "0.0.0.0");

  appLayerParameters = CS104_Slave_getAppLayerParameters(slave);

  CS104_Slave_setInterrogationHandler(slave, interrogationHandler, NULL);

  CS104_Slave_setASDUHandler(slave, asduHandler, NULL);

  CS104_Slave_setConnectionRequestHandler(slave, connectionRequestHandler, NULL);

  CS104_Slave_setServerMode(slave, CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP);

  CS104_Slave_start(slave);

  if (CS104_Slave_isRunning(slave) == false) {
    std::clog << "Starting server failed!" << std::endl;
    CS104_Slave_destroy(slave);
    return 0;
  }

  timer.start(1000, checkChangeValuesAndSendThey);

  while (running) {
    int connectionsCount = CS104_Slave_getOpenConnections(slave);

    if (connectionsCount != openConnections) {
      openConnections = connectionsCount;
      std::clog << "Connected clients: " << openConnections << std::endl;
    }

    Thread_sleep(997);
  }

  CS104_Slave_stop(slave);
  CS104_Slave_destroy(slave);
}

