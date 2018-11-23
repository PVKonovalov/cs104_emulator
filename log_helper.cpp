//
//  log_helper.cpp
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

#include "log_helper.hpp"

Log::Log(std::string ident, int facility) {
  facility_ = facility;
  priority_ = LOG_DEBUG;
  strncpy(ident_, ident.c_str(), sizeof(ident_));
  ident_[sizeof(ident_)-1] = '\0';

  openlog(ident_, LOG_PID, facility_);
}

int Log::sync() {
  if (buffer_.length()) {
    syslog(priority_, "%s", buffer_.c_str());
    buffer_.erase();
    priority_ = LOG_DEBUG; // default to debug for each message
  }
  return 0;
}

int Log::overflow(int c) {
  if (c != EOF) {
    buffer_ += static_cast<char>(c);
  } else {
    sync();
  }
  return c;
}

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority) {
  static_cast<Log *>(os.rdbuf())->priority_ = (int)log_priority;
  return os;
}

