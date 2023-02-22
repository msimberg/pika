# Copyright (c) 2020 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cxx_std="20"
boost_version="1.78.0"
hwloc_version="2.7.0"
stdexec_version="c0fe16f451137b00ceb85d912ec3a288990ce766"
nvhpc_version="22.11"
spack_compiler="nvhpc@${nvhpc_version}"
spack_arch="cray-cnl7-haswell"

spack_spec="pika@main arch=${spack_arch} %${spack_compiler} +cuda malloc=system cxxstd=${cxx_std} ^boost@${boost_version} ^hwloc@${hwloc_version} ^stdexec@${stdexec_version}"

configure_extra_options+=" -DPIKA_WITH_CXX_STANDARD=${cxx_std}"
configure_extra_options+=" -DPIKA_WITH_MALLOC=system"
configure_extra_options+=" -DPIKA_WITH_CUDA=ON"
configure_extra_options+=" -DPIKA_WITH_P2300_REFERENCE_IMPLEMENTATION=ON"
