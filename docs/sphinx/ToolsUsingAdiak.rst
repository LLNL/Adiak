Tools using Adiak
=================

This section provides an overview of tools using Adiak.

.. _caliper-section:

Caliper
-----------------

`Caliper <https://github.com/LLNL/caliper>`_ is an instrumentation and performance
profiling library for HPC codes. Caliper can import Adiak run metadata into its
performance profile output. Refer to the Caliper
`documentation <https://software.llnl.gov/Caliper/index.html>`_ for installing
Caliper with Adiak support.

Typically, users add Caliper source-code annotations to their program to mark
regions of interest in the code. At runtime, users can select a Caliper measurement
recipe to record performance data and import any registered Adiak name/value pairs.
Here is a small example program:

.. code-block:: c++

    #include <adiak.hpp>
    #include <caliper/cali.h>

    int main(int argc, char* argv)
    {
        adiak::init(nullptr);
        adiak::date();

        adiak::value("dimensions", 3);
        adiak::value("runid", "exec34_51");
        adiak::value("calced_sum", 5.15, adiak_physics);

        CALI_MARK_BEGIN("main");
        // ...
        CALI_MARK_END("main");

        adiak::fini();
    }

Most of Caliper's measurement recipes import Adiak metadata. For example,
the `runtime-report` recipe can be augmented with the `print.metadata`
parameter, which will print a performance report and the imported Adiak
data (as well as Caliper-provided metadata) in a human-readable table.
An example report for the program above could look like this:

.. code-block::

    caliper.config       : runtime-report,print.metadata
    launchdate           :  1693355555
    dimensions           :           3
    runid                : exec34_51
    calced_sum           :        5.15
    cali.caliper.version : 2.10.0
    cali.channel         : runtime-report
    Path       Time (E) Time (I) Time % (E) Time % (I)
    main       0.000103 0.000103  65.368809  65.368809

The Adiak metadata is also included in Caliper's machine-readable .json
and .cali output files. It is particularly valuable for analyzing large
collections of runs with specialized analysis tools like
`Thicket <https://github.com/LLNL/thicket>`_.