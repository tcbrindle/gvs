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

/******************************************************************************
 *
 * Entity handling functions
 *
 ******************************************************************************/

typedef struct
{
    GType type;
    gsize id;
} EntityRef;

static EntityRef *
entity_ref(GType type, gsize id)
{
    EntityRef *ref = g_slice_new(EntityRef);
    ref->type = type;
    ref->id = id;
    return ref;
}

static void
entity_ref_free(gpointer ref)
{
    g_slice_free(EntityRef, ref);
}

static GVariant *get_entity_ref(GvsSerializer *self, gpointer entity, GType entity_type);

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
    return g_variant_new_strv(g_value_get_boxed(value), -1);
}

typedef struct
{
    const char *gtype_name;
    GvsPropertySerializeFunc serialize;
} BuiltinTransform;

static const BuiltinTransform builtin_transforms[] = {
    { "GStrv",  strv_serialize },
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

      return g_variant_ref_sink(variant);
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
serialize_object(GvsSerializer *self, const GValue *value, gpointer unused)
{
    gpointer object = g_value_get_object(value);
    GVariant *ref = NULL;

    if (object)
        ref = get_entity_ref(self, object, G_VALUE_TYPE(value));

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
serialize_object_default(GvsSerializer *self, GObject *object)
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
                              serialize_pspec(self, pspec, &value));

        g_value_unset (&value);
    }

    g_free(pspecs);

    return g_variant_builder_end (&builder);
}

static void
serialize_entity(GvsSerializer *self, gpointer entity)
{
    GvsSerializerPrivate *priv = self->priv;

    GObject *object = G_OBJECT(entity);
    
    /* Now, create our new object. This is type "(sv)" */
    g_variant_builder_open(priv->builder, GVS_ENTITY_TYPE);
    
    /* First, add GType name */
    g_variant_builder_add(priv->builder, "s",
                          g_type_name (G_TYPE_FROM_INSTANCE (object)));

    /* Then add the serialized object itself */
    /* TODO: Handle GvsSerializable here */
    g_variant_builder_add(priv->builder, "v",
                          serialize_object_default(self, object));

    /* Close the tuple we just opened */
    g_variant_builder_close(priv->builder);
}

static gsize
push_entity(GvsSerializer *self, gpointer entity, GType entity_type)
{
    GvsSerializerPrivate *priv = self->priv;

    EntityRef *ref = entity_ref(entity_type, priv->num_entities++);
    
    g_hash_table_insert(priv->entity_map, entity, ref);

    g_queue_push_head(&priv->queue, entity);

    return ref->id;
}

static gpointer
pop_entity(GvsSerializer *self)
{
    return g_queue_pop_tail(&self->priv->queue);
}

static GVariant *
get_entity_ref(GvsSerializer *self, gpointer entity, GType entity_type)
{
    GvsSerializerPrivate *priv = self->priv;
    gsize entity_id = 0;

    /* If we have this entity already, returns its reference */
    if (g_hash_table_contains(priv->entity_map, entity))
    {
        EntityRef *ref = g_hash_table_lookup(priv->entity_map, entity);
        entity_id = ref->id;
    }
    else
    {
        entity_id = push_entity(self, entity, entity_type);
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

    g_return_val_if_fail(GVS_IS_SERIALIZER(self), NULL);
    g_return_val_if_fail(G_IS_OBJECT(object), NULL);

    priv = self->priv;


    priv->builder = g_variant_builder_new(GVS_ENTITY_ARRAY_TYPE);
    priv->entity_map = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                             NULL, entity_ref_free);
    g_queue_init(&priv->queue);

    push_entity(self, object, G_TYPE_FROM_INSTANCE(object));

    {
        gpointer e = NULL;

        while ((e = pop_entity(self)) != NULL)
        {
            serialize_entity(self, e);
        }
    }

    variant = g_variant_builder_end(priv->builder);
    g_variant_ref_sink(variant);

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
