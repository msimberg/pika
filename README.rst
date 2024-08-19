..
    Copyright (c) 2022-2023 ETH Zurich

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

|zenodo|
|circleci_status|
|github_actions_linux_debug_status|
|github_actions_linux_hip_status|
|github_actions_linux_asan_ubsan_lsan_status|
|github_actions_linux_tsan_status|
|github_actions_macos_debug_status|
|cscsci|
|codacy|
|codacy_coverage|

====
pika
====

pika is a C++ library for concurrency and parallelism. It implements
senders/receivers (as proposed in `P2300 <https://wg21.link/p2300>`_) for CPU
thread pools, MPI, and CUDA.

To get started using pika see the `documentation <https://pikacpp.org>`_.

.. |zenodo| image:: https://zenodo.org/badge/DOI/10.5281/zenodo.10579225.svg
     :target: https://doi.org/10.5281/zenodo.10579225
     :alt: Zenodo

.. |circleci_status| image:: https://circleci.com/gh/pika-org/pika/tree/main.svg?style=svg
     :target: https://circleci.com/gh/pika-org/pika/tree/main
     :alt: CircleCI

.. |github_actions_linux_debug_status| image:: https://github.com/pika-org/pika/actions/workflows/linux_debug.yml/badge.svg
     :target: https://github.com/pika-org/pika/actions/workflows/linux_debug.yml
     :alt: Linux CI (Debug)

.. |github_actions_linux_hip_status| image:: https://github.com/pika-org/pika/actions/workflows/linux_hip.yml/badge.svg
     :target: https://github.com/pika-org/pika/actions/workflows/linux_hip.yml
     :alt: Linux CI (HIP, Debug)

.. |github_actions_linux_asan_ubsan_lsan_status| image:: https://github.com/pika-org/pika/actions/workflows/linux_asan_ubsan_lsan.yml/badge.svg
     :target: https://github.com/pika-org/pika/actions/workflows/linux_asan_ubsan_lsan.yml
     :alt: Linux CI (asan/ubsan/lsan)

.. |github_actions_linux_tsan_status| image:: https://github.com/pika-org/pika/actions/workflows/linux_tsan.yml/badge.svg
     :target: https://github.com/pika-org/pika/actions/workflows/linux_tsan.yml
     :alt: Linux CI (asan/ubsan/lsan)

.. |github_actions_macos_debug_status| image:: https://github.com/pika-org/pika/actions/workflows/macos_debug.yml/badge.svg
     :target: https://github.com/pika-org/pika/actions/workflows/macos_debug.yml
     :alt: macOS CI (Debug)

.. |cscsci| image:: https://gitlab.com/cscs-ci/ci-testing/webhook-ci/mirrors/479009878135925/5304355110917878/badges/main/pipeline.svg
     :target: https://gitlab.com/cscs-ci/ci-testing/webhook-ci/mirrors/479009878135925/5304355110917878/-/commits/main
     :alt: CSCS CI

.. |codacy| image:: https://app.codacy.com/project/badge/Grade/e03f57f1c4cd40e7b514e552a723c125
     :target: https://app.codacy.com/gh/pika-org/pika
     :alt: Codacy

.. |codacy_coverage| image:: https://app.codacy.com/project/badge/Coverage/e03f57f1c4cd40e7b514e552a723c125
     :target: https://app.codacy.com/gh/pika-org/pika
     :alt: Codacy coverage

--------------
modules branch
--------------

This branch attempts to add support for C++ 20 modules to pika. The branch is incomplete. Currently only some (pika) modules are compiled as C++ modules. See ()[libs/pika/CMakeLists.txt] for a list of enabled modules.
