from datetime import date, datetime, time
from pathlib import Path as PyPath
from typing import Any
from warnings import warn

from pyadiak.__pyadiak_impl.annotations import (
    adiakversion,
    clean,
    clustername,
    cmdline,
    collect_all,
    cputime,
    executable,
    executablepath,
    fini,
    flush,
    hostlist,
    hostname,
    jobsize,
    launchdate,
    launchday,
    libraries,
    mpi_library,
    mpi_library_version,
    mpi_version,
    numhosts,
    systime,
    uid,
    user,
    walltime,
    workdir,
)
from pyadiak.__pyadiak_impl.annotations import (
    init as cpp_init,
)
from pyadiak.__pyadiak_impl.annotations import (
    value as cpp_value,
)
from pyadiak.__pyadiak_impl.annotations import (
    fini as cpp_fini,
)
from pyadiak.types import *

def fini():
    cpp_fini()

def init(comm=None):
    try:
        from mpi4py.MPI import Comm

        if comm is not None and not isinstance(comm, Comm):
            raise TypeError(
                "The 'comm' argument to 'init' must be of type mpi4py.MPI.Comm"
            )
        cpp_init(comm)
    except ImportError:
        if comm is not None:
            warn(
                "A value was passed to the 'comm' argument, but there is no way for it to be the correct type because mpi4py cannot be imported. If you want to pass an MPI communicator, install mpi4py first.",
                RuntimeWarning,
            )
        cpp_init(None)


def value(name: str, value: Any, category=Category.General, subcategory="") -> bool:
    wrapped_value = value
    if isinstance(value, date):
        wrapped_value = Date(value)
    elif isinstance(value, (time, datetime)):
        wrapped_value = Timepoint(value)
    elif isinstance(value, PyPath):
        wrapped_value = Path(value)
    if not isinstance(category, Category):
        raise TypeError(
            f"Invalid type ('{type(category)}') for 'category' parameter. Should be 'Category'"
        )
    if not isinstance(subcategory, str):
        raise TypeError(
            f"Invalid type ('{type(subcategory)}') for 'subcategory' parameter. Should be 'str'"
        )
    return cpp_value(name, wrapped_value, int(category), subcategory)


__all__ = [
    "init",
    "fini",
    "value",
    "adiakversion",
    "user",
    "uid",
    "launchdate",
    "launchday",
    "executable",
    "executablepath",
    "workdir",
    "libraries",
    "cmdline",
    "hostname",
    "clustername",
    "walltime",
    "systime",
    "cputime",
    "jobsize",
    "hostlist",
    "numhosts",
    "mpi_version",
    "mpi_library",
    "mpi_library_version",
    "flush",
    "clean",
    "collect_all",
]
