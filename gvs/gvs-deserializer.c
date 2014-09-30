/* gvs-deserializer.c: GObject serializer
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


#define __GVS_INSIDE__
#include "gvs-deserializer.h"

#include "gvs-gobject.h"
#undef __GVS_INSIDE__

struct _GvsDeserializerPrivate
{
    GVariant  *toplevel;
    gpointer  *entities;
};

#define GVS_ENTITY_TYPE            ((const GVariantType*) "(sv)")
#define GVS_ENTITY_REF_TYPE        ((const GVariantType*) "u")
#define GVS_ENTITY_ARRAY_TYPE      ((const GVariantType*) "a(sv)")
#define GVS_SERIALIZED_OBJECT_TYPE ((const GVariantType*) "(uqa(sv))")
#define GVS_MAGIC_NUMBER           ((guint32) 0x6776736F) /*'gvso'*/
#define GVS_PROTOCOL_VERSION       ((guint16) 1)

static gpointer get_entity(GvsDeserializer *self, gsize id);

/******************************************************************************
 *
 * Entity handling functions
 *
 ******************************************************************************/


/******************************************************************************
 *
 * Internal GvsPropertySerializeFuncs for known types
 *
 ******************************************************************************/

/*
 *
 * Built-in boxed transformations
 *
 */

static void
strv_deserialize(GvsDeserializer *self, GVariant *variant, GValue *value, gpointer unused)
{
    g_value_set_boxed(value, g_variant_get_strv(variant, NULL));
}

static void
gbytes_deserialize(GvsDeserializer *self, GVariant *variant, GValue *value, gpointer unused)
{
    gsize length;
    char *data = g_variant_dup_bytestring(variant, &length);
    GBytes *bytes = g_bytes_new_take(data, length);
    g_value_set_boxed(value, bytes);
}


typedef struct
{
    const char *gtype_name;
    GvsPropertyDeserializeFunc deserialize;
} BuiltinTransform;

static const BuiltinTransform builtin_transforms[] = {
    { "GStrv",  strv_deserialize },
    { "GBytes", gbytes_deserialize },
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
 * Fundemental type transformations
 *
 */
static void
deserialize_fundamental(GvsDeserializer *self, GVariant *variant, GValue *value, gpointer unused)
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
static void
deserialize_enum(GvsDeserializer *self, GVariant *variant, GValue *value, gpointer unused)
{
    g_value_set_enum(value, g_variant_get_int32(variant));
}

static void
deserialize_flags(GvsDeserializer *self, GVariant *variant, GValue *value, gpointer unused)
{
    g_value_set_flags(value, g_variant_get_uint32(variant));
}



/*
 *
 * Objects
 *
 */
static void
deserialize_object(GvsDeserializer *self, GVariant *variant, GValue *value, gpointer unused)
{
    GVariant *child = g_variant_get_maybe(variant);

    if (child)
    {
        gsize child_id = g_variant_get_uint64(child);
        g_value_take_object(value, get_entity(self, child_id));
    }
    else
    {
        g_value_set_object(value, NULL);
    }
}



/******************************************************************************
 *
 * Internal functions
 *
 ******************************************************************************/

static void
deserialize_pspec(GvsDeserializer *self, GParamSpec *pspec, GVariant *variant, GValue *value)
{
    GvsPropertyDeserializeFunc func = NULL;
    GType type = pspec->value_type;

    /* Try to find the right deserialization function */
    func = g_param_spec_get_qdata(pspec, gvs_property_deserialize_func_quark());

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
        else if (G_TYPE_IS_INTERFACE(type))
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
        func(self, variant, value, NULL);
    }
    else
    {
        g_warning("Could not deserialize property %s of type %s\n"
                  "Use gvs_register_property_deserialize_func() in your class_init function\n",
                  pspec->name, g_type_name(pspec->value_type));
    }
}


static void
gvs_deserialize_object_default(GvsDeserializer *self,
                               GObject         *object,
                               GVariant        *variant)
{
    guint n_params, i;
    GObjectClass *gobject_class = NULL;

    gobject_class = G_OBJECT_GET_CLASS(object);
    g_assert(G_IS_OBJECT_CLASS(gobject_class));
    
    n_params = g_variant_n_children(variant);
    
    /* Pretty simple: for every entry in the properties dict, if the
     * entry is a property which is suitable to set, set it */
    for (i = 0; i < n_params; i++)
    {
        char *prop_name;
        GVariant *prop_var;
        GParamSpec *pspec = NULL;
        GValue value = G_VALUE_INIT;

        g_variant_get_child(variant, i, "{sv}", &prop_name, &prop_var);
        g_assert(prop_name);
        g_assert(prop_var);

        pspec = g_object_class_find_property(gobject_class, prop_name);
        g_assert(pspec);

        if ((pspec->flags & G_PARAM_CONSTRUCT_ONLY) == 0)
        {
            deserialize_pspec(self, pspec, prop_var, &value);

            g_object_set_property(object, prop_name, &value);
        }

        g_variant_unref(prop_var);
        g_free(prop_name);
    }
}


