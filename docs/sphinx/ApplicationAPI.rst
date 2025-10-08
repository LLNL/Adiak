Application API Guide
================================

This section describes Adiak's application interface. It is primarily routines
for registering name/value pairs, and for initializing and flushing adiak data.

As previously discussed, Adiak has a C++ and a C interface. The C++ interface
is wrapped in the ``adiak`` namespace.  The C-interface functions
are prefixed by the string ``adiak_``.

See :ref:`concepts` for an overview of important design decisions in Adiak.

Implicit routines
--------------------------------

Adiak's implicit routines register commonly used name/value pairs about a
job's execution environment under common names. The table below lists
the available implicit routines, and the name/value pair they register.

+---------------------------------+----------------+-------------------------------------+
| Function                        | Name           | Description                         |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`adiakversion`        | adiakversion   | Adiak library version               |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`user`                | user           | Name of user running the job        |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`uid`                 | uid            | User ID of user running the job     |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`launchdate`          | launchdate     | Day/time of job launch (UNIX time)  |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`launchday`           | launchday      | Day of job launch (UNIX time)       |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`executable`          | executable     | Executable name                     |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`executablepath`      | executablepath | Full path to the program executable |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`workdir`             | workdir        | Working directory for the job       |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`libraries`           | libraries      | Set of loaded shared libraries      |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`cmdline`             | cmdline        | Program command line parameters     |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`hostname`            | hostname       | Network host name                   |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`clustername`         | clustername    | Cluster name (hostname w/o numbers) |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`walltime`            | walltime       | Process walltime                    |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`cputime`             | cputime        | Process CPU time                    |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`systime`             | systime        | Process system time                 |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`jobsize`             | jobsize        | MPI job size                        |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`numhosts`            | numhosts       | Number of distinct nodes in MPI job |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`hostlist`            | hostlist       | List of distinct nodes in MPI job   |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`mpi_version`         | mpi_version    | MPI standard version                |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`mpi_library`         | mpi_library    | MPI_Get_library_version() output    |
+---------------------------------+----------------+-------------------------------------+
| :cpp:func:`mpi_library_version` | mpi_library_   | MPI library version and vendor      |
|                                 | vendor/version |                                     |
+---------------------------------+----------------+-------------------------------------+

The catchall function :cpp:func:`adiak_collect_all` function collects all of the
common name/value pairs except walltime, systime, and cputime.

Using datatypes
--------------------------------

Adiak uses a custom type system for its name/value pairs. The type system supports
scalars, string-based types, and compound types like lists. In addition to basic
C/C++ datatypes like integers and strings, Adiak's type system also includes types
to encode specific kinds on values like calendar dates, file paths, or program
versions. See :cpp:enum:`adiak_type_t` for the full list of Adiak datatypes.

Each Adiak datatype also belongs to a value category like "rational", "ordinal",
or "categorical". This information helps tools pick appropriate visualizations for
for the type, for example a bar chart vs. a line chart. The full type information is
encoded in an :cpp:struct:`adiak_datatype_t` object, which includes the kind, value
category, and (for compound types) information about the sub-type(s) and number
of elements.

Users don't usually create :cpp:struct:`adiak_datatype_t` objects directly, but
instead use Adiak's convenience functionality. In C, the :cpp:func:`adiak_namevalue`
function uses printf-style type descriptors to specify the datatype; for example,
"%d" to create an :cpp:enumerator:`adiak_type_t::adiak_int` and "%v"
to create a :cpp:enumerator:`adiak_type_t::adiak_version` name/value pair. See
:cpp:enum:`adiak_type_t` for all available typestrings. For example:

.. code-block:: c

   /* create an adiak_double name/value pair*/
   adiak_namevalue("pi", adiak_general, NULL, "%f", 3.14159);
   /* create an adiak_path name/value pair */
   adiak_namevalue("input file", adiak_general, NULL, "%p", "/home/user/sim.in");

In C++, the :cpp:func:`value` template function automatically derives a
suitable Adiak datatype from the given value. In addition, there are type
adapters (:cpp:struct:`date`, :cpp:struct:`version`, :cpp:struct:`path`, and
:cpp:struct:`catstring`) to create the special adiak types :cpp:enumerator:`adiak_type_t::adiak_date`,
:cpp:enumerator:`adiak_type_t::adiak_version`, etc., from a ``long`` or string
value, respectively. For example:

.. code-block:: c++

   // create an adiak_double name/value pair
   adiak::value("pi", 3.14159);
   // create an adiak_path name/value pair
   adiak_value("input file", adiak::path("/home/user/sim.in"));

Compound types
................................

Adiak allows the creation of compound types including lists, sets, and tuples.
Compound types can be recursive, so it is possible to create, for example,
a list of tuples.

In the C API, the typestring for compound types uses a constructor and a
subtype, for example "{%d}" for a list of ints. The value is provided via the
varargs parameters for :cpp:func:`adiak_namevalue` as a C-style array followed by
the data dimensions (e.g., the array length). Recursive type definitions use
multiple dimension parameters, specified in order from the outermost to the
innermost type. For example:

