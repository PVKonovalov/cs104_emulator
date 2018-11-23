//
//  log_helper.hpp
//  cs104_emulator
//
// Created by eater https://stackoverflow.com/users/532530/eater
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

#ifndef log_helper_hpp
#define log_helper_hpp

#include <stdio.h>
#include <string>
#include <syslog.h>
#include <iostream>

enum LogPriority {
  kLogEmerg   = LOG_EMERG,   // system is unusable
  kLogAlert   = LOG_ALERT,   // action must be taken immediately
  kLogCrit    = LOG_CRIT,    // critical conditions
  kLogErr     = LOG_ERR,     // error conditions
  kLogWarning = LOG_WARNING, // warning conditions
  kLogNotice  = LOG_NOTICE,  // normal, but significant, condition
  kLogInfo    = LOG_INFO,    // informational message
  kLogDebug   = LOG_DEBUG    // debug-level message
};

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);

class Log : public std::basic_streambuf<char, std::char_traits<char> > {
public:
  explicit Log(std::string ident, int facility);

protected:
  int sync();
  int overflow(int c);

private:
  friend std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);
  std::string buffer_;
  int facility_;
  int priority_;
  char ident_[50];
};

#endif /* log_helper_hpp */
