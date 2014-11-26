/* gvs-serializer.c: GObject serializer
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
#include "gvs-serializer.h"

#include "gvs-gobject.h"
#undef __GVS_INSIDE__

struct _GvsSerializerPrivate
{
    GVariantBuilder *builder;
    GHashTable      *entity_map;
    GQueue           queue;
    gsize            num_entities;
};

#define GVS_ENTITY_TYPE            ((const GVariantType*) "(sv)")
#define GVS_ENTITY_REF_TYPE        G_VARIANT_TYPE_UINT64
#define GVS_ENTITY_ARRAY_TYPE      ((const GVariantType*) "a(sv)")
#define GVS_SERIALIZED_OBJECT_TYPE ("(uq@a(sv))")
#define GVS_MAGIC_NUMBER           ((guint32) 0x6776736F) /*'gvso'*/
#define GVS_PROTOCOL_VERSION       ((guint16) 1)

/******************************************************************************
 *
 * Entity handling functions
 *
 ******************************************************************************/

typedef struct
{
    gsize id;
    GValue value;
} EntityRef;

static EntityRef *
make_entity_ref(gsize id, const GValue *value)
{
    EntityRef *ref = g_slice_new(EntityRef);
    ref->id = id;
    g_value_init(&ref->value, G_VALUE_TYPE (value));
    g_value_copy(value, &ref->value);

    return ref;
}

static void
entity_ref_free(gpointer ptr)
{
    EntityRef *ref = ptr;
    g_value_reset(&ref->value);
    g_slice_free(EntityRef, ref);
}

static GVariant *get_entity_ref(GvsSerializer *self, const GValue *value);

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

static GVariant *
strv_serialize(GvsSerializer *self, const GValue *value, gpointer unused)
{
    const char * const * strv = g_value_get_boxed(value);
    return g_variant_new_strv(strv, g_strv_length((GStrv) strv));
}

static GVariant *
bytes_serialize(GvsSerializer *self, const GValue *value, gpointer unused)
{
    gsize size, i;
    const guint8 *data;
    GVariantBuilder builder;

    data = g_bytes_get_data(g_value_get_boxed(value), &size);

    /* Hmmm, g_variant_new_bytestring for some reason wants the incoming
     * data to be nul-terminated, which of course a generic chunk of data
     * in the form of bytes might not be. So we have to manually go through
     * and create the byte array ourselves. */
    g_variant_builder_init(&builder, G_VARIANT_TYPE_BYTESTRING);
    for (i = 0; i < size; i++)
    {
        g_variant_builder_add_value(&builder, g_variant_new_byte(data[i]));
    }

    return g_variant_builder_end(&builder);
}

typedef struct
{
    const char *gtype_name;
    const GVariantType *variant_type;
    GvsPropertySerializeFunc serialize;
} BuiltinTransform;

static const BuiltinTransform builtin_transforms[] = {
    { "GStrv",  G_VARIANT_TYPE_STRING_ARRAY, strv_serialize },
    { "GBytes", G_VARIANT_TYPE_BYTESTRING, bytes_serialize },
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

static GVariant *
serialize_boxed_property(GvsSerializer *self, const GValue *value, gpointer unused)
{
    gpointer object = g_value_get_boxed(value);
    GVariant *ref = NULL;

    if (object)
        ref = get_entity_ref(self, value);

    return g_variant_new_maybe(GVS_ENTITY_REF_TYPE, ref);
}

/*
 *
 * Fundemental type transformations
 *
 */
static GVariant *
serialize_fundamental(GvsSerializer *self, const GValue *value, gpointer unused)
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

    return variant;
}

/*
 * 
 * Flags and enums
 * 
 */
static GVariant *
serialize_enum(GvsSerializer *self, const GValue *value, gpointer unused)
{
    return g_variant_ref_sink(g_variant_new_int32(g_value_get_enum(value)));
}

static GVariant *
serialize_flags(GvsSerializer *self, const GValue *value, gpointer unused)
{
    return g_variant_ref_sink(g_variant_new_uint32(g_value_get_flags(value)));
}

/*
 *
 * Objects
 *
 */
static GVariant *
serialize_object_property(GvsSerializer *self, const GValue *value, gpointer unused)
{
    gpointer object = g_value_get_object(value);
    GVariant *ref = NULL;

    if (object)
    {
    	/* The passed GValue has the declared type of the property; we want to
     	 * record the type of the actual instance */
        GValue derived_value = G_VALUE_INIT;
        g_value_init (&derived_value, G_TYPE_FROM_INSTANCE (object));
        g_value_set_object (&derived_value, object);
        ref = get_entity_ref(self, &derived_value);
        g_value_reset (&derived_value);
    }

    return g_variant_new_maybe(GVS_ENTITY_REF_TYPE, ref);
}

/******************************************************************************
 *
 * Internal functions
 *
 ******************************************************************************/

static GVariant *
serialize_pspec(GvsSerializer *self, GParamSpec *pspec, const GValue *value)
{
    GvsPropertySerializeFunc func = NULL;
    GVariant *variant = NULL;
    GType type = pspec->value_type;

    /* Try to find the right serialization function */
    func = g_param_spec_get_qdata(pspec, gvs_property_serialize_func_quark());

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
        else if (G_TYPE_IS_OBJECT(type) || G_TYPE_IS_INTERFACE (type))
        {
            func = serialize_object_property;
        }
        else if (g_type_is_a(type, G_TYPE_BOXED))
        {
            func = serialize_boxed_property;
        }
    }

    if (func)
    {
        variant = func(self, value, NULL);
    }
    else
    {
        g_warning("Could not serialize property %s of type %s\n"
                  "Use gvs_register_property_serialize_func() in your class_init function\n",
                  pspec->name, g_type_name(pspec->value_type));
    }

    return variant;
}

