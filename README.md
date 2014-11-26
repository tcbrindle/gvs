
GVS
===

GVS ("GObject Variant Serialization") is an attempt to provide a generic
serialization library for GObject. It serializes objects to GVariant format.
GVariant is a highly efficient serialization format which forms the basis
for GSettings and GDBus, and will in future be used for kdbus. GVariants can
be saved in binary or human-readable text form, or converted to JSON using
the JSON-Glib library.

How to use
----------

For most GObjects, serialization is as simple as calling

```C
gvs_gobject_serialize(object);
```

which will return a `GVariant*` which can then be saved to disk, or as a binary
blob in a database, or whatever else you want to do with it.

Going the other way, we can create a new object from serialized state by calling

```C
gvs_gobject_new_deserialize(variant);
```

which will return a new GObject instance. Alternatively, if you have created the
object yourself but just wish to restore a saved state, you can call

```C
gvs_gobject_deserialize(object, variant);
```

which will apply the saved state in `variant` to `object`.


Default serialization
---------------------

By default, GVS will serialize an object by serializing each of its publicly
readable and writable properties in turn. These are saved as a `GVariant` with
type

    (sa{sv})

where the first string contains the name of the object's GType, followed by
a dictionary containing `{property_name: variant}` pairs. The type of the 
"child" variant depends on the GType of the property.

###Fundamental properties

For basic properties of "fundamental" type, GVS will serialize to the 
appropriate GVariant type according to the following table:

C Type          | GType           | GVariant type 
--------------- | --------------- | -------------
`gboolean`      | `G_TYPE_BOOLEAN`| `b`
`int`           | `G_TYPE_INT`    | `i`
`unsigned`      | `G_TYPE_UINT`   | `u`
`int64_t`       | `G_TYPE_INT64`  | `x`
`uint64_t`      | `G_TYPE_UINT64` | `t`
`long`          | `G_TYPE_LONG`   | `x`
`unsigned long` | `G_TYPE_ULONG`  | `t`
`char`          | `G_TYPE_CHAR`   | `y`
`unsigned char` | `G_TYPE_UCHAR`  | `y`
`float`         | `G_TYPE_FLOAT`  | `d`
`double`        | `G_TYPE_DOUBLE` | `d`
`char*`         | `G_TYPE_STRING` | `ms`

(Note that string properties may hold a `NULL` value.)

###Enum and flags properties

For properties which of type `G_TYPE_ENUM` or `G_TYPE_FLAGS`, GVS will serialize
to GVariant types `i` and `u` (int32 and uint32) respectively.

###Object properties

For properties which are themselves objects (i.e. the property type is a subtype
of `G_TYPE_OBJECT`), GVS will recurively call `gvs_gobject_serialize`, and store
the variant as `maybe <object-type>` (allowing for the possibility of the
object being NULL). So if you're not using the `GSerializable` interface (see
below), object properties are themselves serialized to `m(sa{sv})`.

###Boxed properties of built-in type

GLib includes serveral boxed types which can be used as property types, for
example `GDateTime` and `GBytes`. GVS currently understands and will automatically
serialize the following built-in types:

C Type          | GType           | GVariant type 
--------------- | --------------- | -------------
`GBytes*`       | `G_TYPE_BYTES`  | `ay`
`GStrv`         | `G_TYPE_STRV`   | `as`

(This list is likely to be expanded in future.)


Specific Property Serialization
-------------------------------

For properties which are not one of the above types, you'll need to tell GVS
how to serialize and deserialize the property. (The same mechanism can also be
used to override the default serialization for any property if you wish to.)
This is done by registering a pair of functions with GVS (for serialization and
deserialization). The functions must have signatures:

```C
GVariant *my_serialization_func(const GValue *gvalue);

void my_deserialization_func(GVariant *variant, GValue *out_gvalue);
```

You register these functions by calling

```C
gvs_register_property_serialization_func(pspec, serialization_func);
gvs_register_property_deserialization_func(pspec, deserialization_func);
```

The best place to do this is in your `class_init()` function, right after
defining the property itself.

(TODO: Include example here.)


Custom Serialization -- using GvsSerializable
---------------------------------------------

Default serialization is appropriate for objects which can be completely
described and reconstructed using only their public properties. However, for
objects with internal state, customized serialization may be required. This
can be achieved by implementing methods from the `GvsSerializable` interface.

No methods from `GvsSerializable` are *required* to be implemented. That is, if
no overrides are provided, serialization will proceed exactly as per the default
process described above. However, by implementing GvsSerializable methods, you
can run custom code at various points in the [de-]serialization process.

Needless to say, if you override a serialization method, you must override the
corresponding deserialization method, or things will go badly wrong.

###Simple custom serialization

The easiest way to provide custom serialization is to override only the
`post_serialize()` virtual function. This will run the default serialization
(i.e. serializing each property), and then pass you the generated dictionary
so that you can add extra members.

(TODO: work out whether this is worth implementing as a signal too, to make
life easier for bindings.)

Correspondingly, when deserializing, you override the `pre_deserialize()`
function. 

###The serialization process

1. The serializer will call the `can_serialize()` virtual method. If this
   returns `FALSE`, serialization of the object is aborted. The default
   implementation simply returns `TRUE`, which is probably what you want.

2. The serializer will call `get_variant_type()`. This should return the
   `GVariantType` which will be generated. As described above, the default is
   `a{sv}`, a dictionary mapping property names to variants.

3. The serializer will call `serialize()`. This function can be overriden
   for complete control of serialization. It must return a `GVariant`
   of the type declared in step 2.

   The default implementation will call `serialize_property()` for each property
   of the object, in the order returned by `g_object_class_list_properties()`;
   this is typically in declaration order, from base class to most derived.

   The `serialize_property()` function can be overridden to provide special
   behaviour for certain properties, as an alternative to registering
   transformation functions as described above.

4. Finally, the default implementation of `serialize()` will call the
   `post_serialize()` virtual function, passing the variant which has been
   generated so far. This can be used for a simple way to add extra members to
   the default property dictionary.

###The deserialization process

Just to be confusing, there are actually two different deserialization
processes, one for objects which already exist, and one for creating new objects

For objects which already exist, the procedure is pretty simple:

* The deserialize() virtual method is called.

  If `deserialize()` has not been overridden, the default implementation will
  first call the `pre_deserialize()` virtual function, passing the object and
  the vardict. If you implemented `post_serialize()`, you should do the
  reverse here.

  Next, the default implementation will call the `deserialize_property()`
  virtual function for each object property (**note:** not just the ones for
  which there is an entry in the property dictionary.)

And that's it.

TODO: Describe the procedure for creating new instances.


