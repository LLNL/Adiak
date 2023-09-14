Getting Adiak
=================================

Adiak is available through the
`Spack <https://github.com/spack/spack>`_ package manager.

.. code-block:: sh

    $ spack install adiak

When not using Spack, clone Adiak from its
`Github repository <https://github.com/LLNL/Adiak>`_.
Be sure to clone recursively, since Adiak imports the
`BLT <https://github.com/LLNL/blt>`_ CMake helper library as a git submodule.
Configure and install Adiak as needed, for example:

.. code-block:: sh

    $ git clone --recursive https://github.com/LLNL/Adiak.git
    $ mkdir build && cd build
    $ cmake -DBUILD_SHARED_LIBS=On -DENABLE_MPI=On -DCMAKE_INSTALL_PREFIX=<install dir> ..
    $ make && make install

There are a few useful CMake flags to set:

ENABLE_MPI
  Build Adiak with MPI support

BUILD_SHARED_LIBS
  Build Adiak as a shared library. Default is `Off`, that is, Adiak will be
  built as a static library.

Using Adiak in CMake
---------------------------------

Adiak installs CMake targets that can be imported with a CMake ``find_package``
call. In your CMake script, add ``adiak::adiak`` as a dependency to any executable
or library that requires Adiak: ::

    project(MyApp)
    find_package(adiak REQUIRED)
    add_executable(myapp myapp.cpp)
    target_link_libraries(myapp PUBLIC adiak::adiak)
