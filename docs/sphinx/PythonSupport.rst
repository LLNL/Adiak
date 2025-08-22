Python support
==============

Adiak provides Python bindings based on `pybind11 <https://pybind11.readthedocs.io/en/stable/>`_.
To build Adiak with Python support, enable
the :code:`ENABLE_PYTHON_BINDINGS` option in the CMake configuration:

.. code-block:: sh

    $ cmake -DENABLE_PYTHON_BINDINGS=On ..

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