static gpointer
gvs_create_object_default(GvsDeserializer *self, GType class_type, GVariant *variant)
{
    guint n_params, i;
    GObjectClass *gobject_class = NULL;
    gpointer object = NULL;
    GParamSpec **pspecs;
    
    GArray *params = g_array_new(FALSE, TRUE, sizeof(GParameter));

    gobject_class = G_OBJECT_CLASS(g_type_class_ref(class_type));
    g_assert(gobject_class);
    
    pspecs = g_object_class_list_properties(gobject_class, &n_params);
    for (i=0; i < n_params; i++)
    {
        GParamSpec *pspec = pspecs[i];
        GVariant *pvariant = NULL;
        GParameter param = { 0, };

        /* Only handle construct-only properties here*/
        if ((pspec->flags & G_PARAM_CONSTRUCT_ONLY) == 0)
            continue;

        pvariant = g_variant_lookup_value(variant,
                                          g_param_spec_get_name(pspec),
                                          NULL);

        if (!pvariant)
            continue;

        param.name = g_param_spec_get_name(pspec);
        deserialize_pspec(self, pspec, pvariant, &param.value);

        g_array_append_val(params, param);

        g_variant_unref(pvariant);
    }

    object = g_object_newv(class_type,
                           params->len,
                           (GParameter *) params->data);

    g_free(pspecs);
    g_array_free(params, TRUE);
    g_type_class_unref(gobject_class);

    return object;
}

static void
deserialize_entity(GvsDeserializer *self, gsize index)
{
    GvsDeserializerPrivate *priv = self->priv;
    GVariant *child;
    gpointer object = priv->entities[index];

    g_assert(object);

    /* Grab the nth entry from the toplevel */
    g_variant_get_child(priv->toplevel, index, "(sv)", NULL, &child);

    gvs_deserialize_object_default(self, object, child);

    g_variant_unref(child);
}

static gpointer
create_entity(GvsDeserializer *self, gsize index)
{
    GvsDeserializerPrivate *priv = self->priv;
    char *gtype_str;
    GType gtype;
    GVariant *child;
    gpointer object = NULL;

    /* Grab the nth entry from the toplevel */
    g_variant_get_child(priv->toplevel, index, "(sv)", &gtype_str, &child);

    if (gtype_str == NULL)
    {
        g_critical("Could not read GType string from serialized object");
        goto out;
    }

    gtype = g_type_from_name(gtype_str);

    if (gtype == 0)
    {
        g_critical("Type name \"%s\" is not registered with GType", gtype_str);
        goto out;
    }

    g_assert(g_type_is_a(gtype, G_TYPE_OBJECT));

    /* TODO: Handle other entity types here, and GvsSerializable etc */
    object = gvs_create_object_default(self, gtype, child);

    g_assert(object);

    priv->entities[index] = object;

out:
    g_free(gtype_str);
    g_variant_unref(child);

    return object;
}


static gpointer
get_entity(GvsDeserializer *self, gsize index)
{
    gpointer entity = self->priv->entities[index];

    if (!entity)
    {
        entity = create_entity(self, index);
    }

    return entity;
}


/******************************************************************************
 *
 * Public API
 *
 ******************************************************************************/

/**
 * gvs_deserializer_deserialize:
 * @deserializer: A #GvsDeserializer
 * @variant: A #GVariant containing a GObject serialization
 * 
 * Returns: (type GObject) (transfer full): A new #GObject created from the
 *  serialized state. Free with g_object_unref()
 */
gpointer
gvs_deserializer_deserialize(GvsDeserializer *self, GVariant *variant)
{
    GvsDeserializerPrivate *priv = self->priv;
    gsize n_entities, i;
    gpointer object;
    guint32 magic_number;
    guint16 protocol_version;
    
    g_return_val_if_fail(GVS_IS_DESERIALIZER(self), NULL);
    g_return_val_if_fail(g_variant_is_of_type(variant, GVS_SERIALIZED_OBJECT_TYPE), NULL);

    /* Check magic number is correct */
    g_variant_get_child(variant, 0, "u", &magic_number);
    g_return_val_if_fail(magic_number == GVS_MAGIC_NUMBER, NULL);

    /* Check the protocol version (currently must be 1) */
    g_variant_get_child(variant, 1, "q", &protocol_version);
    if (protocol_version != 1)
    {
        g_critical("This version of libgvs cannot deserialize GVS protocol version %i\n",
                   protocol_version);
        return NULL;
    }

    /* Go ahead and start unpacking the array */
    priv->toplevel = g_variant_get_child_value(variant, 2);
    n_entities = g_variant_n_children(priv->toplevel);
    priv->entities = g_new0(gpointer, n_entities);

    /* We do deserialization in two stages.*/
    
    /* First, create all the entities */
    for (i = 0; i < n_entities; i++)
    {
        create_entity(self, i);
    }

    /* Now, do proper deserialization */
    for (i = 0; i < n_entities; i++)
    {
        deserialize_entity(self, i);
    }

    object = priv->entities[0];

    g_variant_unref(priv->toplevel);
    g_free(priv->entities);

    return object;
}

/**
 * gvs_deserializer_new:
 * 
 * Returns: (transfer full): A new #GvsDeserializer, free with g_object_unref()
 */
GvsDeserializer *
gvs_deserializer_new(void)
{
    return g_object_new(GVS_TYPE_DESERIALIZER, NULL);
}

/******************************************************************************
 *
 * GObject boilerplate
 *
 ******************************************************************************/

G_DEFINE_TYPE_WITH_PRIVATE(GvsDeserializer, gvs_deserializer, G_TYPE_OBJECT)

static void
gvs_deserializer_class_init(GvsDeserializerClass *klass)
{
}

static void
gvs_deserializer_init (GvsDeserializer *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GVS_TYPE_DESERIALIZER, GvsDeserializerPrivate);
}
