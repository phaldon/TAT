/**
 * \file TAT.hpp
 *
 * Copyright (C) 2019  Hao Zhang<zh970205@mail.ustc.edu.cn>
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

#ifndef TAT_HPP_
#define TAT_HPP_

#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

// #include <mpi.h>

#ifdef NDEBUG
#   define TAT_WARNING(msg) std::clog
#else
#   define TAT_WARNING(msg) std::clog << msg << std::endl
#endif

namespace TAT {
   // TODO: Tensor 的一个meta_data方法， stream可以只用来格式化输出
   // 然后这个meta_data在二进制输出中和传输前pack时都可以用到
   // TODO: MPI 这个最后弄, 不然valgrind一大堆报错
   /*
   struct MPIWorld {
      int rank;
      int size;
      MPIWorld() {
         MPI_Init(0, 0);
         MPI_Comm_rank(MPI_COMM_WORLD, &rank);
         MPI_Comm_size(MPI_COMM_WORLD, &size);
      }
      ~MPIWorld() {
         MPI_Finalize();
      }
   };
   static MPIWorld mpi;

   struct MPIFile {
      MPI_File file_handle;
      MPIFile(const char* filename) {
         MPI_File_open(
               MPI_COMM_WORLD,
               filename,
               MPI_MODE_CREATE | MPI_MODE_RDWR,
               MPI_INFO_NULL,
               &file_handle);
      }
      ~MPIFile() {
         MPI_File_close(&file_handle);
      }
      void seek(MPI_Offset offset, int whence = MPI_SEEK_SET) {
         MPI_File_seek(file_handle, offset, whence);
      }
      void write(const void* buf, int size) {
         MPI_File_write(file_handle, buf, size, MPI_CHAR, MPI_STATUS_IGNORE);
      }
      void read(void* buf, int size) {
         MPI_File_read(file_handle, buf, size, MPI_CHAR, MPI_STATUS_IGNORE);
      }
   };
   */

   using Rank = unsigned int;
   using Nums = unsigned long;
   using Size = unsigned long long;
   using Z2 = bool;
   using U1 = long;
   using Fermi = int;

   template<class T>
   struct is_scalar : std::is_scalar<T> {};
   template<class T>
   struct is_scalar<std::complex<T>> : std::is_scalar<T> {};
   template<class T>
   static constexpr bool is_scalar_v = is_scalar<T>::value;

   template<class T>
   struct type_identity {
      using type = T;
   };
   template<class T>
   using type_identity_t = typename type_identity<T>::type;

   template<class T>
   struct real_base : type_identity<T> {};
   template<class T>
   struct real_base<std::complex<T>> : type_identity<T> {};
   template<class T>
   using real_base_t = typename real_base<T>::type;

   template<class T>
   struct remove_cvref : std::remove_cv<typename std::remove_reference<T>::type> {};
   template<class T>
   using remove_cvref_t = typename remove_cvref<T>::type;

   template<class T, class U>
   struct is_same_nocvref : std::is_same<remove_cvref_t<T>, remove_cvref_t<U>> {};
   template<class T, class U>
   static constexpr bool is_same_nocvref_v = is_same_nocvref<T, U>::value;

   using NameIdType = int;

   NameIdType names_total = 0;
   std::map<std::string, NameIdType> name_to_id = {};
   std::map<NameIdType, std::string> id_to_name = {};

   struct Name {
      NameIdType id = -1;
      Name() = default;
      Name(NameIdType id) : id{id} {}
      Name(const char* s) : Name(std::string(s)) {}
      Name(const std::string& name) {
         auto pos = name_to_id.find(name);
         if (pos == name_to_id.end()) {
            id = names_total++;
            name_to_id[name] = id;
            id_to_name[id] = name;
         } else {
            id = pos->second;
         }
      }
   };

   std::ostream& operator<<(std::ostream& out, const Name& name);
   std::istream& operator>>(std::istream& in, Name& name);

#define TAT_DEF_NAME_OP(OP, EXP)           \
   bool OP(const Name& a, const Name& b) { \
      return EXP;                          \
   }
   TAT_DEF_NAME_OP(operator==, a.id == b.id)
   TAT_DEF_NAME_OP(operator!=, a.id != b.id)
   TAT_DEF_NAME_OP(operator>=, a.id >= b.id)
   TAT_DEF_NAME_OP(operator<=, a.id <= b.id)
   TAT_DEF_NAME_OP(operator>, a.id> b.id)
   TAT_DEF_NAME_OP(operator<, a.id<b.id)
#undef TAT_DEF_NAME_OP

