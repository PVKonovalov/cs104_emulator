//
//  timer.hpp
//  cs104_emulator
//
//  Created by Mikael Persson https://stackoverflow.com/users/491645/mikael-persson
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

#ifndef timer_hpp
#define timer_hpp

#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

class Timer
{
public:
  Timer()
  :_execute(false)
  {}

  ~Timer() {
    if( _execute.load(std::memory_order_acquire) ) {
      stop();
    };
  }

  void stop()
  {
    _execute.store(false, std::memory_order_release);
    if( _thd.joinable() )
      _thd.join();
  }

  void start(int interval, std::function<void(void)> func)
  {
    if( _execute.load(std::memory_order_acquire) ) {
      stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this, interval, func]()
                       {
                         while (_execute.load(std::memory_order_acquire)) {
                           func();
                           std::this_thread::sleep_for(
                           std::chrono::milliseconds(interval));
                         }
                       });
  }

  bool is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) &&
            _thd.joinable() );
  }

private:
  std::atomic<bool> _execute;
  std::thread _thd;
};

#endif /* timer_hpp */
