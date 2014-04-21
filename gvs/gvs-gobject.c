/* gvs-gobject.h: Default serialization of GObjects
 *
 * Copyright (c) 2014 Tristan Brindle <t.c.brindle@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 *
 */

#define __GVS_INSIDE__
#include "gvs-gobject.h"
#undef __GVS_INSIDE__

static GQuark
serialize_func_quark(void)
{
    return g_quark_from_static_string("gvs-property-serialize_func");
}

static GQuark
deserialize_func_quark(void)
{
    return g_quark_from_static_string("gvs-property-deserialize-func");
}


/*
 * 
 * GLib built-in boxed types
 * 
 */

static GVariant *
strv_serialize(const GValue *value)
{
    return g_variant_new_strv(g_value_get_boxed(value), -1);
}

static void
strv_deserialize(GVariant *variant, GValue *value)
{
    g_value_set_boxed(value, g_variant_get_strv(variant, NULL));
}

static GVariant *
gbytes_serialize(const GValue *value)
{
    gsize length;
    GBytes *bytes = g_value_get_boxed(value);
    gconstpointer data = g_bytes_get_data(bytes, &length);
    return g_variant_new_bytestring(data);
}

static void
gbytes_deserialize(GVariant *variant, GValue *value)
{
    gsize length;
    char *data = g_variant_dup_bytestring(variant, &length);
    GBytes *bytes = g_bytes_new_take(data, length);
    g_value_set_boxed(value, bytes);
}

typedef struct
{
    const char *gtype_name;
    GvsPropertySerializeFunc serialize;
    GvsPropertyDeserializeFunc deserialize;
} BuiltinTransform;

static const BuiltinTransform builtin_transforms[] = {
    { "GBytes", gbytes_serialize, gbytes_deserialize },
    { "GStrv",  strv_serialize, strv_deserialize },
    { NULL, }
};

static const BuiltinTransform *
lookup_builtin_transform(GType type)
{
    const char *name = g_type_name(type);
    const BuiltinTransform *t = NULL;

    for (t = builtin_transforms; t->gtype_name != NULL; t++) {
        if (g_str_equal(t->gtype_name, name))
            return t;
    }

    return NULL;
}



/*
 *
 * Fundamental types
 *
 */
static GVariant *
serialize_fundamental(const GValue *value)
{
    GVariant *variant;

    switch (G_VALUE_TYPE(value))
    {
        case G_TYPE_BOOLEAN:
            variant = g_variant_new_boolean(g_value_get_boolean(value));
            break;
        case G_TYPE_CHAR:
            variant = g_variant_new_byte(g_value_get_schar(value));
            break;
        case G_TYPE_DOUBLE:
            variant = g_variant_new_double(g_value_get_double(value));
            break;
        case G_TYPE_FLOAT:
            variant = g_variant_new_double(g_value_get_float(value));
            break;
        case G_TYPE_INT:
            variant = g_variant_new_int32(g_value_get_int(value));
            break;
        case G_TYPE_INT64:
        case G_TYPE_LONG:
            variant = g_variant_new_int64(g_value_get_int64(value));
            break;
        case G_TYPE_STRING:
            variant = g_variant_new("ms", g_value_get_string(value));
            break;
        case G_TYPE_UCHAR:
            variant = g_variant_new_byte(g_value_get_uchar(value));
            break;
        case G_TYPE_UINT:
            variant = g_variant_new_uint32(g_value_get_uint(value));
            break;
        case G_TYPE_UINT64:
        case G_TYPE_ULONG:
            variant = g_variant_new_uint64(g_value_get_uint64(value));
            break;
        case G_TYPE_VARIANT:
            variant = g_value_dup_variant(value);
            break;
        default:
            g_assert_not_reached();
            break;
      }

      return g_variant_ref_sink(variant);
}

