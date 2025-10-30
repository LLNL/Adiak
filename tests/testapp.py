import os

# Optional MPI
USE_MPI = os.environ.get("ADIAK_TEST_MPI", "0") == "1"
MPI = None
if USE_MPI:
    try:
        from mpi4py import MPI  # noqa: F401
    except Exception:
        USE_MPI = False
        MPI = None

from pyadiak.annotations import (
    init, value, fini,
    collect_all, walltime, cputime, systime, Version
)

def must_ok(ok, what):
    if not ok:
        raise AssertionError(f"adiak call failed: {what}")

def main():
    comm = MPI.COMM_WORLD if USE_MPI else None
    init(comm)

    # --- Scalars
    must_ok(value("str", "s"), "value(str)")
    must_ok(value("compiler", Version("gcc@8.1.0")), "value(Version)")
    must_ok(value("mydouble", 3.14), "value(double)")
    must_ok(value("problemsize", 14000), "value(int)")
    must_ok(value("countdown", 9876543210), "value(large int)")

    # --- Flat containers
    grid = [4.5, 1.18, 0.24, 8.92]
    must_ok(value("gridvalues", grid), "value(list[float])")

    names = {"matt", "david", "greg"}
    must_ok(value("allnames", names), "value(set[str])")

    # --- “points” and “antipoints”
    # Option A: parallel arrays (simple)
    ap_a = ["first", "second", "third"]
    ap_b = [1.0, 2.0, 3.0]
    ap_c = [1.0, 4.0, 9.0]
    must_ok(value("points.names", ap_a), "value(points.names)")
    must_ok(value("points.x", ap_b),     "value(points.x)")
    must_ok(value("points.y", ap_c),     "value(points.y)")
    must_ok(value("antipoints.a", ap_a), "value(antipoints.a)")
    must_ok(value("antipoints.b", ap_b), "value(antipoints.b)")
    must_ok(value("antipoints.c", ap_c), "value(antipoints.c)")

    floatar = [0.01, 0.02, 0.03]
    must_ok(value("floats", floatar), "value(list[float])")

    # System collectors / timers (if bound; they are in your annotations)
    must_ok(collect_all(), "collect_all()")
    must_ok(walltime(), "walltime()")
    must_ok(cputime(), "cputime()")
    must_ok(systime(), "systime()")

    fini()

if __name__ == "__main__":
    main()
