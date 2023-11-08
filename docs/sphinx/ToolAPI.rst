Tool API Guide
================================

This section describes Adiak's tools interface, which provides routines for
querying name/value pairs.

Basics
-------------

There are three ways to query Adiak name/value pairs:

* Use :cpp:func:`adiak_register_cb` to register a callback function that is invoked every time an
  adiak name/value pair of a given category (or any category) is registered
* Use :cpp:func:`adiak_list_namevals` to iterate over all currently registered name/value pairs
* Use :cpp:func:`adiak_get_nameval` to query a specific name/value pair by name

Interpreting Adiak values
.........................

The Adiak query methods return a :cpp:struct:`adiak_value_t` union and a
:cpp:struct:`adiak_datatype_t` object. The ``adiak_value_t`` union must be interpreted
based on the datatype. For example, for a ``adiak_catstring`` type, use the ``v_ptr``
union member cast to ``char*``. Compound types such as lists must be interpreted recursively.
See the :cpp:struct:`adiak_value_t` reference for the complete list of datatypes.

Example
.............

The ``print_nameval`` example below implements a Adiak callback function that
prints Adiak name/value pairs.

.. code-block:: c

   static void print_value(adiak_value_t *val, adiak_datatype_t *t)
   {
      if (!t)
         printf("ERROR");
      switch (t->dtype) {
         case adiak_type_unset:
            printf("UNSET");
            break;
         case adiak_long:
            printf("%ld", val->v_long);
            break;
         case adiak_ulong:
            printf("%lu", (unsigned long) val->v_long);
            break;
         case adiak_longlong:
            printf("%lld", val->v_longlong);
            break;
         case adiak_ulonglong:
            printf("%llu", (unsigned long long) val->v_longlong);
            break;
         case adiak_int:
            printf("%d", val->v_int);
            break;
         case adiak_uint:
            printf("%u", (unsigned int) val->v_int);
            break;
         case adiak_double:
            printf("%f", val->v_double);
            break;
         case adiak_date: {
            char datestr[512];
            signed long seconds_since_epoch = (signed long) val->v_long;
            struct tm *loc = localtime(&seconds_since_epoch);
            strftime(datestr, sizeof(datestr), "%a, %d %b %Y %T %z", loc);
            printf("%s", datestr);
            break;
         }
         case adiak_timeval: {
            struct timeval *tval = (struct timeval *) val->v_ptr;
            double duration = tval->tv_sec + (tval->tv_usec / 1000000.0);
            printf("%fs (timeval)", duration);
            break;
         }
         case adiak_version: {
            char *s = (char *) val->v_ptr;
            printf("%s (version)", s);
            break;
         }
         case adiak_string: {
            char *s = (char *) val->v_ptr;
            printf("%s (string)", s);
            break;
         }
         case adiak_catstring: {
            char *s = (char *) val->v_ptr;
            printf("%s (catstring)", s);
            break;
         }
         case adiak_path: {
            char *s = (char *) val->v_ptr;
            printf("%s (path)", s);
            break;
         }
         case adiak_range: {
            adiak_value_t subvals[2];
            adiak_datatype_t* subtypes[2];

            adiak_get_subval(t, val, 0, subtypes+0, subvals+0);
            adiak_get_subval(t, val, 1, subtypes+1, subvals+1);

            print_value(subvals+0, *(subtypes+0));
            printf(" - ");
            print_value(subvals+1, *(subtypes+1));
            break;
         }
         case adiak_set: {
            printf("[");
            for (int i = 0; i < adiak_num_subvals(t); i++) {
               adiak_value_t subval;
               adiak_datatype_t* subtype;
               adiak_get_subval(t, val, i, &subtype, &subval);
               print_value(&subval, subtype);
               if (i+1 != t->num_elements)
                  printf(", ");
            }
            printf("]");
            break;
         }
         case adiak_list: {
            printf("{");
            for (int i = 0; i < adiak_num_subvals(t); i++) {
               adiak_value_t subval;
               adiak_datatype_t* subtype;
               adiak_get_subval(t, val, i, &subtype, &subval);
               print_value(&subval, subtype);
               if (i+1 != t->num_elements)
                  printf(", ");
            }
            printf("}");
            break;
         }
         case adiak_tuple: {
            printf("(");
            for (int i = 0; i < adiak_num_subvals(t); i++) {
               adiak_value_t subval;
               adiak_datatype_t* subtype;
               adiak_get_subval(t, val, i, &subtype, &subval);
               print_value(&subval, subtype);
               if (i+1 != t->num_elements)
                  printf(", ");
            }
            printf(")");
            break;
         }
      }
   }

   static void print_nameval(const char *name, int UNUSED(category), const char *UNUSED(subcategory), adiak_value_t *value, adiak_datatype_t *t, void *UNUSED(opaque_value))
   {
      printf("%s: ", name);
      print_value(value, t);
      printf("\n");
   }

   int main(int argc, char* argv[])
   {
      adiak_init(NULL);

      adiak_register_cb(1, adiak_category_all, print_nameval, NULL);

      adiak_executable();
      adiak_namevalue("countdown", adiak_general, NULL, "%lld", 9876543210);
      int ints[3] = { 1, 2, 3 };
      adiak_namevalue("ints", adiak_general, NULL, "{%d}", ints, 3);

      adiak_fini();
   }

API reference
-------------

.. doxygengroup:: ToolAPI
   :project: Adiak
   :members:
