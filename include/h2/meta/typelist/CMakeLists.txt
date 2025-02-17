################################################################################
## Copyright 2019-2024 Lawrence Livermore National Security, LLC and other
## DiHydrogen Project Developers. See the top-level LICENSE file for details.
##
## SPDX-License-Identifier: Apache-2.0
################################################################################

# Append this directory to the current install prefix
set(H2_CURRENT_INSTALL_PREFIX "${H2_CURRENT_INSTALL_PREFIX}/typelist")

# Setup this directory's files
h2_add_sources_to_target_and_install(
  TARGET H2Meta COMPONENT META SCOPE INTERFACE
  INSTALL_PREFIX "${H2_CURRENT_INSTALL_PREFIX}"
  SOURCES
  Append.hpp
  At.hpp
  CartProd.hpp
  Expand.hpp
  Find.hpp
  Flatten.hpp
  HaskellAccessors.hpp
  Length.hpp
  LispAccessors.hpp
  Map.hpp
  Member.hpp
  print.hpp
  Reduce.hpp
  Remove.hpp
  RemoveAll.hpp
  Repeat.hpp
  Replace.hpp
  ReplaceAll.hpp
  Reverse.hpp
  Select.hpp
  SelectAll.hpp
  Size.hpp
  Sort.hpp
  ToFromTuple.hpp
  TypeList.hpp
  Unique.hpp
  Zip.hpp
)
