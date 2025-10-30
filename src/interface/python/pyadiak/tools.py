from __future__ import annotations

from enum import IntEnum
from typing import Any, Callable, List, Tuple

from pyadiak.__pyadiak_impl.tools import (
    AdiakDataType,
    AdiakNumerical,
    AdiakTypeRaw,
)
from pyadiak.__pyadiak_impl.tools import (
    get_nameval as cpp_get_nameval,
)
from pyadiak.__pyadiak_impl.tools import (
    list_namevals as cpp_list_namevals,
)
from pyadiak.__pyadiak_impl.tools import (
    register_cb as cpp_register_cb,
)
from pyadiak.__pyadiak_impl.tools import (
    type_to_string as cpp_type_to_string,
)
from pyadiak.types import Category


class Type(IntEnum):
    Unset = AdiakTypeRaw.AdiakTypeUnset
    Long = AdiakTypeRaw.AdiakTypeLong
    ULong = AdiakTypeRaw.AdiakTypeULong
    Int = AdiakTypeRaw.AdiakTypeInt
    UInt = AdiakTypeRaw.AdiakTypeUInt
    Double = AdiakTypeRaw.AdiakTypeDouble
    Date = AdiakTypeRaw.AdiakTypeDate
    Timeval = AdiakTypeRaw.AdiakTypeTimeval
    Version = AdiakTypeRaw.AdiakTypeVersion
    String = AdiakTypeRaw.AdiakTypeString
    CatString = AdiakTypeRaw.AdiakTypeCatString
    Path = AdiakTypeRaw.AdiakTypePath
    Range = AdiakTypeRaw.AdiakTypeRange
    Set = AdiakTypeRaw.AdiakTypeSet
    List = AdiakTypeRaw.AdiakTypeList
    Tuple = AdiakTypeRaw.AdiakTypeTuple
    LongLong = AdiakTypeRaw.AdiakTypeLongLong
    ULongLong = AdiakTypeRaw.AdiakTypeULongLong
    JsonString = AdiakTypeRaw.AdiakTypeJsonString

    @staticmethod
    def from_cpp_type(v: AdiakTypeRaw) -> Type:
        return Type(int(v))


class Numerical(IntEnum):
    Unset = AdiakNumerical.AdiakNumericalUnset
    Categorical = AdiakNumerical.AdiakNumericalCategorical
    Ordinal = AdiakNumerical.AdiakNumericalOrdinal
    Interval = AdiakNumerical.AdiakNumericalInterval
    Rational = AdiakNumerical.AdiakNumericalRational

    @staticmethod
    def from_cpp_numerical(v: AdiakNumerical) -> Numerical:
        return Numerical(int(v))


class DataType:
    def __init__(self, raw_datatype: AdiakDataType):
        self.dtype = raw_datatype

    def get_dtype(self) -> Type:
        cpp_type = self.dtype.get_dtype()
        return Type.from_cpp_type(cpp_type)

    def get_numerical(self) -> Numerical:
        cpp_num = self.dtype.get_numerical()
        return Numerical.from_cpp_numerical(cpp_num)

    def get_num_elems(self) -> int:
        return self.dtype.get_num_elems()

    def get_num_subtypes_in_container(self) -> int:
        return self.dtype.get_num_subtypes_in_container()

    def is_reference(self) -> bool:
        return self.dtype.is_reference()

    def get_subtypes(self) -> List[DataType]:
        cpp_types = self.get_subtypes()
        return [DataType(ct) for ct in cpp_types]


AdiakToolCallback = Callable[str, Category, str, Any, DataType]


def __wrap_cb_in_caster_func(cb: AdiakToolCallback) -> Callable:
    def __wrapper_func(
        name: str, cat: int, subcat: str, val: Any, dtype: AdiakDataType
    ):
        local_category = Category(cat)
        local_dtype = DataType(dtype)
        cb(str, local_category, subcat, val, local_dtype)

    return __wrapper_func


def register_cb(
    adiak_version: int,
    cat: Category,
    callback: AdiakToolCallback,
    report_on_all_ranks: bool,
):
    if not isinstance(adiak_version, int):
        raise TypeError("The 'adiak_version' parameter must be an int")
    if not isinstance(cat, Category):
        raise TypeError("The 'cat' parameter must be a 'pyadiak.types.Category'")
    if not isinstance(callback, AdiakToolCallback):
        raise TypeError(
            "The 'callback' parameter must be a 'pyadiak.tools.AdiakToolCallback'"
        )
    wrapped_cb = __wrap_cb_in_caster_func(callback)
    cpp_register_cb(adiak_version, int(cat), wrapped_cb, report_on_all_ranks)


def list_namevals(adiak_version: int, cat: Category, callback: AdiakToolCallback):
    if not isinstance(adiak_version, int):
        raise TypeError("The 'adiak_version' parameter must be an int")
    if not isinstance(cat, Category):
        raise TypeError("The 'cat' parameter must be a 'pyadiak.types.Category'")
    if not isinstance(callback, AdiakToolCallback):
        raise TypeError(
            "The 'callback' parameter must be a 'pyadiak.tools.AdiakToolCallback'"
        )
    wrapped_cb = __wrap_cb_in_caster_func(callback)
    cpp_list_namevals(adiak_version, int(cat), wrapped_cb)


def type_to_string(dtype: DataType, long_form=False) -> str:
    if not isinstance(dtype, DataType):
        raise TypeError("The 'dtype' parameter must be a 'pyadiak.tools.DataType'")
    return cpp_type_to_string(dtype.dtype, long_form)


def get_nameval(name: str) -> Tuple[DataType, Any, Category, str]:
    cpp_tup = cpp_get_nameval(name)
    return (DataType(cpp_tup[0]), cpp_tup[1], Category(cpp_tup[2]), cpp_tup[3])


__all__ = [
    "Type",
    "Numerical",
    "DataType",
    "AdiakToolCallback",
    "register_cb",
    "list_namevals",
    "type_to_string",
    "get_nameval",
]