static void
deserialize_fundamental(GVariant *variant, GValue *value)
{
    switch (G_VALUE_TYPE(value))
    {
        case G_TYPE_BOOLEAN:
            g_value_set_boolean(value, g_variant_get_boolean(variant));
            break;
        case G_TYPE_CHAR:
            g_value_set_schar(value, g_variant_get_byte(variant));
            break;
        case G_TYPE_DOUBLE:
            g_value_set_double(value, g_variant_get_double(variant));
            break;
        case G_TYPE_FLOAT:
            g_value_set_float(value, (float) g_variant_get_double(variant));
            break;
        case G_TYPE_INT:
            g_value_set_int(value, g_variant_get_int32(variant));
            break;
        case G_TYPE_INT64:
            g_value_set_int64(value, g_variant_get_int64(variant));
            break;
        case G_TYPE_LONG:
            g_value_set_long(value, g_variant_get_int64(variant));
            break;
        case G_TYPE_STRING:
        {
            char *str = NULL;
            g_variant_get(variant, "ms", &str);
            g_value_set_string(value, str);
            break;
        }
        case G_TYPE_UCHAR:
            g_value_set_uchar(value, g_variant_get_byte(variant));
            break;
        case G_TYPE_UINT:
            g_value_set_uint(value, g_variant_get_uint32(variant));
            break;
        case G_TYPE_UINT64:
            g_value_set_uint64(value, g_variant_get_uint64(variant));
            break;
        case G_TYPE_ULONG:
            g_value_set_ulong(value, g_variant_get_uint64(variant));
            break;
        case G_TYPE_VARIANT:
            g_value_take_variant(value, g_variant_get_variant(variant));
            break;
        default:
            g_assert_not_reached();
            break;
      }
}

/*
 * 
 * Flags and enums
 * 
 */
static GVariant *
serialize_enum(const GValue *value)
{
    return g_variant_ref_sink(g_variant_new_int32(g_value_get_enum(value)));
}

static void
deserialize_enum(GVariant *variant, GValue *value)
{
    g_value_set_enum(value, g_variant_get_int32(variant));
}

static GVariant *
serialize_flags(const GValue *value)
{
    return g_variant_ref_sink(g_variant_new_uint32(g_value_get_flags(value)));
}

static void
deserialize_flags(GVariant *variant, GValue *value)
{
    g_value_set_flags(value, g_variant_get_uint32(variant));
}

/*
 *
 * Objects
 *
 */
static const GVariantType *
get_variant_type_for_object_type(GType type)
{
    return (const GVariantType *) "(sa{sv})";
}

static GVariant *
serialize_object(const GValue *value)
{
    gpointer object = g_value_get_object(value);
    return g_variant_new_maybe(get_variant_type_for_object_type(G_VALUE_TYPE(value)),
                               object ? gvs_gobject_serialize(object)
                                      : NULL);
}

static void
deserialize_object(GVariant *variant, GValue *value)
{
    GVariant *child = g_variant_get_maybe(variant);

    if (child)
        g_value_take_object(value, gvs_gobject_new_deserialize(child));
    else
        g_value_set_object(value, NULL);
}


static GVariant *
serialize_pspec(GParamSpec *pspec, const GValue *value)
{
    GvsPropertySerializeFunc func = NULL;
    GVariant *variant = NULL;
    GType type = pspec->value_type;

    /* Try to find the right serialization function */
    func = g_param_spec_get_qdata(pspec, serialize_func_quark());

    if (func == NULL)
    {
        if (G_TYPE_IS_FUNDAMENTAL(type))
        {
            func = serialize_fundamental;
        }
        else if (G_TYPE_IS_ENUM(type))
        {
            func = serialize_enum;
        }
        else if (G_TYPE_IS_FLAGS(type))
        {
            func = serialize_flags;
        }
        else if (G_TYPE_IS_OBJECT(type))
        {
            func = serialize_object;
        }
        else
        {
            func = lookup_builtin_transform(type)->serialize;
        }
    }

    if (func)
    {
        variant = func(value);
    }
    else
    {
        g_warning("Could not serialize property %s of type %s\n"
                  "Use gvs_register_property_serialize_func() in your class_init function\n",
                  pspec->name, g_type_name(pspec->value_type));
    }

    return variant;
}

