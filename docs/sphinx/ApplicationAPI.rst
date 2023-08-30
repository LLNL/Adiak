Application API Guide
================================

This section describes Adiak's application interface. It is primarily routines 
for registering name/value pairs, and for initializing and flushing adiak data.

As previously discussed, Adiak has a C++ and a C interface. The C++ interface 
is wrapped in an `adiak::` namespace.  The C-interface functions are prefixed 
by the string `adiak_`.

Implicit functions
--------------------------------

Using datatypes
--------------------------------

Adiak and MPI
--------------------------------

API reference
--------------------------------

.. doxygengroup:: UserAPI
   :project: Adiak
   :members:
