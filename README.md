Adiak: Recording HPC Run Metadata
==================

Adiak is a library for collecting metadata from HPC application runs, and distributing that metadata to subscriber tools.  Examples of Adiak metadata are:
- What user performed this run
- At what time did the run launch
- How long did the run spend in system time
- What MPI hosts are part of this run
- Application-provided parameters, such as peak velocity in an N-body problem

This metadata is provided as name/value pairs.  For application-provided data, an example name might be "peak_velocity" and and have a value of 5.6.  For other data, such as what user ran the code, there are adiak-managed standard names like "username".

Adiak has a tool interface, which allows tools to subscribe to this metadata.  Example tools might include performance analysis tools, workflow tracking tools, or anything else that needs this metadata.  Tools can iterate receive metadata name/values as they are provided by the application, or examine existing metadata values.

Documentation
----------------
More information can be found in https://llnl.github.io/Adiak/.

Contributing
----------------
Contributions to Adiak are welcome, and should be made under the MIT license.  Please submit pull requests to the github repository.


SPDX-License-Identifier: MIT

LLNL-CODE-792240