static GVariant *
serialize_object_default(GvsSerializer *self, EntityRef *ref)
{
    GObjectClass *klass;
    guint n_props, i;
    GParamSpec **pspecs;
    GVariantBuilder builder;
    GObject *object;

    object = g_value_get_object(&ref->value);

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
                              serialize_pspec(self, pspec, &value));

        g_value_unset (&value);
    }

    g_free(pspecs);

    return g_variant_builder_end (&builder);
}

static GVariant *
serialize_boxed_default(GvsSerializer *self, EntityRef *ref)
{
    GVariant *variant = NULL;
    const BuiltinTransform *transform;

    transform = lookup_builtin_transform(G_VALUE_TYPE(&ref->value));

    if (g_value_peek_pointer(&ref->value) && transform)
    {
        variant = transform->serialize(self, &ref->value, NULL);
    }

    return variant;
}

static void
serialize_entity(GvsSerializer *self, EntityRef *ref)
{
    GvsSerializerPrivate *priv = self->priv;
    GType type = G_VALUE_TYPE(&ref->value);
    
    /* Now, create our new object. This is type "(sv)" */
    g_variant_builder_open(priv->builder, GVS_ENTITY_TYPE);
    
    /* First, add GType name */
    g_variant_builder_add(priv->builder, "s", g_type_name(type));

    /* Then add the serialized item itself */
    if (g_type_is_a(type, G_TYPE_OBJECT))
    {
        g_variant_builder_add(priv->builder, "v",
                              serialize_object_default(self, ref));
    }
    else if (g_type_is_a(type, G_TYPE_BOXED))
    {
        g_variant_builder_add(priv->builder, "v",
                              serialize_boxed_default(self, ref));
    }
    else
    {
        g_assert_not_reached();
    }

    /* Close the tuple we just opened */
    g_variant_builder_close(priv->builder);
}

static gsize
push_entity(GvsSerializer *self, const GValue *value)
{
    GvsSerializerPrivate *priv = self->priv;

    EntityRef *ref = make_entity_ref(priv->num_entities++, value);
    
    g_hash_table_insert(priv->entity_map, g_value_peek_pointer(value), ref);

    g_queue_push_head(&priv->queue, ref);

    return ref->id;
}

static EntityRef *
pop_entity(GvsSerializer *self)
{
    return g_queue_pop_tail(&self->priv->queue);
}

static GVariant *
get_entity_ref(GvsSerializer *self, const GValue *value)
{
    GvsSerializerPrivate *priv = self->priv;
    gsize entity_id = 0;
    gpointer ptr = g_value_peek_pointer(value);

    /* If we have this entity already, returns its reference */
    if (g_hash_table_contains(priv->entity_map, ptr))
    {
        EntityRef *ref = g_hash_table_lookup(priv->entity_map, ptr);
        entity_id = ref->id;
    }
    else
    {
        entity_id = push_entity(self, value);
    }

    return g_variant_new_uint64(entity_id);
}


/******************************************************************************
 *
 * Public API
 *
 ******************************************************************************/

/**
 * gvs_serializer_serialize_object:
 * @serializer: A #GvsSerializer
 * @object: (type GObject): A #GObject to serialize
 *
 * Returns: (transfer full): A new, non-floating #GVariant containing the
 * serialized state of @object. Free with g_variant_unref()
 */
GVariant *
gvs_serializer_serialize_object(GvsSerializer *self, GObject *object)
{
    GvsSerializerPrivate *priv;
    GVariant *variant = NULL;
    GVariant *array = NULL;
    GValue val = G_VALUE_INIT;

    g_return_val_if_fail(GVS_IS_SERIALIZER(self), NULL);
    g_return_val_if_fail(G_IS_OBJECT(object), NULL);

    priv = self->priv;

    priv->builder = g_variant_builder_new(GVS_ENTITY_ARRAY_TYPE);
    priv->entity_map = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                             NULL, entity_ref_free);
    g_queue_init(&priv->queue);

    g_value_init(&val, G_TYPE_FROM_INSTANCE(object));
    g_value_set_object(&val, object);

    push_entity(self, &val);

    {
        EntityRef *e = NULL;

        while ((e = pop_entity(self)) != NULL)
        {
            serialize_entity(self, e);
        }
    }

    array = g_variant_builder_end(priv->builder);

    variant = g_variant_new(GVS_SERIALIZED_OBJECT_TYPE,
                            GVS_MAGIC_NUMBER,
                            GVS_PROTOCOL_VERSION,
                            array);

    g_value_reset(&val);
    g_hash_table_destroy(priv->entity_map);
    g_variant_builder_unref(priv->builder);

    return variant;
}

/**
 * gvs_serializer_new:
 * 
 * Returns: (transfer full): A new #GvsSerializer, free with g_object_unref()
 */
GvsSerializer *
gvs_serializer_new(void)
{
    return g_object_new(GVS_TYPE_SERIALIZER, NULL);
}

/******************************************************************************
 *
 * GObject boilerplate
 *
 ******************************************************************************/

G_DEFINE_TYPE_WITH_PRIVATE(GvsSerializer, gvs_serializer, G_TYPE_OBJECT)

static void
gvs_serializer_class_init(GvsSerializerClass *klass)
{
}

static void
gvs_serializer_init (GvsSerializer *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, GVS_TYPE_SERIALIZER, GvsSerializerPrivate);
}
