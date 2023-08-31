Basic Usage
=================================

Adiak records application metadata in the form of name/value pairs.

To register a name/value pair with Adiak's C++ API:

.. code-block:: c++

    #include <adiak.hpp>

    int main(int argc, char* argv[])
    {
        adiak::init(nullptr);
        adiak::value("dimensions", 3);
        adiak::value("runid", "exec34_51");
        adiak::value("calced_sum", 5.15, adiak_physics);
        adiak::fini();
    }

To register the same name/value pairs with the C API:

.. code-block:: c

    #include <adiak.h>

    int main(int argc, char* argv[])
    {
        adiak_init(NULL);
        adiak_namevalue("dimensions", adiak_general, NULL, "%d", 3);
        adiak_namevalue("runid", adiak_general, NULL, "%s", "exec34_51");
        adiak_namevalue("calced_sum", adiak_physics, NULL, "%f", 5.15);
        adiak_fini();
    }

There are several points to note about the above examples:

* Adiak has different include files, ``adiak.h`` and ``adiak.hpp``, for the C
  and C++ interfaces respectively. You only need to use one.
* It is safe to mix calls to the C and C++ APIs, though these examples
  do not illustrate that.
* Before making any other Adiak API calls you must call adiak's init
  function. The NULL parameter in this example disables MPI support in Adiak.
  A pointer to an MPI communicator can be passed in this parameter to enable
  MPI support.
* Both of the :cpp:func:`value` and :cpp:func:`adiak_namevalue` functions are
  performing the same task: associating a group of names: `dimensions`,
  `runid`, and `calced_sum`, with respective values of types `int`, `string`,
  and `double`.  In the C++ example we can simply pass both the name and
  value to the ``adiak::value`` function. In the C example we have to describe
  the type of the value, using a printf-style string.
* The C example also sets an adiak category when it passes ``adiak_general``.
  This can be used to group name/value pairs into one of several categories,
  which some tools may find useful. The C++ interface can also set a category,
  but it is passed as a default parameter that is set to ``adiak_general`` already.
* We finish by calling the appropriate fini function to adiak.

Implicit Routines
-----------------

Adiak contains several convenience routines that register commonly used name/value
pairs under standardized names. For example:

.. code-block:: c++

    #include <adiak.hpp>

    int main(int argc, char* argv[])
    {
        adiak::init(nullptr);
        adiak::uid();
        adiak::date();
        adiak::fini();
    }

The :cpp:func:`uid` function creates a name of "uid" and associates it
with a string of the user id that owns this process. For example, this call
might create a name/value pair of `{ "uid": "jsmith1" }`. The
:cpp:func:`date` function associates a “date” name with the current
date and time. There are a larger set of standardized names in the
:doc:`ApplicationAPI` section. Equivalent functions are also available
in the C interface.

Tool Interface
-----------------

Tools can plug into Adiak to receive name/value pairs provided by the application.
The name/value pairs are provided by callbacks, which can be delivered in batches
at certain points or as the application provides them. For example:

.. code-block:: c

    #include <adiak_tool.h>

    void cb(
             const char*      name,
             adiak_category_t category,
             const char*      subcategory,
             adiak_value_t    *value,
             adiak_datatype_t *t,
             void             *user_arg
           )
    {
        char* typestr = adiak_type_to_string(t, 1);
        char* valstr = value_to_string(value, t);
        printf("Received name/value pair of %s/%s of type %s\n", name, valstr, typestr);
        free(valstr);
        free(typestr);
    }

    void init_tool()
    {
        adiak_register_cb(1, adiak_category_all, cb, 1, NULL);
    }

Unlike the application interface, there is only a C interface for tools.

In this example, the tool receives a callback to cb every time the
application uses adiak to register a name/value pair. The ``value_to_string``
function in this example processes the value, and can be found in the
:doc:`ToolAPI` section.

Multiple tools can register to receive Adiak callbacks. Tools can also
iterate over and examine the already-set name/value pairs.

.. _concepts:

Concepts
-----------------

As described in the introduction, Adiak is an interface for providing
name/value pairs to subscriber tools. This section describes several
other important design decisions.

Memory Management and Data Lifetime
...................................

Adiak makes a copy of every data value that is passed through the
application interface. This means that:

* It is safe to free data values after passing them to Adiak.
* Adiak is not appropriate for large data structures that consume
  a significant fraction of memory.

Adiak's data copies are deep, which means that containers and pointers
are followed when doing the copy.

The adiak utility API has calls, such as :cpp:func:`adiak_clean`,
which deallocate all data value copies made by Adiak.

The name C-style strings passed to Adiak are not copied—Adiak only
retains a pointer to these strings. The application should ensure
that such strings have a program lifetime or through the next
``adiak_clean`` call.

The tool interface can retain pointers to Adiak data values, through
if it does so the tool should watch for ``adiak_clean`` operations to
clean those pointers.

Data Types
.................

The details of Adiak's type system are covered in the :doc:`ApplicationAPI`
section, this section covers the high-level concepts.

Adiak needs a type system to manage data values. The application language
types are converted to Adiak types, which can then be copied and passed
to Adiak tools.

In most ways the Adiak type system is less flexible than the application
language's type system, and in other ways it is more specialized.

Adiak only supports a few base type: integrals, floating points, strings.
On top of those base types, Adiak provides specialized types that provide
more information about the data: filepaths, dates, version numbers, and
other specializations in that are described in the API section. All of these
types can be placed in containers: ranges, sets, lists, and tuples.

Containers can be nested and mixed. A two-dimensional array of integers could
be, for example, a list of a list of integers.  Adiak sets, lists, and tuples
include the size of the container in the type system—when using the C-style
application interface the size of the container will need to be passed
along with the data values.

In the C-style interface types are described with a printf-style type string,
which is passed along with the value when registering name/value pairs.
For example, integers are represented as `%d`, sets as brackets `[]`, and
lists as braces `{}`.

Categories
.................

Each Adiak name/value pair belongs to a "category". The tool interface allows
tools to query name/value pairs by category, e.g. to only process values of a
certain category.

Adiak contains two built-in categories: :c:macro:`adiak_general`, which should
be used for general run metadata, and :c:macro:`adiak_performance`, which should
be used for name/value pairs describing program performance. The values
:c:macro:`adiak_category_all` and :c:macro:`adiak_control` are special values
reserved for the tool API and should not be assigned to any name/value pair.

In addition to the built-in categories, users can also define custom categories.
However, values up to 1000 are reserved for Adiak, so any user-defined category
should have a value of 1001 or higher.

Users must provide a category when registering a name/value pair with
:cpp:func:`adiak_namevalue`. If unsure, use ``adiak_general``. In the
C++ API, the :cpp:func:`value` function uses ``adiak_general`` by default,
but users can overwrite this.

Name/value pairs can also be assigned an optional `subcategory` string. This 
can be helpful to e.g. group certain values together. Interpretation of the 
subcategory strings is entirely up to each application and tool.

Adiak and MPI
..................