static void
deserialize_pspec(GParamSpec *pspec, GVariant *variant, GValue *value)
{
    GvsPropertyDeserializeFunc func = NULL;
    GType type = pspec->value_type;

    /* Try to find the right deserialization function */
    func = g_param_spec_get_qdata(pspec, deserialize_func_quark());

    if (func == NULL)
    {
        if (G_TYPE_IS_FUNDAMENTAL(type))
        {
            func = deserialize_fundamental;
        }
        else if (G_TYPE_IS_ENUM(type))
        {
            func = deserialize_enum;
        }
        else if (G_TYPE_IS_FLAGS(type))
        {
            func = deserialize_flags;
        }
        else if (G_TYPE_IS_OBJECT(type))
        {
            func = deserialize_object;
        }
        else
        {
            func = lookup_builtin_transform(type)->deserialize;
        }
    }

    if (func)
    {
        g_value_init(value, type);
        func(variant, value);
    }
    else
    {
        g_warning("Could not deserialize property %s of type %s\n"
                  "Use gvs_register_property_deserialize_func() in your class_init function\n",
                  pspec->name, g_type_name(pspec->value_type));
    }
}

static GVariant *
gobject_serialize_default(GObject *object)
{
    GObjectClass *klass;
    guint n_props, i;
    GParamSpec **pspecs;
    GVariantBuilder builder;

    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

    klass = G_OBJECT_GET_CLASS(object);
    
    pspecs = g_object_class_list_properties(klass, &n_props);
    
    for (i = 0; i < n_props; i++)
    {
        GValue value = G_VALUE_INIT;
        GParamSpec *pspec = pspecs[i];

        /* Skip read-only properties which we can't deserialize, and write-only
         * properties which are just stupid */
        if ((pspec->flags & G_PARAM_READABLE) == 0 ||
            (pspec->flags & G_PARAM_WRITABLE) == 0)
        {
            continue;
        }

        g_value_init(&value, pspec->value_type);

        g_object_get_property(object, pspec->name, &value);

        g_variant_builder_add(&builder, "{sv}", pspec->name,
                              serialize_pspec(pspec, &value));

        g_value_unset (&value);
    }

    g_free(pspecs);

    return g_variant_builder_end (&builder);
}

static void
gobject_deserialize_default(GObject *object, GVariant *variant)
{

}

static gpointer
gobject_new_deserialize_default(GType class_type, GVariant *variant)
{
    guint n_params, i;
    GParameter *params;
    GObjectClass *gobject_class = NULL;
    gpointer object = NULL;

    gobject_class = G_OBJECT_CLASS(g_type_class_ref(class_type));
    g_assert(gobject_class);
    
    n_params = g_variant_n_children(variant);
    params = g_new0(GParameter, n_params);

    for (i = 0; i < n_params; i++)
    {
        GParameter *param = &params[i];
        GVariant *prop_var = NULL;
        GParamSpec *pspec = NULL;

        g_variant_get_child(variant, i, "{sv}", &param->name, &prop_var);
        g_assert(prop_var);

        pspec = g_object_class_find_property(gobject_class, param->name);
        g_assert(pspec);

        deserialize_pspec(pspec, prop_var, &param->value);
        
        g_variant_unref(prop_var);
    }

    object = g_object_newv(class_type, n_params, params);

    g_free(params);
    g_type_class_unref(gobject_class);

    return object;
}

/**
 * gvs_register_property_serialize_func: (skip)
 */
void
gvs_register_propery_serialize_func(GParamSpec *pspec,
                                    GvsPropertySerializeFunc serialize)
{
    gvs_register_property_serialize_func_full(pspec, serialize, NULL, NULL);
}

/**
 * gvs_register_property_serialize_func_full: (rename-to gvs_register_property_serialize_func)
 */
void
gvs_register_property_serialize_func_full(GParamSpec *pspec,
                                          GvsPropertySerializeFunc serialize,
                                          gpointer user_data,
                                          GDestroyNotify destroy_notify)
{
    g_param_spec_set_qdata_full(pspec, serialize_func_quark(),
                                user_data, destroy_notify);
}

/**
 * gvs_register_property_deserialize_func: (skip)
 * @pspec:
 * @deserialize:
 */
void
gvs_register_property_deserialize_func(GParamSpec *pspec,
                                       GvsPropertyDeserializeFunc deserialize)
{
    gvs_register_property_deserialize_func_full(pspec, deserialize, NULL, NULL);
}

/**
 * gvs_register_property_deserialize_func_full: (rename-to gvs_register_property_deserialize_func)
 *
 */
