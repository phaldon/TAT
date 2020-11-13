/**
 * \file timer.hpp
 *
 * Copyright (C) 2019-2020 Hao Zhang<zh970205@mail.ustc.edu.cn>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef TAT_TIMER_HPP
#define TAT_TIMER_HPP

#include <chrono>

namespace TAT {

   inline auto get_current_time() {
      return std::chrono::high_resolution_clock::now();
   }

   struct timer {
#ifndef TAT_USE_NO_TIMER
      std::string timer_name;
      std::chrono::high_resolution_clock::duration timer_count;

      timer(const char* name) : timer_name(name) {}

      ~timer() {
         if (timer_count.count() != 0) {
            auto count_in_second = std::chrono::duration<float>(timer_count).count();
            TAT_log((timer_name + " : " + std::to_string(count_in_second)).c_str());
         }
      }

      struct timer_guard {
         std::chrono::time_point<std::chrono::high_resolution_clock> tic;
         std::chrono::time_point<std::chrono::high_resolution_clock> toc;
         timer* owner;

         timer_guard(timer* owner) : owner(owner) {
            resume();
         }

         ~timer_guard() {
            pause();
         }

         void pause() {
            toc = get_current_time();
            owner->timer_count += toc - tic;
         }

         void resume() {
            tic = get_current_time();
         }
      };

      auto operator()() {
         return timer_guard(this);
      }
#else
      timer(const char*) {}

      struct timer_guard {
         void pause() {}
         void resume() {}
      };
      auto operator()() {
         return timer_guard();
      }
#endif
   };

#define TAT_DEFINE_TIMER(x) inline timer x##_guard(#x);
   TAT_DEFINE_TIMER(contract_misc)
   TAT_DEFINE_TIMER(contract_kernel)
   TAT_DEFINE_TIMER(svd_misc)
   TAT_DEFINE_TIMER(svd_kernel)
   TAT_DEFINE_TIMER(qr_misc)
   TAT_DEFINE_TIMER(qr_kernel)
   TAT_DEFINE_TIMER(scalar_outplace)
   TAT_DEFINE_TIMER(scalar_inplace)
   TAT_DEFINE_TIMER(transpose_misc)
   TAT_DEFINE_TIMER(transpose_kernel)
   TAT_DEFINE_TIMER(transpose_kernel_core)
#undef TAT_DEFINE_TIMER
} // namespace TAT
#endif