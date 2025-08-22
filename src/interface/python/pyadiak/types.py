from enum import IntEnum

from pyadiak.__pyadiak_impl.types import (
    ADIAK_CATEGORY_ALL,
    ADIAK_CATEGORY_CONTROL,
    ADIAK_CATEGORY_GENERAL,
    ADIAK_CATEGORY_PERFORMANCE,
    ADIAK_CATEGORY_UNSET,
    CatStr,
    Date,
    JsonStr,
    Path,
    Timepoint,
    Version,
)


class Category(IntEnum):
    Unset = ADIAK_CATEGORY_UNSET
    All = ADIAK_CATEGORY_ALL
    General = ADIAK_CATEGORY_GENERAL
    Performance = ADIAK_CATEGORY_PERFORMANCE
    Control = ADIAK_CATEGORY_CONTROL


__all__ = ["CatStr", "Date", "JsonStr", "Path", "Timepoint", "Version", "Category"]
