..
    Copyright (c) 2022-2023 ETH Zurich

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _usage:

=====
Usage
=====

.. _getting_started:

Getting started
===============

The recommended way to install pika is through `spack <https://spack.readthedocs.io>`_:

.. code-block:: bash

   spack install pika

See

.. code-block:: bash

   spack info pika

for available options.

Manual installation
-------------------

If you'd like to build pika manually you will need CMake 3.22.0 or greater and a recent C++ compiler
supporting C++17:

- `GCC <https://gcc.gnu.org>`_ 9 or greater
- `clang <https://clang.llvm.org>`_ 11 or greater

Additionally, pika depends on:

- `header-only Boost <https://boost.org>`_ 1.71.0 or greater
- `hwloc <https://www-lb.open-mpi.org/projects/hwloc/>`_ 1.11.5 or greater
- `fmt <https://fmt.dev/latest/index.html>`_ 9.0.0 or greater

pika optionally depends on:

* `gperftools/tcmalloc <https://github.com/gperftools/gperftools>`_, `jemalloc
  <http://jemalloc.net/>`_, or `mimalloc <https://github.com/microsoft/mimalloc>`_. It is *highly*
  recommended to use one of these allocators as they perform significantly better than the system
  allocators. You can set the allocator through the CMake variable ``PIKA_WITH_MALLOC``. If you want
  to use the system allocator (e.g. for debugging) you can do so by setting
  ``PIKA_WITH_MALLOC=system``.
* `CUDA <https://docs.nvidia.com/cuda/>`_ 11.0 or greater. CUDA support can be enabled with
  ``PIKA_WITH_CUDA=ON``. pika can also be built with nvc++ from the `NVIDIA HPC SDK
  <https://developer.nvidia.com/hpc-sdk>`_. In the latter case, set ``CMAKE_CXX_COMPILER`` to
  ``nvc++``.
* `HIP <https://rocmdocs.amd.com/en/latest/index.html>`_ 5.2.0 or greater. HIP support can be
  enabled with ``PIKA_WITH_HIP=ON``.
* `MPI <https://www.mpi-forum.org/>`_. MPI support can be enabled with ``PIKA_WITH_MPI=ON``.
* `Boost.Context <https://boost.org>`_ on macOS or exotic platforms which are not supported by the
  default user-level thread implementations in pika. This can be enabled with
  ``PIKA_WITH_GENERIC_CONTEXT_COROUTINES=ON`` (TODO rename option...).
* `stdexec <https://github.com/NVIDIA/stdexec>`_. stdexec support can be enabled with
  ``PIKA_WITH_STDEXEC=ON`` (currently tested with commit `7a47a4aa411c1ca9adfcb152c28cc3dd7b156b4d
  <https://github.com/NVIDIA/stdexec/commit/7a47a4aa411c1ca9adfcb152c28cc3dd7b156b4d>`_).  The
  integration is experimental. See :ref:`pika_stdexec` for more information about the integration.

If you are using `nix <https://nixos.org>`_ you can also use the ``shell.nix`` file provided at the root of the repository to quickly enter a development environment:

.. code-block:: bash

   nix-shell <pika-root>/shell.nix

The ``nixpgks`` version is not pinned and may break occasionally.

Including in CMake projects
---------------------------

Once installed, pika can be used in a CMake project through the ``pika::pika`` target. Other ways of
depending on pika are likely to work but are not officially supported.

.. code-block:: cmake

   find_package(pika REQUIRED)
   add_executable(app main.cpp)
   target_link_libraries(app PRIVATE pika::pika)

.. _cmake_configuration:

Customizing the pika installation
=================================

The most important CMake options are listed in :ref:`getting_started`. Below is a more complete list
of CMake options you can use to customize the installation.

- ``PIKA_WITH_MALLOC``: This defaults to ``mimalloc`` which requires mimalloc to be installed.  Can
  be set to ``tcmalloc``, ``jemalloc``, ``mimalloc``, or ``system``. Setting it to ``system`` can be
  useful in debug builds.
- ``PIKA_WITH_CUDA``: Enable CUDA support.
- ``PIKA_WITH_HIP``: Enable HIP support.
- ``PIKA_WITH_MPI``: Enable MPI support.
- ``PIKA_WITH_APEX``: Enable `APEX <todo>`_ support.
- ``PIKA_WITH_TRACY``: Enable `Tracy <todo>`_ support.
- ``PIKA_WITH_GENERIC_CONTEXT_COROUTINES``: Enable the use of Boost.Context for fiber context
  switching.
- ``PIKA_WITH_TESTS``: Enable tests. Tests can be built with ``cmake --build . --target tests`` and
  run with ``ctest --output-on-failure``.
- ``PIKA_WITH_EXAMPLES``: Enable examples. Binaries will be placed under ``bin`` in the build
  directory.

.. _pika_stdexec:

Relation to std::execution and stdexec
======================================

When pika was first created as a fork of `HPX <https://github.com/STEllAR-GROUP/hpx>`_ in 2022
stdexec was in its infancy. Because of this, pika contains an implementation of a subset of the
earlier revisions of P2300. The main differences to stdexec and the proposed facilities are:

- The pika implementation uses C++17 and thus does not make use of concepts or coroutines. This
  allows compatibility with slightly older compiler versions and e.g. nvcc.
- The pika implementation uses ``value_types``, ``error_types``, and ``sends_done`` instead of
  ``completion_signatures`` in sender types, as in the `first 3 revisions of P2300
  <https://wg21.link/p2300r3>`_.

pika has an experimental CMake option ``PIKA_WITH_STDEXEC`` which can be enabled to use stdexec for
the P2300 facilities. pika brings the ``stdexec`` namespace into ``pika::execution::experimental``,
but provides otherwise no guarantees of interchangeable functionality. pika only implements a subset
of the proposed sender algorithms which is why we recommend that you enable ``PIKA_WITH_STDEXEC``
whenever possible. We plan to deprecate and remove the P2300 implementation in pika in favour of
stdexec and/or standard library implementations.