void
gvs_register_property_deserialize_func_full(GParamSpec *pspec,
                                            GvsPropertyDeserializeFunc deserialize,
                                            gpointer user_data,
                                            GDestroyNotify destroy_notify)
{
    g_param_spec_set_qdata_full(pspec, deserialize_func_quark(),
                                user_data, destroy_notify);
}

/**
 * gvs_gobject_serialize_property:
 * @object: A #GObject
 * @property_name: The name of a property of @object to serialize
 * 
 * Returns: (transfer full): A non-floating #GVariant holding the serialized
 *  state of @object:@property_name
 */
GVariant *
gvs_gobject_serialize_property(GObject *object,
                               const char *property_name)
{
    GObjectClass *klass;
    GParamSpec *pspec;
    GValue value = G_VALUE_INIT;
    GVariant *variant;
    
    g_return_val_if_fail(G_IS_OBJECT(object), NULL);
    g_return_val_if_fail(property_name != NULL, NULL);
    
    klass = G_OBJECT_GET_CLASS(object);
    g_assert(klass);

    pspec = g_object_class_find_property(klass, property_name);
    g_return_val_if_fail(pspec != NULL, NULL);

    g_value_init(&value, pspec->value_type);
    g_object_get_property(object, property_name, &value);

    variant = serialize_pspec(pspec, &value);

    g_value_unset(&value);

    return variant;
}

/**
 * gvs_gobject_deserialize_property:
 * @object: A #GObject
 * @property_name: The name of a property of @object to set from @variant
 * @variant: A #GVariant holding a serialization of @object::@property_name
 * 
 */
void
gvs_gobject_deserialize_property(GObject *object,
                                 const char *property_name,
                                 GVariant *variant)
{
    GObjectClass *klass;
    GParamSpec *pspec;
    GValue value = G_VALUE_INIT;
    
    g_return_if_fail(G_IS_OBJECT(object));
    g_return_if_fail(property_name != NULL);
    g_return_if_fail(variant != NULL);
    
    klass = G_OBJECT_GET_CLASS(object);
    g_assert(klass);

    pspec = g_object_class_find_property(klass, property_name);
    g_return_val_if_fail(pspec != NULL, NULL);

    deserialize_pspec(pspec, variant, &value);
    g_object_set_property(object, property_name, &value);

    g_value_unset(&value);
}

/**
 * gvs_gobject_serialize:
 * @object: A #GObject to serialize
 *
 * Returns: (transfer full): A non-floating #GVariant holding the serialized\
 *  state of @object
 */
GVariant *
gvs_gobject_serialize(GObject *object)
{
    GVariantBuilder builder;

    g_return_val_if_fail(object != NULL, NULL);

    g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);

    /* First, add GType name */
    g_variant_builder_add(&builder, "s", g_type_name (G_TYPE_FROM_INSTANCE (object)));

    g_variant_builder_add_value(&builder,
                                gobject_serialize_default(object));

    return g_variant_builder_end(&builder);
}

/**
 * gvs_gobject_deserialize:
 * @object: A #GObject
 * @variant: A #GVariant holding a serialization of @object
 * 
 */
void
gvs_gobject_deserialize(GObject *object,
                        GVariant *variant)
{
    GVariant *child;

    child = g_variant_get_child_value(variant, 1);

    gobject_deserialize_default(object, child);
}


/**
 * gvs_gobject_new_deserialize:
 * @variant: A #GVariant containing a serialized #GObject
 * 
 * Returns: (transfer full) (type GObject): A newly constructed #GObject
 */
gpointer
gvs_gobject_new_deserialize(GVariant *variant)
{
    char *gtype_str = NULL;
    gpointer object = NULL;
    GType gtype = 0;
    GVariant *child;

    g_variant_get_child(variant, 0, "s", &gtype_str);

    if (gtype_str == NULL)
        goto out;

    gtype = g_type_from_name(gtype_str);

    if (gtype == 0)
    {
        g_critical("Type name \"%s\" is not registered with GType", gtype_str);
        goto out;
    }

    child = g_variant_get_child_value(variant, 1);

    object = gobject_new_deserialize_default(gtype, child);

out:
    return object;
}