#define TAT_DEF_NAME(x) const Name x(#x)
#define TAT_DEF_NAMES(n)      \
   TAT_DEF_NAME(Phy##n);      \
   TAT_DEF_NAME(Left##n);     \
   TAT_DEF_NAME(Right##n);    \
   TAT_DEF_NAME(Up##n);       \
   TAT_DEF_NAME(Down##n);     \
   TAT_DEF_NAME(LeftUp##n);   \
   TAT_DEF_NAME(LeftDown##n); \
   TAT_DEF_NAME(RightUp##n);  \
   TAT_DEF_NAME(RightDown##n)
   TAT_DEF_NAMES();
   TAT_DEF_NAMES(1);
   TAT_DEF_NAMES(2);
   TAT_DEF_NAMES(3);
   TAT_DEF_NAMES(4);
#undef TAT_DEF_NAMES
   TAT_DEF_NAME(Contract1);
   TAT_DEF_NAME(Contract2);
#undef TAT_DEF_NAME

   template<class T>
   struct allocator_without_initialize : std::allocator<T> {
      template<class U>
      struct rebind {
         using other = allocator_without_initialize<U>;
      };

      template<class... Args>
      void construct([[maybe_unused]] T* p, Args&&... args) {
         if constexpr (!((sizeof...(args) == 0) && (std::is_trivially_destructible_v<T>))) {
            new (p) T(args...);
         }
      }

      allocator_without_initialize() = default;
      template<class U>
      allocator_without_initialize(allocator_without_initialize<U>) {}
   };

   template<class T>
   struct vector : public std::vector<T, allocator_without_initialize<T>> {
      using std::vector<T, allocator_without_initialize<T>>::vector;
   };

   template<class T>
   std::ostream& operator<<(std::ostream& out, const vector<T>& vec);
   template<class T>
   std::istream& operator>>(std::istream& in, vector<T>& vec);

   template<class Symmetry>
   struct Edge : public std::map<Symmetry, Size> {
      using std::map<Symmetry, Size>::map;

      Edge(Size s) : std::map<Symmetry, Size>({{Symmetry(), s}}) {}
   };

   template<class Symmetry>
   std::ostream& operator<<(std::ostream& out, const Edge<Symmetry>& edge);

   template<class Symmetry>
   std::istream& operator>>(std::istream& in, Edge<Symmetry>& edge);

   template<class Symmetry>
   struct EdgePosition {
      Symmetry sym;
      Size position;

      EdgePosition(Size p) : sym(Symmetry()), position(p) {}
      EdgePosition(Symmetry s, Size p) : sym(s), position(p) {}
   };

   template<class ScalarType, class Symmetry>
   struct Block {
      const vector<Edge<Symmetry>>& edges;
      vector<Symmetry> symmetries;
      vector<ScalarType> raw_data;
      Size size;

      template<
            class T = vector<Symmetry>,
            class = std::enable_if_t<is_same_nocvref_v<T, vector<Symmetry>>>>
      Block(const vector<Edge<Symmetry>>& e, T&& s) : edges(e), symmetries(std::forward<T>(s)) {
         size = 1;
         for (Rank i = 0; i < edges.size(); i++) {
            size *= edges[i].at(symmetries[i]);
         }
         raw_data = vector<ScalarType>(size);
      }
   };

   struct NoSymmetry {
      static bool get_parity(
            [[maybe_unused]] const vector<NoSymmetry>& syms,
            [[maybe_unused]] const vector<Rank>& plan) {
         return false;
      }
   };
   NoSymmetry
   operator+([[maybe_unused]] const NoSymmetry& s1, [[maybe_unused]] const NoSymmetry& s2) {
      return NoSymmetry();
   }
   NoSymmetry& operator+=(NoSymmetry& s1, [[maybe_unused]] const NoSymmetry& s2) {
      return s1;
   }

   std::ostream& operator<<(std::ostream& out, const NoSymmetry&);
   std::istream& operator>>(std::istream& in, NoSymmetry&);
#define TAT_DEF_SYM_OP(OP, EXP)                    \
   bool OP(const NoSymmetry&, const NoSymmetry&) { \
      return EXP;                                  \
   }
   TAT_DEF_SYM_OP(operator==, true)
   TAT_DEF_SYM_OP(operator!=, false)
   TAT_DEF_SYM_OP(operator>=, true)
   TAT_DEF_SYM_OP(operator<=, true)
   TAT_DEF_SYM_OP(operator>, false)
   TAT_DEF_SYM_OP(operator<, false)
#undef TAT_DEF_SYM_OP

   struct Z2Symmetry {
      Z2 z2 = 0;

      Z2Symmetry(Z2 z2 = 0) : z2(z2) {}

      static bool get_parity(
            [[maybe_unused]] const vector<Z2Symmetry>& syms,
            [[maybe_unused]] const vector<Rank>& plan) {
         return false;
      }
   };
   Z2Symmetry operator+(const Z2Symmetry& s1, const Z2Symmetry& s2) {
      return Z2Symmetry(s1.z2 ^ s2.z2);
   }
   Z2Symmetry& operator+=(Z2Symmetry& s1, const Z2Symmetry& s2) {
      s1.z2 ^= s2.z2;
      return s1;
   }

   std::ostream& operator<<(std::ostream& out, const Z2Symmetry& s);
   std::istream& operator>>(std::istream& in, Z2Symmetry& s);
#define TAT_DEF_SYM_OP(OP, EXP)                        \
   bool OP(const Z2Symmetry& a, const Z2Symmetry& b) { \
      return EXP;                                      \
   }
   TAT_DEF_SYM_OP(operator==, a.z2 == b.z2)
   TAT_DEF_SYM_OP(operator!=, a.z2 != b.z2)
   TAT_DEF_SYM_OP(operator>=, a.z2 >= b.z2)
   TAT_DEF_SYM_OP(operator<=, a.z2 <= b.z2)
   TAT_DEF_SYM_OP(operator>, a.z2> b.z2)
   TAT_DEF_SYM_OP(operator<, a.z2<b.z2)
#undef TAT_DEF_SYM_OP

   struct U1Symmetry {
      U1 u1 = 0;

      U1Symmetry(U1 u1 = 0) : u1(u1) {}

      static bool get_parity(
            [[maybe_unused]] const vector<U1Symmetry>& syms,
            [[maybe_unused]] const vector<Rank>& plan) {
         return false;
      }
   };
   U1Symmetry operator+(const U1Symmetry& s1, const U1Symmetry& s2) {
      return U1Symmetry(s1.u1 + s2.u1);
   }
   U1Symmetry& operator+=(U1Symmetry& s1, const U1Symmetry& s2) {
      s1.u1 += s2.u1;
      return s1;
   }

   std::ostream& operator<<(std::ostream& out, const U1Symmetry& s);
   std::istream& operator>>(std::istream& in, U1Symmetry& s);

#define TAT_DEF_SYM_OP(OP, EXP)                        \
   bool OP(const U1Symmetry& a, const U1Symmetry& b) { \
      return EXP;                                      \
   }
   TAT_DEF_SYM_OP(operator==, a.u1 == b.u1)
   TAT_DEF_SYM_OP(operator!=, a.u1 != b.u1)
   TAT_DEF_SYM_OP(operator>=, a.u1 >= b.u1)
   TAT_DEF_SYM_OP(operator<=, a.u1 <= b.u1)
   TAT_DEF_SYM_OP(operator>, a.u1> b.u1)
   TAT_DEF_SYM_OP(operator<, a.u1<b.u1)
#undef TAT_DEF_SYM_OP

   struct FermiSymmetry {
      Fermi fermi = 0;

      FermiSymmetry(Fermi fermi = 0) : fermi(fermi) {}

      static bool get_parity(const vector<FermiSymmetry>& syms, const vector<Rank>& plan) {
         auto rank = Rank(plan.size());
         bool res = false;
         for (Rank i = 0; i < rank; i++) {
            for (Rank j = i + 1; j < rank; j++) {
               if (plan[i] > plan[j]) {
                  res ^= (bool(syms[i].fermi % 2) && bool(syms[j].fermi % 2));
               }
            }
         }
         return res;
      }
   };
   FermiSymmetry operator+(const FermiSymmetry& s1, const FermiSymmetry& s2) {
      return FermiSymmetry(s1.fermi + s2.fermi);
   }
   FermiSymmetry& operator+=(FermiSymmetry& s1, const FermiSymmetry& s2) {
      s1.fermi += s2.fermi;
      return s1;
   }

   std::ostream& operator<<(std::ostream& out, const FermiSymmetry& s);
   std::istream& operator>>(std::istream& in, FermiSymmetry& s);

#define TAT_DEF_SYM_OP(OP, EXP)                              \
   bool OP(const FermiSymmetry& a, const FermiSymmetry& b) { \
      return EXP;                                            \
   }
   TAT_DEF_SYM_OP(operator==, a.fermi == b.fermi)
   TAT_DEF_SYM_OP(operator!=, a.fermi != b.fermi)
   TAT_DEF_SYM_OP(operator>=, a.fermi >= b.fermi)
   TAT_DEF_SYM_OP(operator<=, a.fermi <= b.fermi)
   TAT_DEF_SYM_OP(operator>, a.fermi> b.fermi)
   TAT_DEF_SYM_OP(operator<, a.fermi<b.fermi)
#undef TAT_DEF_SYM_OP

   struct FermiZ2Symmetry {
      Fermi fermi = 0;
      Z2 z2 = 0;
   };
   // TODO: FermiZ2Symmetry

   struct FermiU1Symmetry {
      Fermi fermi = 0;
      U1 u1 = 0;
   };
   // TODO: FermiU1Symmetry

   template<class Symmetry>
   bool check_symmetry_satisfied(const vector<Symmetry>& sym) {
      auto sum = Symmetry();
      for (const auto& i : sym) {
         sum += i;
      }
      return sum == Symmetry();
   }

   template<class Symmetry>
   auto initialize_block_symmetries_with_check(const vector<Edge<Symmetry>>& edges) {
      auto rank = edges.size();
      if (!rank) {
         return vector<vector<Symmetry>>{{}};
      }
      auto pos = vector<typename std::map<Symmetry, Size>::const_iterator>();
      auto vec = vector<Symmetry>();
      auto res = vector<vector<Symmetry>>();
      for (const auto& i : edges) {
         auto ptr = i.begin();
         if (ptr == i.end()) {
            return res;
         }
         pos.push_back(ptr);
         vec.push_back(ptr->first);
      }
      while (true) {
         if (check_symmetry_satisfied(vec)) {
            res.push_back(vec);
         }
         Rank ptr = 0;
         pos[ptr]++;
         while (pos[ptr] == edges[ptr].end()) {
            pos[ptr] = edges[ptr].begin();
            vec[ptr] = pos[ptr]->first;
            ptr++;
            if (ptr == rank) {
               return res;
            }
            pos[ptr]++;
         }
         vec[ptr] = pos[ptr]->first;
      }
   }

   std::map<Name, Rank> construct_name_to_index(const vector<Name>& names) {
      std::map<Name, Rank> res;
      auto rank = Rank(names.size());
      for (Rank i = 0; i < rank; i++) {
         res[names[i]] = i;
      }
      return res;
   }

   template<class ScalarType, bool parity>
   void matrix_transpose_kernel(
         Size M,
         Size N,
         const void* __restrict src,
         Size leading_src,
         void* __restrict dst,
         Size leading_dst,
         Size scalar_size) {
      Size line_size = scalar_size / sizeof(ScalarType);
      for (Size i = 0; i < M; i++) {
         for (Size j = 0; j < N; j++) {
            ScalarType* line_dst = (ScalarType*)((char*)dst + (j * leading_dst + i) * scalar_size);
            const ScalarType* line_src =
                  (ScalarType*)((char*)src + (i * leading_src + j) * scalar_size);
            for (Size k = 0; k < line_size; k++) {
               if constexpr (parity) {
                  line_dst[k] = -line_src[k];
               } else {
                  line_dst[k] = line_src[k];
               }
            }
            // TODO: 向量化
         }
      }
   }

   template<class ScalarType, bool parity, Size cache_size, Size... other>
   void matrix_transpose(
         Size M,
         Size N,
         const void* __restrict src,
         Size leading_src,
         void* __restrict dst,
         Size leading_dst,
         Size scalar_size) {
      Size block_size = 1;
      // TODO: 是否应该乘以二做冗余？
      while (block_size * block_size * scalar_size * 2 < cache_size) {
         block_size <<= 1;
      }
      block_size >>= 1;
      for (Size i = 0; i < M; i += block_size) {
         for (Size j = 0; j < N; j += block_size) {
            void* block_dst = (char*)dst + (j * leading_dst + i) * scalar_size;
            const void* block_src = (char*)src + (i * leading_src + j) * scalar_size;
            Size m = M - i;
            Size n = N - j;
            m = (block_size <= m) ? block_size : m;
            n = (block_size <= n) ? block_size : n;

            if constexpr (sizeof...(other) == 0) {
               matrix_transpose_kernel<ScalarType, parity>(
                     m, n, block_src, leading_src, block_dst, leading_dst, scalar_size);
            } else {
               matrix_transpose<ScalarType, parity, other...>(
                     m, n, block_src, leading_src, block_dst, leading_dst, scalar_size);
            }
         }
      }
   }

   static constexpr Size l1_cache = 32768;
   static constexpr Size l2_cache = 262144;
   static constexpr Size l3_cache = 9437184;
   // TODO: 如何确定系统cache

   template<class ScalarType, bool parity>
   void block_transpose(
         const void* __restrict src,
         void* __restrict dst,
         const vector<Rank>& plan_src_to_dst,
         const vector<Rank>& plan_dst_to_src,
         const vector<Size>& dims_src,
         const vector<Size>& dims_dst,
         [[maybe_unused]] const Size& size,
         const Rank& rank,
         const Size& scalar_size) {
      vector<Size> step_src(rank);
      step_src[rank - 1] = 1;
      for (Rank i = rank - 1; i > 0; i--) {
         step_src[i - 1] = step_src[i] * dims_src[i];
      }
      vector<Size> step_dst(rank);
      step_dst[rank - 1] = 1;
      for (Rank i = rank - 1; i > 0; i--) {
         step_dst[i - 1] = step_dst[i] * dims_dst[i];
      }

      vector<Size> index_list_src(rank, 0);
      vector<Size> index_list_dst(rank, 0);
      Size index_src = 0;
      Size index_dst = 0;

      Size dim_N = dims_src[rank - 1];
      Size dim_M = dims_dst[rank - 1];
      Rank pos_M = plan_dst_to_src[rank - 1];
      Rank pos_N = plan_src_to_dst[rank - 1];
      Size leading_M = step_src[pos_M];
      Size leading_N = step_dst[pos_N];

      while (1) {
         // TODO: l3太大了, 所以只按着l2和l1来划分
         matrix_transpose<ScalarType, parity, l2_cache, l1_cache>(
               dim_M,
               dim_N,
               (char*)src + index_src * scalar_size,
               leading_M,
               (char*)dst + index_dst * scalar_size,
               leading_N,
               scalar_size);

         Rank temp_rank_dst = rank - 2;
         Rank temp_rank_src = plan_dst_to_src[temp_rank_dst];
         if (temp_rank_src == rank - 1) {
            if (temp_rank_dst == 0) {
               return;
            }
            temp_rank_dst -= 1;
            temp_rank_src = plan_dst_to_src[temp_rank_dst];
         }

         index_list_src[temp_rank_src] += 1;
         index_src += step_src[temp_rank_src];
         index_list_dst[temp_rank_dst] += 1;
         index_dst += step_dst[temp_rank_dst];

         while (index_list_dst[temp_rank_dst] == dims_dst[temp_rank_dst]) {
            index_list_src[temp_rank_src] = 0;
            index_src -= dims_src[temp_rank_src] * step_src[temp_rank_src];
            index_list_dst[temp_rank_dst] = 0;
            index_dst -= dims_dst[temp_rank_dst] * step_dst[temp_rank_dst];
            if (temp_rank_dst == 0) {
               return;
            }
            temp_rank_dst -= 1;
            temp_rank_src = plan_dst_to_src[temp_rank_dst];
            if (temp_rank_src == rank - 1) {
               if (temp_rank_dst == 0) {
                  return;
               }
               temp_rank_dst -= 1;
               temp_rank_src = plan_dst_to_src[temp_rank_dst];
            }
            index_list_src[temp_rank_src] += 1;
            index_src += step_src[temp_rank_src];
            index_list_dst[temp_rank_dst] += 1;
            index_dst += step_dst[temp_rank_dst];
         }
      }
   }

   template<class Symmetry>
   bool check_same_symmetries(
         const vector<Symmetry>& s1,
         const vector<Symmetry>& s2,
         const vector<Rank>& plan) {
      auto rank = Rank(plan.size());
      for (Rank i = 0; i < rank; i++) {
         if (s1[i] != s2[plan[i]]) {
            return false;
         }
      }
      return true;
   }

   auto merge_in_transpose(
         const vector<Rank>& plan_src_to_dst,
         const vector<Rank>& plan_dst_to_src,
         const vector<Size>& dims_src,
         const vector<Size>& dims_dst,
         const Rank& rank) {
      vector<bool> merged_src_to_dst(rank);
      vector<bool> merged_dst_to_src(rank);
      merged_src_to_dst[0] = false;
      for (Rank i = 1; i < rank; i++) {
         if (plan_src_to_dst[i] == plan_src_to_dst[i - 1] + 1) {
            merged_src_to_dst[i] = true;
         } else {
            merged_src_to_dst[i] = false;
         }
      }
      merged_dst_to_src[0] = false;
      for (Rank i = 1; i < rank; i++) {
         if (plan_dst_to_src[i] == plan_dst_to_src[i - 1] + 1) {
            merged_dst_to_src[i] = true;
         } else {
            merged_dst_to_src[i] = false;
         }
      }
      vector<Rank> accum_src_to_dst(rank);
      vector<Rank> accum_dst_to_src(rank);
      accum_src_to_dst[0] = 0;
      for (Rank i = 1; i < rank; i++) {
         accum_src_to_dst[i] = accum_src_to_dst[i - 1] + Rank(merged_src_to_dst[i]);
      }
      accum_dst_to_src[0] = 0;
      for (Rank i = 1; i < rank; i++) {
         accum_dst_to_src[i] = accum_dst_to_src[i - 1] + Rank(merged_dst_to_src[i]);
      }
      vector<Rank> merged_plan_src_to_dst;
      vector<Rank> merged_plan_dst_to_src;
      for (Rank i = 0; i < rank; i++) {
         if (!merged_src_to_dst[i]) {
            merged_plan_src_to_dst.push_back(
                  plan_src_to_dst[i] - accum_dst_to_src[plan_src_to_dst[i]]);
         }
      }
      for (Rank i = 0; i < rank; i++) {
         if (!merged_dst_to_src[i]) {
            merged_plan_dst_to_src.push_back(
                  plan_dst_to_src[i] - accum_src_to_dst[plan_dst_to_src[i]]);
         }
      }
      auto merged_rank = Rank(merged_plan_src_to_dst.size());
      vector<Size> merged_dims_src(merged_rank);
      vector<Size> merged_dims_dst(merged_rank);
      Rank tmp_src_index = rank;
      for (Rank i = merged_rank; i-- > 0;) {
         merged_dims_src[i] = dims_src[--tmp_src_index];
         while (merged_src_to_dst[tmp_src_index]) {
            merged_dims_src[i] *= dims_src[--tmp_src_index];
         }
      }
      Rank tmp_dst_index = rank;
      for (Rank i = merged_rank; i-- > 0;) {
         merged_dims_dst[i] = dims_dst[--tmp_dst_index];
         while (merged_dst_to_src[tmp_dst_index]) {
            merged_dims_dst[i] *= dims_dst[--tmp_dst_index];
         }
      }
      return std::tuple{merged_plan_src_to_dst,
                        merged_plan_dst_to_src,
                        merged_dims_src,
                        merged_dims_dst,
                        merged_rank};
   }

   bool is_valid_name(const vector<Name>& names, const Rank& rank) {
      return names.size() == std::set<Name>(names.begin(), names.end()).size() &&
             names.size() == rank;
   }

   auto noone_in_transpose(
         const vector<Rank>& plan_src_to_dst,
         const vector<Rank>& plan_dst_to_src,
         const vector<Size>& dims_src,
         const vector<Size>& dims_dst,
         const Rank& rank) {
      vector<bool> isone_src(rank);
      vector<bool> isone_dst(rank);
      for (Rank i = 0; i < rank; i++) {
         isone_src[i] = dims_src[i] == 1;
      }
      for (Rank i = 0; i < rank; i++) {
         isone_dst[i] = dims_dst[i] == 1;
      }
      vector<Rank> accum_src(rank);
      vector<Rank> accum_dst(rank);
      accum_src[0] = isone_src[0];
      for (Rank i = 1; i < rank; i++) {
         accum_src[i] = accum_src[i - 1] + Rank(isone_src[i]);
      }
      accum_dst[0] = isone_dst[0];
      for (Rank i = 1; i < rank; i++) {
         accum_dst[i] = accum_dst[i - 1] + Rank(isone_dst[i]);
      }

      vector<Rank> noone_plan_src_to_dst;
      vector<Rank> noone_plan_dst_to_src;
      for (Rank i = 0; i < rank; i++) {
         if (!isone_src[i]) {
            noone_plan_src_to_dst.push_back(plan_src_to_dst[i] - accum_dst[plan_src_to_dst[i]]);
         }
      }
      for (Rank i = 0; i < rank; i++) {
         if (!isone_dst[i]) {
            noone_plan_dst_to_src.push_back(plan_dst_to_src[i] - accum_src[plan_dst_to_src[i]]);
         }
      }
      auto noone_rank = Rank(noone_plan_dst_to_src.size());

      vector<Size> noone_dims_src;
      vector<Size> noone_dims_dst;
      for (Rank i = 0; i < rank; i++) {
         if (dims_src[i] != 1) {
            noone_dims_src.push_back(dims_src[i]);
         }
      }
      for (Rank i = 0; i < rank; i++) {
         if (dims_dst[i] != 1) {
            noone_dims_dst.push_back(dims_dst[i]);
         }
      }
      return std::tuple{noone_plan_src_to_dst,
                        noone_plan_dst_to_src,
                        noone_dims_src,
                        noone_dims_dst,
                        noone_rank};
   }

   template<class ScalarType = double, class Symmetry = NoSymmetry>
   struct Tensor {
      struct TensorCore {
         vector<Edge<Symmetry>> edges;
         vector<Block<ScalarType, Symmetry>> blocks;

         template<
               class T = vector<Edge<Symmetry>>,
               class = std::enable_if_t<is_same_nocvref_v<T, vector<Edge<Symmetry>>>>>
         TensorCore(T&& e) : edges(std::forward<T>(e)) {
            auto symmetries_list = initialize_block_symmetries_with_check<Symmetry>(edges);
            for (auto& i : symmetries_list) {
               blocks.push_back(Block<ScalarType, Symmetry>(edges, std::move(i)));
            }
         }
      };

      vector<Name> names;
      std::map<Name, Rank> name_to_index;
      std::shared_ptr<TensorCore> core;

      template<
            class U = vector<Name>,
            class T = vector<Edge<Symmetry>>,
            class = std::enable_if_t<is_same_nocvref_v<U, vector<Name>>>,
            class = std::enable_if_t<is_same_nocvref_v<T, vector<Edge<Symmetry>>>>>
      Tensor(U&& n, T&& e) :
            names(std::forward<U>(n)), name_to_index(construct_name_to_index(names)),
            core(std::make_shared<TensorCore>(std::forward<T>(e))) {
         if (!is_valid_name(names, core->edges.size())) {
            TAT_WARNING("Invalid Names");
         }
      }

      Tensor() = default;
      Tensor(const Tensor& other) :
            names(other.names), name_to_index(other.name_to_index),
            core(std::make_shared<TensorCore>(*other.core)) {
         TAT_WARNING("Data Copy In Tensor Copy");
      }
      Tensor(Tensor&& other) = default;
      ~Tensor() = default;
      Tensor& operator=(const Tensor& other) {
         names = other.names;
         name_to_index = other.name_to_index;
         core = std::make_shared<TensorCore>(*other.core);
         TAT_WARNING("Data Copy In Tensor Copy");
      }
      Tensor& operator=(Tensor&& other) = default;

      Tensor(ScalarType num) : Tensor({}, {}) {
         core->blocks[0].raw_data[0] = num;
      }

      operator ScalarType() const {
         if (names.size() != 0) {
            TAT_WARNING("Conversion From Multiple Rank Tensor To Scalar");
         }
         return core->blocks[0].raw_data[0];
      }

      template<class Generator>
      Tensor<ScalarType, Symmetry>& set(Generator&& generator) & {
         if (core.use_count() != 1) {
            TAT_WARNING("Set Tensor Shared");
         }
         for (auto& i : core->blocks) {
            std::generate(i.raw_data.begin(), i.raw_data.begin() + i.size, generator);
         }
         return *this;
      }
      template<class Generator>
      Tensor<ScalarType, Symmetry> set(Generator&& generator) && {
         return std::move(set(generator));
      }

      Tensor<ScalarType, Symmetry>& zero() & {
         return set([]() { return 0; });
      }
      Tensor<ScalarType, Symmetry> zero() && {
         return std::move(zero());
      }

      std::tuple<Nums, Size>
      get_pos_for_at(const std::map<Name, EdgePosition<Symmetry>>& position) const {
         auto rank = Rank(names.size());
         vector<Symmetry> block_symmetries(rank);
         vector<Size> scalar_position(rank);
         vector<Size> dimensions(rank);
         for (const auto& [name, res] : position) {
            auto index = name_to_index.at(name);
            block_symmetries[index] = res.sym;
            scalar_position[index] = res.position;
            dimensions[index] = core->edges[index].at(res.sym);
         }
         Size offset = 0;
         for (Rank j = 0; j < rank; j++) {
            offset *= dimensions[j];
            offset += scalar_position[j];
         }
         for (Nums i = 0; i < core->blocks.size(); i++) {
            if (block_symmetries == core->blocks[i].symmetries) {
               return {i, offset};
            }
         }
         TAT_WARNING("Cannot Find Correct Block When Get Item");
         return {0, 0};
      }

      const ScalarType& at(const std::map<Name, EdgePosition<Symmetry>>& position) const& {
         auto [sym, pos] = get_pos_for_at(position);
         return core->blocks[sym].raw_data[pos];
      }

      ScalarType& at(const std::map<Name, EdgePosition<Symmetry>>& position) & {
         auto [sym, pos] = get_pos_for_at(position);
         return core->blocks[sym].raw_data[pos];
      }

      ScalarType at(const std::map<Name, EdgePosition<Symmetry>>& position) && {
         auto [sym, pos] = get_pos_for_at(position);
         return core->blocks[sym].raw_data[pos];
      }

      template<class OtherScalarType>
      Tensor<OtherScalarType, Symmetry> to() const {
         if constexpr (std::is_same_v<ScalarType, OtherScalarType>) {
            auto res = Tensor<ScalarType, Symmetry>{};
            res.names = names;
            res.name_to_index = name_to_index;
            res.core = core;
            return res;
         } else {
            auto res = Tensor<OtherScalarType, Symmetry>{};
            res.names = names;
            res.name_to_index = name_to_index;
            res.core = std::make_shared<typename Tensor<OtherScalarType, Symmetry>::TensorCore>(
                  core->edges);
            auto blocks_num = Nums(core->blocks.size());
            for (Nums i = 0; i < blocks_num; i++) {
               Size block_size = core->blocks[i].size;
               auto& dst = res.core->blocks[i].raw_data;
               const auto& src = core->blocks[i].raw_data;
               for (Size j = 0; j < block_size; j++) {
                  dst[j] = static_cast<OtherScalarType>(src[j]);
               }
            }
            return res;
         }
      }

      template<int p = 2>
      Tensor<real_base_t<ScalarType>, Symmetry> norm() const {
         real_base_t<ScalarType> res = 0;
         if constexpr (p == -1) {
            auto blocks_num = Nums(core->blocks.size());
            for (Nums i = 0; i < blocks_num; i++) {
               const auto& block = core->blocks[i];
               const auto& data = block.raw_data;
               for (Size j = 0; j < block.size; j++) {
                  auto tmp = std::abs(data[j]);
                  if (tmp > res) {
                     res = tmp;
                  }
               }
            }
         } else if constexpr (p == 0) {
            auto blocks_num = Nums(core->blocks.size());
            for (Nums i = 0; i < blocks_num; i++) {
               res += real_base_t<ScalarType>(core->blocks[i].size);
            }
         } else {
            auto blocks_num = Nums(core->blocks.size());
            for (Nums i = 0; i < blocks_num; i++) {
               const auto& block = core->blocks[i];
               const auto& data = block.raw_data;
               for (Size j = 0; j < block.size; j++) {
                  if constexpr (p == 1) {
                     res += std::abs(data[j]);
                  } else if constexpr (p == 2) {
                     if constexpr (std::is_same_v<ScalarType, real_base_t<ScalarType>>) {
                        auto tmp = data[j];
                        res += tmp * tmp;
                     } else {
                        auto tmp = std::abs(data[j]);
                        res += tmp * tmp;
                     }
                  } else {
                     if constexpr (
                           p % 2 == 0 && std::is_same_v<ScalarType, real_base_t<ScalarType>>) {
                        res += std::pow(data[j], p);
                     } else {
                        res += std::pow(std::abs(data[j]), p);
                     }
                  }
               }
            }
            return res = std::pow(res, 1. / p);
         }
         return res;
      }

      Tensor<ScalarType, Symmetry> edge_rename(const std::map<Name, Name>& dict) const {
         auto res = Tensor<ScalarType, Symmetry>{};
         res.core = core;
         std::transform(names.begin(), names.end(), std::back_inserter(res.names), [&](Name name) {
            auto pos = dict.find(name);
            if (pos == dict.end()) {
               return name;
            } else {
               return pos->second;
            }
         });
         res.name_to_index = construct_name_to_index(res.names);
         return res;
      }

      template<class T = vector<Name>, class = std::enable_if_t<is_same_nocvref_v<T, vector<Name>>>>
      Tensor<ScalarType, Symmetry> transpose(T&& target_names) const {
         auto res = Tensor<ScalarType, Symmetry>{};
         res.names = std::forward<T>(target_names);
         if (res.names == names) {
            res.name_to_index = name_to_index;
            res.core = core;
            return res;
         }
         res.name_to_index = construct_name_to_index(res.names);

         auto rank = Rank(names.size());
         vector<Rank> plan_src_to_dst(rank);
         vector<Rank> plan_dst_to_src(rank);
         for (Rank i = 0; i < rank; i++) {
            plan_src_to_dst[i] = res.name_to_index.at(names[i]);
         }
         for (Rank i = 0; i < rank; i++) {
            plan_dst_to_src[plan_src_to_dst[i]] = i;
         }

         vector<Edge<Symmetry>> res_edges(rank);
         for (Rank i = 0; i < rank; i++) {
            res_edges[plan_src_to_dst[i]] = core->edges[i];
         }
         res.core = std::make_shared<TensorCore>(std::move(res_edges));

         auto block_number = Nums(core->blocks.size());
         for (Nums index_src = 0; index_src < block_number; index_src++) {
            Nums index_dst = 0;
            while (!check_same_symmetries(
                  core->blocks[index_src].symmetries,
                  res.core->blocks[index_dst].symmetries,
                  plan_src_to_dst)) {
               index_dst++;
            }

            bool parity = Symmetry::get_parity(core->blocks[index_src].symmetries, plan_src_to_dst);

            Size block_size = core->blocks[index_src].size;
            if (block_size == 1) {
               if (parity) {
                  res.core->blocks[index_dst].raw_data[0] = -core->blocks[index_src].raw_data[0];
               } else {
                  res.core->blocks[index_dst].raw_data[0] = core->blocks[index_src].raw_data[0];
               }
            } else {
               vector<Size> dims_src(rank);
               vector<Size> dims_dst(rank);
               for (Rank i = 0; i < rank; i++) {
                  dims_src[i] = core->edges[i].at(core->blocks[index_src].symmetries[i]);
               }
               for (Rank i = 0; i < rank; i++) {
                  dims_dst[plan_src_to_dst[i]] = dims_src[i];
               }

               auto [noone_plan_src_to_dst,
                     noone_plan_dst_to_src,
                     noone_dims_src,
                     noone_dims_dst,
                     noone_rank] =
                     noone_in_transpose(plan_src_to_dst, plan_dst_to_src, dims_src, dims_dst, rank);

               auto [noone_merged_plan_src_to_dst,
                     noone_merged_plan_dst_to_src,
                     noone_merged_dims_src,
                     noone_merged_dims_dst,
                     noone_merged_rank] =
                     merge_in_transpose(
                           noone_plan_src_to_dst,
                           noone_plan_dst_to_src,
                           noone_dims_src,
                           noone_dims_dst,
                           noone_rank);

               if (noone_merged_rank == 1) {
                  ScalarType* dst = res.core->blocks[index_dst].raw_data.data();
                  const ScalarType* src = core->blocks[index_src].raw_data.data();
                  if (parity) {
                     for (Size k = 0; k < block_size; k++) {
                        dst[k] = -src[k];
                     }
                  } else {
                     for (Size k = 0; k < block_size; k++) {
                        dst[k] = src[k];
                     }
                  }
               } else {
                  Rank effective_rank = noone_merged_rank;
                  Size effective_size = sizeof(ScalarType);
                  if (noone_merged_plan_src_to_dst[noone_merged_rank - 1] ==
                      noone_merged_rank - 1) {
                     effective_rank--;
                     effective_size *= noone_merged_dims_dst[noone_merged_rank - 1];
                  }
                  // TODO: 需要考虑极端细致的情况
                  if (parity) {
                     block_transpose<ScalarType, true>(
                           core->blocks[index_src].raw_data.data(),
                           res.core->blocks[index_dst].raw_data.data(),
                           noone_merged_plan_src_to_dst,
                           noone_merged_plan_dst_to_src,
                           noone_merged_dims_src,
                           noone_merged_dims_dst,
                           block_size,
                           effective_rank,
                           effective_size);
                  } else {
                     block_transpose<ScalarType, false>(
                           core->blocks[index_src].raw_data.data(),
                           res.core->blocks[index_dst].raw_data.data(),
                           noone_merged_plan_src_to_dst,
                           noone_merged_plan_dst_to_src,
                           noone_merged_dims_src,
                           noone_merged_dims_dst,
                           block_size,
                           effective_rank,
                           effective_size);
                  }
               }
            }
         }
         return res;
      }

      bool exist_symmetry_in_blocks(
            const vector<typename std::map<Symmetry, Size>::const_iterator>& pos,
            const vector<Rank>& index) const {
         auto index_to_check = Rank(index.size());
         for (const auto& block : core->blocks) {
            bool res = true;
            for (Rank i = 0; i < index_to_check; i++) {
               if (pos[i]->first != block.symmetries[i]) {
                  res = false;
                  break;
               }
            }
            if (res) {
               return true;
            }
         }
         return false;
      }

      Edge<Symmetry> get_single_merged_edge(const vector<Name>& src) const {
         auto edges_num = Rank(src.size());
         auto res_edge = Edge<Symmetry>();
         if (!edges_num) {
            res_edge[Symmetry()] = 1;
            return res_edge;
         } else {
            auto edges_to_merge_index = vector<Rank>();
            auto edges_to_merge = vector<Edge<Symmetry>*>();
            for (const auto& s : src) {
               auto index = name_to_index.at(s);
               edges_to_merge_index.push_back(index);
               edges_to_merge.push_back(&core->edges[index]);
            }
            auto pos = vector<typename std::map<Symmetry, Size>::const_iterator>();
            for (const auto& i : edges_to_merge) {
               auto ptr = i->begin();
               if (ptr == i->end()) {
                  return res_edge;
               }
               pos.push_back(ptr);
            }
            auto sym = vector<Symmetry>(edges_num);
            auto dim = vector<Size>(edges_num);
            auto update_sym_and_dim = [&sym, &dim, &pos, &edges_num](Rank top) {
               for (Rank i = top; i-- > 0;) {
                  const auto& ptr = pos[i];
                  if (i == edges_num - 1) {
                     sym[i] = ptr->first;
                     dim[i] = ptr->second;
                  } else {
                     sym[i] = ptr->first + sym[i + 1];
                     dim[i] = ptr->second * dim[i + 1];
                     // do not check dim=0, because in constructor, i didn't check
                  }
               }
            };
            update_sym_and_dim(edges_num);
            while (true) {
               if (exist_symmetry_in_blocks(pos, edges_to_merge_index)) {
                  res_edge[sym[0]] += dim[0];
               }
               Rank ptr = 0;
               pos[ptr]++;
               while (pos[ptr] == edges_to_merge[ptr]->end()) {
                  pos[ptr] = edges_to_merge[ptr]->begin();
                  ptr++;
                  if (ptr == edges_num) {
                     return res_edge;
                  }
                  pos[ptr]++;
               }
               update_sym_and_dim(ptr + 1);
            }
         }
      }

      // TODO: merge and split
      // 原则上可以一边转置一边 merge split
      // merge 的时候会产生半个parity符号
      // 所以这个接口需要有个bool来确定是否处理符号
      // contract 调用的时候， 两个张量一个带符号一个不带即可

      /*Tensor<ScalarType, Symmetry>*/ void merge_edge(
            const vector<std::tuple<vector<TAT::Name>, TAT::Name>>& merge_pairs,
            const bool& apply_parity = false,
            const bool& side_preference = false) const {
         Rank rank = names.size();
         vector<Edge<Symmetry>> merged_res_edges;
         for (const auto& [src, dst] : merge_pairs) {
            merged_res_edges.push_back(get_single_merged_edge(src));
         }
         std::cout << merged_res_edges << "\n";
         vector<Name> target_names(rank);
         if (side_preference) {
            auto merged_number = Rank(merge_pairs.size());
            auto not_found = [](const vector<Name>& vec, const Name& n) {
               return std::find(vec.begin(), vec.end(), n) == vec.end();
            };
            if (merged_number == 1) {
               const auto& merge_list = std::get<0>(merge_pairs[0]);
               auto merge_list_length = Rank(merge_list.size());
               Rank i = 0;
               for (; i < merge_list_length; i++) {
                  target_names[i] = merge_list[i];
               }
               Rank src = 0;
               for (; i < rank; i++) {
                  auto current_name = names[src];
                  if (not_found(merge_list, current_name)) {
                     target_names[i] = current_name;
                  }
               }
            } else if (merged_number == 2) {
               const auto& merge_list_0 = std::get<0>(merge_pairs[0]);
               const auto& merge_list_1 = std::get<0>(merge_pairs[1]);
               auto merge_list_length_0 = Rank(merge_list_0.size());
               auto merge_list_length_1 = Rank(merge_list_1.size());
               Rank i = 0;
               for (; i < merge_list_length_0; i++) {
                  target_names[i] = merge_list_0[i];
               }
               Rank src = 0;
               Rank offset = rank - merge_list_length_1;
               for (; i < offset; i++) {
                  auto current_name = names[src];
                  if ((not_found(merge_list_0, current_name)) &&
                      (not_found(merge_list_1, current_name))) {
                     target_names[i] = current_name;
                  }
               }
               for (; i < rank; i++) {
                  target_names[i] = merge_list_0[i - offset];
               }
            } else {
               TAT_WARNING("Side Preference Set But Get More Than Two Pairs");
            }
         } else {
            auto merged_number = Rank(merge_pairs.size());
            Rank dst = rank - 1;
            for (Rank src = rank; src-- > 0;) {
               auto current_name = names[src];
               Rank found_in_to_merge = 0;
               Rank index_found;
               Rank merge_list_length;
               for (; found_in_to_merge < merged_number; found_in_to_merge++) {
                  const auto& merge_list = std::get<0>(merge_pairs[found_in_to_merge]);
                  merge_list_length = Rank(merge_list.size());
                  index_found = 0;
                  for (; index_found < merge_list_length; index_found++) {
                     if (current_name == merge_list[index_found]) {
                        goto end_of_loop;
                     }
                  }
               }
            end_of_loop:
               if (found_in_to_merge == merged_number) {
                  target_names[dst--] = current_name;
               } else if (index_found == merge_list_length - 1) {
                  const auto& merge_list = std::get<0>(merge_pairs[found_in_to_merge]);
                  for (Rank i = merge_list_length; i-- > 0;) {
                     target_names[dst--] = merge_list[i];
                  }
               }
            }
         }
         std::cout << target_names << "\n";
         auto tmp_tensor = transpose(std::move(target_names));
         // auto new_edges = core->edges;
         // core->
         // 确定Edge
         // 转置, 如果是side, 需要处理到边上
         // merge
      }

      struct split_pair {
         TAT::Name src;
         vector<TAT::Name> dst;
         vector<Edge<Symmetry>> new_edges;
      };

      Tensor<ScalarType, Symmetry>
      split_edge(const vector<split_pair>& split_pairs, const bool& apply_parity = false) const {
         // 无法确定Edge， 所以需要输入
         // split
      }

      // TODO: contract
      // 调用 merge ， 这样就不用考虑contract特有的符号问题了
      static Tensor<ScalarType, Symmetry> contract(
            const Tensor<ScalarType, Symmetry>& tensor1,
            const Tensor<ScalarType, Symmetry>& tensor2,
            const vector<Name>& names1,
            const vector<Name>& names2) {
         // TODO: 不转置成矩阵直接乘积的可能
         // names1 names2 需要 check order, 这个无法再merge中判断
         // merge 需要按照原样的顺序进行转置
         auto merged_tensor1 = tensor1.merge_edge({{names1, Contract1}}, false, true);
         auto merged_tensor2 = tensor2.merge_edge({{names2, Contract2}}, true, true);
         // check which one in 8 and do matrix contract
      }

      // TODO: converge
      // multiple 的扩展版
      // 考虑到PESS可能和对称性不兼容, 那么还是恢复原来的multiple模式
      // 因为这个东西唯一出现的地方就是svd了
      static Tensor<ScalarType, Symmetry> converge(
            const Tensor<ScalarType, Symmetry>& tensor1,
            const Tensor<ScalarType, Symmetry>& tensor2,
            const vector<Name>& names1,
            const vector<Name>& names2,
            const vector<Name>& names_res,
            const Size order) {
         // 这个东西在svd写好后再弄
      }

      struct svd_res {
         Tensor<ScalarType, Symmetry> U;
         Tensor<real_base_t<ScalarType>, Symmetry> S;
         Tensor<ScalarType, Symmetry> V;
      };

      // TODO: SVD
      // 根据情况选择转置方式
      // 或许可以称作cutted orthogonalize
      // 需要先merge再split, 不然不能保证分块依然分块
      svd_res svd(const vector<Name>& u_edges, Name u_new_name, Name v_new_name) const {
         // auto merged_tensor = merge_edge({{u_edges, SVD1}, {v_edges, SVD2}});
         // svd
         // auto split...
      }

      struct orthogonalize_res {
         Tensor<ScalarType, Symmetry> U;
         Tensor<ScalarType, Symmetry> T;
      };

      // TODO: QR LQ
      // 除了svd的考虑外， 需要考虑使用lq还是qr等
      orthogonalize_res orthogonalize(
            const vector<Name>& given_edges,
            Name given_new_name,
            Name other_new_name,
            bool u_edges_given = true) const {
         //
      }

      // TODO: 代码review
      // TODO: 更美观的IO
   };

   template<class ScalarType, class Symmetry>
   std::ostream& operator<<(std::ostream& out, const Tensor<ScalarType, Symmetry>& tensor);
   template<class ScalarType, class Symmetry>
   std::istream& operator>>(std::istream& in, Tensor<ScalarType, Symmetry>& tensor);

#define DEF_SCALAR_OP(OP, EVAL1, EVAL2, EVAL3)                                                 \
   template<class ScalarType1, class ScalarType2, class Symmetry>                              \
   auto OP(const Tensor<ScalarType1, Symmetry>& t1, const Tensor<ScalarType2, Symmetry>& t2) { \
      using ScalarType = std::common_type_t<ScalarType1, ScalarType2>;                         \
      if (t1.names.size() == 0) {                                                              \
         const auto& x = t1.core->blocks[0].raw_data[0];                                       \
         auto res = Tensor<ScalarType, Symmetry>{t2.names, t2.core->edges};                    \
         auto blocks_number = Nums(res.core->blocks.size());                                   \
         for (Nums i = 0; i < blocks_number; i++) {                                            \
            const ScalarType2* __restrict b = t2.core->blocks[i].raw_data.data();              \
            ScalarType* __restrict c = res.core->blocks[i].raw_data.data();                    \
            Size block_size = res.core->blocks[i].size;                                        \
            for (Size j = 0; j < block_size; j++) {                                            \
               EVAL1;                                                                          \
            }                                                                                  \
         }                                                                                     \
         return res;                                                                           \
      } else if (t2.names.size() == 0) {                                                       \
         const auto& y = t2.core->blocks[0].raw_data[0];                                       \
         auto res = Tensor<ScalarType, Symmetry>{t1.names, t1.core->edges};                    \
         auto blocks_number = Nums(res.core->blocks.size());                                   \
         for (Nums i = 0; i < blocks_number; i++) {                                            \
            const ScalarType1* __restrict a = t1.core->blocks[i].raw_data.data();              \
            ScalarType* __restrict c = res.core->blocks[i].raw_data.data();                    \
            Size block_size = res.core->blocks[i].size;                                        \
            for (Size j = 0; j < block_size; j++) {                                            \
               EVAL2;                                                                          \
            }                                                                                  \
         }                                                                                     \
         return res;                                                                           \
      } else {                                                                                 \
         if (!((t1.names == t2.names) && (t1.core->edges == t2.core->edges))) {                \
            TAT_WARNING("Scalar Operator In Different Shape Tensor");                          \
         }                                                                                     \
         auto res = Tensor<ScalarType, Symmetry>{t1.names, t1.core->edges};                    \
         auto blocks_number = Nums(res.core->blocks.size());                                   \
         for (Nums i = 0; i < blocks_number; i++) {                                            \
            const ScalarType1* __restrict a = t1.core->blocks[i].raw_data.data();              \
            const ScalarType2* __restrict b = t2.core->blocks[i].raw_data.data();              \
            ScalarType* __restrict c = res.core->blocks[i].raw_data.data();                    \
            Size block_size = res.core->blocks[i].size;                                        \
            for (Size j = 0; j < block_size; j++) {                                            \
               EVAL3;                                                                          \
            }                                                                                  \
         }                                                                                     \
         return res;                                                                           \
      }                                                                                        \
   }                                                                                           \
   template<                                                                                   \
         class ScalarType1,                                                                    \
         class ScalarType2,                                                                    \
         class Symmetry,                                                                       \
         class = std::enable_if_t<is_scalar_v<ScalarType2>>>                                   \
   auto OP(const Tensor<ScalarType1, Symmetry>& t1, const ScalarType2& n2) {                   \
      return OP(t1, Tensor<ScalarType2, Symmetry>{n2});                                        \
   }                                                                                           \
   template<                                                                                   \
         class ScalarType1,                                                                    \
         class ScalarType2,                                                                    \
         class Symmetry,                                                                       \
         class = std::enable_if_t<is_scalar_v<ScalarType1>>>                                   \
   auto OP(const ScalarType1& n1, const Tensor<ScalarType2, Symmetry>& t2) {                   \
      return OP(Tensor<ScalarType1, Symmetry>{n1}, t2);                                        \
   }

   DEF_SCALAR_OP(operator+, c[j] = x + b[j], c[j] = a[j] + y, c[j] = a[j] + b[j])
   DEF_SCALAR_OP(operator-, c[j] = x - b[j], c[j] = a[j] - y, c[j] = a[j] - b[j])
   DEF_SCALAR_OP(operator*, c[j] = x* b[j], c[j] = a[j]* y, c[j] = a[j]* b[j])
   DEF_SCALAR_OP(operator/, c[j] = x / b[j], c[j] = a[j] / y, c[j] = a[j] / b[j])
#undef DEF_SCALAR_OP

#define DEF_SCALAR_OP(OP, EVAL1, EVAL2)                                                          \
   template<class ScalarType1, class ScalarType2, class Symmetry>                                \
   Tensor<ScalarType1, Symmetry>& OP(                                                            \
         Tensor<ScalarType1, Symmetry>& t1, const Tensor<ScalarType2, Symmetry>& t2) {           \
      if (t1.core.use_count() != 1) {                                                            \
         TAT_WARNING("Inplace Operator On Tensor Shared");                                       \
      }                                                                                          \
      if (t2.names.size() == 0) {                                                                \
         const auto& y = t2.core->blocks[0].raw_data[0];                                         \
         Nums blocks_number = t1.core->blocks.size();                                            \
         for (Nums i = 0; i < blocks_number; i++) {                                              \
            ScalarType1* __restrict a = t1.core->blocks[i].raw_data.data();                      \
            Size block_size = t1.core->blocks[i].size;                                           \
            for (Size j = 0; j < block_size; j++) {                                              \
               EVAL1;                                                                            \
            }                                                                                    \
         }                                                                                       \
      } else {                                                                                   \
         if (!((t1.names == t2.names) && (t1.core->edges == t2.core->edges))) {                  \
            TAT_WARNING("Scalar Operator In Different Shape Tensor");                            \
         }                                                                                       \
         Nums blocks_number = t1.core->blocks.size();                                            \
         for (Nums i = 0; i < blocks_number; i++) {                                              \
            ScalarType1* __restrict a = t1.core->blocks[i].raw_data.data();                      \
            const ScalarType2* __restrict b = t2.core->blocks[i].raw_data.data();                \
            Size block_size = t1.core->blocks[i].size;                                           \
            for (Size j = 0; j < block_size; j++) {                                              \
               EVAL2;                                                                            \
            }                                                                                    \
         }                                                                                       \
      }                                                                                          \
      return t1;                                                                                 \
   }                                                                                             \
   template<                                                                                     \
         class ScalarType1,                                                                      \
         class ScalarType2,                                                                      \
         class Symmetry,                                                                         \
         class = std::enable_if_t<is_scalar_v<ScalarType2>>>                                     \
   Tensor<ScalarType1, Symmetry>& OP(Tensor<ScalarType1, Symmetry>& t1, const ScalarType2& n2) { \
      return OP(t1, Tensor<ScalarType2, Symmetry>{n2});                                          \
   }
   DEF_SCALAR_OP(operator+=, a[j] += y, a[j] += b[j])
   DEF_SCALAR_OP(operator-=, a[j] -= y, a[j] -= b[j])
   DEF_SCALAR_OP(operator*=, a[j] *= y, a[j] *= b[j])
   DEF_SCALAR_OP(operator/=, a[j] /= y, a[j] /= b[j])
#undef DEF_SCALAR_OP

   template<class T>
   void raw_write(std::ostream& out, const T* data, Size number = 1) {
      out.write(reinterpret_cast<const char*>(data), sizeof(T) * number);
   }
   template<class T>
   void raw_read(std::istream& in, T* data, Size number = 1) {
      in.read(reinterpret_cast<char*>(data), sizeof(T) * number);
   }
   bool is_text_stream(const std::ostream& out) {
      return &out == &std::cout || &out == &std::cerr || &out == &std::clog;
   }

   std::ostream& operator<<(std::ostream& out, const Name& name) {
      if (is_text_stream(out)) {
         auto pos = id_to_name.find(name.id);
         if (pos == id_to_name.end()) {
            return out << "UserDefinedName" << name.id;
         } else {
            return out << id_to_name.at(name.id);
         }
      } else {
         raw_write(out, &name.id);
      }
      return out;
   }

   std::istream& operator>>(std::istream& in, Name& name) {
      raw_read(in, &name.id);
      return in;
   }

   template<class T>
   std::ostream& operator<<(std::ostream& out, const vector<T>& vec) {
      if (is_text_stream(out)) {
         out << "[";
         bool notFirst = false;
         for (const auto& i : vec) {
            if (notFirst) {
               out << ",";
            }
            notFirst = true;
            out << i;
         }
         out << "]";
      } else {
         for (const auto& i : vec) {
            out << i;
         }
      }
      return out;
   }
   template<class T>
   std::istream& operator>>(std::istream& in, vector<T>& vec) {
      for (auto& i : vec) {
         in >> i;
      }
      return in;
   }
   std::ostream& operator<<(std::ostream& out, const NoSymmetry&) {
      return out;
   }
   std::istream& operator>>(std::istream& in, NoSymmetry&) {
      return in;
   }
   std::ostream& operator<<(std::ostream& out, const Z2Symmetry& s) {
      if (is_text_stream(out)) {
         out << s.z2;
      } else {
         raw_write(out, &s.z2);
      }
      return out;
   }
   std::istream& operator>>(std::istream& in, Z2Symmetry& s) {
      raw_read(in, &s.z2);
      return in;
   }
   std::ostream& operator<<(std::ostream& out, const U1Symmetry& s) {
      if (is_text_stream(out)) {
         out << s.u1;
      } else {
         raw_write(out, &s.u1);
      }
      return out;
   }
   std::istream& operator>>(std::istream& in, U1Symmetry& s) {
      raw_read(in, &s.u1);
      return in;
   }
   std::ostream& operator<<(std::ostream& out, const FermiSymmetry& s) {
      if (is_text_stream(out)) {
         out << s.fermi;
      } else {
         raw_write(out, &s.fermi);
      }
      return out;
   }
   std::istream& operator>>(std::istream& in, FermiSymmetry& s) {
      raw_read(in, &s.fermi);
      return in;
   }

   template<class Symmetry>
   std::ostream& operator<<(std::ostream& out, const Edge<Symmetry>& edge) {
      if (is_text_stream(out)) {
         if constexpr (std::is_same_v<Symmetry, NoSymmetry>) {
            out << edge.at(NoSymmetry());
         } else {
            out << "{";
            bool notFirst = false;
            for (const auto& [sym, dim] : edge) {
               if (notFirst) {
                  out << ",";
               }
               notFirst = true;
               out << sym << ":" << dim;
            }
            out << "}";
         }
      } else {
         if constexpr (std::is_same_v<Symmetry, NoSymmetry>) {
            raw_write(out, &edge.at(NoSymmetry()));
         } else {
            auto nums = Nums(edge.size());
            raw_write(out, &nums);
            for (const auto& [sym, dim] : edge) {
               out << sym;
               raw_write(out, &dim);
            }
         }
      }
      return out;
   }
   template<class Symmetry>
   std::istream& operator>>(std::istream& in, Edge<Symmetry>& edge) {
      if constexpr (std::is_same_v<Symmetry, NoSymmetry>) {
         Size dim;
         raw_read(in, &dim);
         edge[NoSymmetry()] = dim;
      } else {
         Nums nums;
         raw_read(in, &nums);
         edge.clear();
         for (Nums i = 0; i < nums; i++) {
            Symmetry sym;
            Size dim;
            in >> sym;
            raw_read(in, &dim);
            edge[sym] = dim;
         }
      }
      return in;
   }

   template<class ScalarType, class Symmetry>
   std::ostream& operator<<(std::ostream& out, const Block<ScalarType, Symmetry>& block) {
      if (!is_text_stream(out)) {
         TAT_WARNING("Should Not Come Here");
      }
      out << "{";
      if constexpr (!std::is_same_v<Symmetry, NoSymmetry>) {
         out << "symmetry:[";
         bool notFirst = false;
         for (const auto& i : block.symmetries) {
            if (notFirst) {
               out << ",";
            }
            notFirst = true;
            out << i;
         }
         out << "],";
      }
      out << "size:";
      out << block.size;
      out << ",data:[";
      bool notFirst = false;
      for (Size i = 0; i < block.size; i++) {
         if (notFirst) {
            out << ",";
         }
         notFirst = true;
         out << block.raw_data[i];
      }
      out << "]}";
      return out;
   }

   template<class ScalarType, class Symmetry>
   std::ostream& operator<<(std::ostream& out, const Tensor<ScalarType, Symmetry>& tensor) {
      if (is_text_stream(out)) {
         out << "{names:";
         out << tensor.names;
         out << ",edges:";
         out << tensor.core->edges;
         out << ",blocks:";
         if constexpr (std::is_same_v<Symmetry, NoSymmetry>) {
            out << tensor.core->blocks[0];
         } else {
            out << tensor.core->blocks;
         }
         out << "}";
      } else {
         auto rank = Rank(tensor.names.size());
         raw_write(out, &rank);
         out << tensor.names;
         out << tensor.core->edges;
         for (const auto& i : tensor.core->blocks) {
            raw_write(out, i.raw_data.data(), i.size);
         }
      }
      return out;
   }
   /*
   template<class ScalarType, class Symmetry>
   MPIFile& operator<<(MPIFile& out, const Tensor<ScalarType, Symmetry>& tensor) {
      std::stringstream ss;
      auto rank = Rank(tensor.names.size());
      raw_write(ss, &rank);
      ss << tensor.names;
      ss << tensor.core->edges;
      return out;
   }
   */

   template<class ScalarType, class Symmetry>
   std::istream& operator>>(std::istream& in, Tensor<ScalarType, Symmetry>& tensor) {
      Rank rank;
      raw_read(in, &rank);
      tensor.names.resize(rank);
      in >> tensor.names;
      tensor.name_to_index = construct_name_to_index(tensor.names);
      vector<Edge<Symmetry>> edges(rank);
      in >> edges;
      tensor.core =
            std::make_shared<typename Tensor<ScalarType, Symmetry>::TensorCore>(std::move(edges));
      if (!is_valid_name(tensor.names, tensor.core->edges.size())) {
         TAT_WARNING("Invalid Names");
      }
      for (auto& i : tensor.core->blocks) {
         raw_read(in, i.raw_data.data(), i.size);
      }
      return in;
   }

   // TODO: lazy framework
   // 需要考虑深搜不可行的问题
   // 支持inplace操作

   // GPU and so on
} // namespace TAT

#endif