.. code-block:: c

   /* create a set of ints */
   int squares[4] = { 1, 4, 9, 16 };
   adiak_namevalue("squares", adiak_general, NULL, "[%d]", squares, 4);

   /* create a (int,string,path) tuple */
   struct tuple_t {
      int i; char* s; char* p;
   } t = {
      1, "a", "/home/usr"
   };
   adiak_namevalue("mytuple", adiak_general, NULL, "(%d,%s,%p)", &t, 3);

   /* create a list of (int,string) tuples */
   struct int_string_t { int i; char* s; };
   int_string_t hello[3] = { { 1, "hello" }, { 2, "adiak" }, { 3, "!" } };
   adiak_namevalue("hello", adiak_general, NULL, "{(%d,%s)}", hello, 3, 2);

In the C++ API, the :cpp:func:`value` template function creates compound types
when given specific STL containers as value, e.g. ``std::tuple`` for tuples or
``std::vector`` for lists. For example:

.. code-block:: c++

   // create a set of ints
   std::set<int> squares { 1, 4, 9, 16 };
   adiak::value("squares", squares);

   // create a (int,string,path) tuple
   std::tuple<int,std::string,adiak::path> t { 1, "a", adiak::path("/home/usr") };
   adiak::value("mytuple", t);

   // create a list of (int,string) tuples
   std::vector<std::tuple<int,std::string>> hello { { 1, "hello" }, { 2, "adiak" }, { 3, "!" } };
   adiak::value("hello", hello);

For detailed information on how to create each compound type, refer to the
:cpp:enum:`adiak_type_t` documentation.

Scalar size specifiers
................................

Adiak internally stores scalar values as 32-bit integer, 64-bit integer, or
64-bit floating point values. Adiak automatically converts smaller integers
(such as ``short``) to ``int``, and ``float`` to ``double``. However, when types
other than (unsigned) ``int`` or ``double`` are used in compound types, you must
use a length specifier in the :cpp:func:`adiak_namevalue` type string argument
to indicate the exact input type, e.g. `"%f32"` for ``float`` or `"%i16"`
for ``signed short``. The available type specifiers are "%i8", "%i16", "%i32",
"%u8", "%u16", "%u32", "%f32", and "%f64". For 64-bit integers, use "%lld" or
"%llu". Examples:

.. code-block:: c

   const unsigned char bytes[4] = { 32, 33, 34, 35 };
   const signed short smallints[4] = { -1024, 0, 1, 512 };
   const float floats[3] = { 0.42, 42.0, 42.42 };

   adiak_namevalue("bytes", adiak_general, NULL, "{%u8}", bytes, 4);
   adiak_namevalue("smallints", adiak_general, NULL, "[%i16]", smallints, 4);
   adiak_namevalue("floats", adiak_general, NULL, "{%f32}", floats, 3);

.. _reference_values:

Reference values
................................

Optionally, values can be stored as references, where Adiak does not copy the data
but only keeps a pointer to it.
Reference entries are available for string types and compound types (lists, sets,
etc.). Scalar types (int, double, etc.) cannot be stored as references.

Any objects stored as references must be retained in memory by the application
throughout the lifetime of the program or until :cpp:func:`adiak_clean` is called.

Currently, reference values must be created through the C API. To do so,
place a ``&`` in front of the type specifier in :cpp:func:`adiak_namevalue`,
e.g. ``&{%d}`` for a list of integers or ``&%s`` for a string. For compound
types, the ``&`` can be applied to an inner type to create a shallow copy, e.g.
``{&%s}`` for a shallow-copy list of zero-copy strings. Examples:

.. code-block:: c

   static const int array[9] = { -1, 2, -3, 4, 5, 4, 3, 2, 1 };
   static const char* string_a = "A long string";
   static const char* string_b = "Another long string";

   struct int_string_tuple {
       long long i; const char* str;
   };
   static const struct int_string_tuple data[3] = {
       { 1, "Hello" }, { 2, "Adiak" }, { 3, "!" }
   };

   /* A zero-copy list of ints */
   adiak_namevalue("array", adiak_general, NULL, "&{%d}", array, 9);
   /* A zero-copy string */
   adiak_namevalue("string", adiak_general, NULL, "&%s", string_a);
   /* A set of zero-copy strings (shallow-copies the list but not the strings)*/
   const char* strings[2] = { string_a, string_b };
   adiak_namevalue("set_of_strings", adiak_general, NULL, "[&%s]", strings, 2);
   /* A full deep copy list of tuples */
   adiak_namevalue("data_deepcopy", adiak_general, NULL, "{(%lld,%s)}", hello_data, 3, 2);
   /* A zero-copy list of tuples */
   adiak_namevalue("data_ref_0", adiak_general, NULL, "&{(%lld,%s)}", hello_data, 3, 2);
   /* A shallow-copy list of zero-copy tuples */
   adiak_namevalue("data_ref_1", adiak_general, NULL, "{&(%lld,%s)}", hello_data, 3, 2);
   /* A shallow-copy list of tuples with zero-copy strings */
   adiak_namevalue("data_ref_2", adiak_general, NULL, "{(%lld,&%s)}", hello_data, 3, 2);

API reference
--------------------------------

.. doxygengroup:: UserAPI
   :project: Adiak
   :members:
