/**
 * \file get_item.hpp
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
#ifndef TAT_GET_ITEM_HPP
#define TAT_GET_ITEM_HPP

#include "tensor.hpp"

namespace TAT {
#ifndef TAT_DOXYGEN_SHOULD_SKIP_THIS
   template<typename Symmetry, typename Name>
   [[nodiscard]] auto get_block_for_get_item(const std::map<Name, Symmetry>& position, const std::vector<Name>& names) {
      auto symmetries = std::vector<Symmetry>();
      symmetries.reserve(names.size());
      for (const auto& name : names) {
         if (auto found = position.find(name); found != position.end()) {
            symmetries.push_back(found->second);
         } else {
            TAT_error("Name not found in position map when finding block");
         }
      }
      return symmetries;
   }

   template<typename ScalarType, typename Symmetry, typename Name>
   [[nodiscard]] auto
   get_offset_for_get_item(const std::map<Name, Size>& position, const std::vector<Name>& names, const Core<ScalarType, Symmetry>& core) {
      const auto rank = Rank(names.size());
      auto scalar_position = std::vector<Size>();
      auto dimensions = std::vector<Size>();
      scalar_position.reserve(rank);
      dimensions.reserve(rank);
      for (auto i = 0; i < rank; i++) {
         if (auto found = position.find(names[i]); found != position.end()) {
            scalar_position.push_back(found->second);
         } else {
            TAT_error("Name not found in position map when finding offset");
         }
         dimensions.push_back(core.edges[i].map.begin()->second);
      }
      Size offset = 0;
      for (Rank j = 0; j < rank; j++) {
         offset *= dimensions[j];
         offset += scalar_position[j];
      }
      return offset;
   }

   template<typename ScalarType, typename Symmetry, typename Name>
   [[nodiscard]] auto get_block_and_offset_for_get_item(
         const std::map<Name, std::tuple<Symmetry, Size>>& position,
         const std::vector<Name>& names,
         const Core<ScalarType, Symmetry>& core) {
      const auto rank = Rank(core.edges.size());
      auto symmetries = std::vector<Symmetry>();
      auto scalar_position = std::vector<Size>();
      auto dimensions = std::vector<Size>();
      symmetries.reserve(rank);
      scalar_position.reserve(rank);
      dimensions.reserve(rank);
      for (auto i = 0; i < rank; i++) {
         const auto& name = names[i];
         auto found = position.find(name);
         if (found == position.end()) {
            TAT_error("Name not found in position map when finding block and offset");
         }
         const auto& [symmetry, index] = found->second;
         symmetries.push_back(symmetry);
         scalar_position.push_back(index);
         dimensions.push_back(core.edges[i].map.at(symmetry));
      }
      Size offset = 0;
      for (Rank j = 0; j < rank; j++) {
         offset *= dimensions[j];
         offset += scalar_position[j];
      }
      return std::make_tuple(symmetries, offset);
   }
#endif

   template<typename ScalarType, typename Symmetry, typename Name>
   const auto& Tensor<ScalarType, Symmetry, Name>::const_block(const std::map<Name, Symmetry>& position) const& {
      if constexpr (std::is_same_v<Symmetry, NoSymmetry>) {
         return core->blocks.begin()->second;
      }
      auto symmetry = get_block_for_get_item(position, names);
      return core->blocks.at(symmetry);
   }

   template<typename ScalarType, typename Symmetry, typename Name>
   auto& Tensor<ScalarType, Symmetry, Name>::block(const std::map<Name, Symmetry>& position) & {
      if (core.use_count() != 1) {
         core = std::make_shared<Core<ScalarType, Symmetry>>(*core);
         TAT_warning_or_error_when_reference_which_may_change("Get reference which may change, use const_block to get const reference");
      }
      if constexpr (std::is_same_v<Symmetry, NoSymmetry>) {
         return core->blocks.begin()->second;
      }
      auto symmetry = get_block_for_get_item(position, names);
      return core->blocks.at(symmetry);
   }

   template<typename ScalarType, typename Symmetry, typename Name>
   const ScalarType& Tensor<ScalarType, Symmetry, Name>::const_at(const std::map<Name, EdgeInfoForGetItem>& position) const& {
      if constexpr (std::is_same_v<Symmetry, NoSymmetry>) {
         auto offset = get_offset_for_get_item(position, names, *core);
         return core->blocks.begin()->second[offset];
      } else {
         auto [symmetry, offset] = get_block_and_offset_for_get_item(position, names, *core);
         return core->blocks.at(symmetry)[offset];
      }
   }

   template<typename ScalarType, typename Symmetry, typename Name>
   ScalarType& Tensor<ScalarType, Symmetry, Name>::at(const std::map<Name, EdgeInfoForGetItem>& position) & {
      if (core.use_count() != 1) {
         core = std::make_shared<Core<ScalarType, Symmetry>>(*core);
         TAT_warning_or_error_when_reference_which_may_change("Get reference which may change, use const_at to get const reference");
      }
      if constexpr (std::is_same_v<Symmetry, NoSymmetry>) {
         auto offset = get_offset_for_get_item(position, names, *core);
         return core->blocks.begin()->second[offset];
      } else {
         auto [symmetry, offset] = get_block_and_offset_for_get_item(position, names, *core);
         return core->blocks.at(symmetry)[offset];
      }
   }
} // namespace TAT
#endif
