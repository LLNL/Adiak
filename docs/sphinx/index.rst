.. Adiak documentation master file, created by
   sphinx-quickstart on Mon Aug 28 15:35:38 2023.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Adiak: Recording HPC Application Run Metadata
=============================================

Adiak is an API for recording meta-data about HPC simulations. An HPC 
application code may, for example, record what user invoked it, the 
version of the code being run, a computed time history showing density 
changes, or how long the application spent performing file IO.  
Adiak represents this metadata as name/value pairs. Names are arbitrary 
strings, with some standardization, and the values are represented by a 
flexible dynamic type system.

Tools can subscribe to Adiak's name/value pairs. Example tools could be
performance analysis tools recording metadata, a workflow manager, or a 
serializer that writes metadata to a standard format.

Adiak has a C and C++ API. The implementation is C-only, and the C++ API 
is header-only wrapper around the C interface. When using the C++ API values 
can be passed as template parameters, allowing for terse and convenient 
flexibility in value types. When using the C API values types are described 
by printf-inspired strings.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   GettingAdiak
   BasicUsage
   ToolsUsingAdiak
   ApplicationAPI
   ToolAPI
   PythonSupport

Contributing
==================

Contributions to Adiak are welcome, and should be made under the MIT license. 
Please submit pull requests to the github repository.

LLNL-CODE-792240.

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
