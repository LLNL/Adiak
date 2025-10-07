Python support
==============

Adiak provides Python bindings based on `pybind11 <https://pybind11.readthedocs.io/en/stable/>`_
for the metadata/annotation APIs (``init``, ``value``, ``fini``) and a set of convenience
collectors (e.g., ``collect_all()``, ``walltime()``).
To build Adiak with Python support, enable
the :code:`ENABLE_PYTHON_BINDINGS` option in the CMake configuration:

.. code-block:: sh

    $ cmake -DENABLE_PYTHON_BINDINGS=ON ..

If you want to initialize Adiak with an MPI communicator from Python, also enable (or auto-detect)
MPI at configure time (mpi4py is only needed at runtime if you actually pass a communicator):

.. code-block:: sh

    $ cmake -DENABLE_PYTHON_BINDINGS=ON -DENABLE_MPI=ON ..

Using the Python module
-----------------------

The Python module requires pybind11 and an installation of Python that both supports
pybind11 and provides development headers (e.g., :code:`Python.h`) and libraries
(e.g., :code:`libpython3.8.so`).

The Adiak Python module is installed in either :code:`lib/pythonX.Y/site-packages/` and/or
:code:`lib64/pythonX.Y/site-packages` in the Adiak installation directory. In these paths,
:code:`X.Y` corresponds to the major and minor version numbers of the Python installation used.
Additionally, :code:`lib/` and :code:`lib64/` will be used in accordance with the configuration
of the Python installed.

To use the Adiak Python module, simply add the directories above to :code:`PYTHONPATH` or
:code:`sys.path`. Note that the module will be automatically added to :code:`PYTHONPATH` when
loading the Adiak package with Spack if the :code:`python` variant is enabled.
The module can then be imported with :code:`import pyadiak`.

Example: basic usage (serial)
-----------------------------

.. code-block:: python

    from datetime import datetime
    from pathlib import Path

    from pyadiak.annotations import init, value, fini, collect_all, walltime, cputime, systime
    from pyadiak.types import Version, Path as APath, CatStr, Category

    def main():
        init(None)  # serial mode

        value("str", "s")
        value("compiler", Version("gcc@8.1.0"))
        value("mydouble", 3.14)
        value("problemsize", 14000, category=Category.Tuning)
        value("countdown", 9876543210)

        grid = [4.5, 1.18, 0.24, 8.92]
        value("gridvalues", grid)

        names = {"bob", "jim", "greg"}
        value("allnames", names)

        # Flatten nested structures (or use JsonStr)
        names_arr = ["first", "second", "third"]
        xs = [1.0, 2.0, 3.0]
        ys = [1.0, 4.0, 9.0]
        value("points.names", names_arr)
        value("points.x", xs)
        value("points.y", ys)

        # Time & paths are auto-wrapped; shown explicitly here for clarity
        value("birthday", datetime.fromtimestamp(286551000))   # Timepoint
        value("nullpath", APath(Path("/dev/null")))            # Path
        value("githash", CatStr("a0c93767478f23602c2eb317f641b091c52cf374"))

        collect_all()
        walltime()
        cputime()
        systime()

        fini()

    if __name__ == "__main__":
        main()


Example: MPI usage (optional)
-----------------------------

If built with :code:`-DENABLE_MPI=ON` and :code:`mpi4py` is available:

.. code-block:: python

    from mpi4py import MPI
    from pyadiak.annotations import init, value, fini

    def main():
        comm = MPI.COMM_WORLD
        init(comm)           # initialize with communicator
        value("rank", comm.Get_rank())
        fini()

    if __name__ == "__main__":
        main()
