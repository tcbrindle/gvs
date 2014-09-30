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

#define __GVS_INSIDE__
#include "gvs-gobject.h"
#undef __GVS_INSIDE__

G_DEFINE_QUARK("gvs-property-serialize-func-quark", gvs_property_serialize_func);

G_DEFINE_QUARK("gvs-property-deserialize-func-quark", gvs_property_deserialize_func);

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
    g_param_spec_set_qdata_full(pspec, gvs_property_serialize_func_quark(),
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
    g_param_spec_set_qdata_full(pspec, gvs_property_deserialize_func_quark(),
                                user_data, destroy_notify);
}


/**
 * gvs_gobject_serialize:
 * @object: A #GObject to serialize
 *
 * Returns: (transfer full): A non-floating #GVariant holding the serialized
 *  state of @object
 */
GVariant *
gvs_gobject_serialize(GObject *object)
{
    GvsSerializer *serializer;
    GVariant *variant;

    serializer = gvs_serializer_new();

    variant = gvs_serializer_serialize_object(serializer, object);

    g_object_unref(serializer);

    return variant;
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
    gpointer object = NULL;
    GvsDeserializer *deserializer;

    deserializer = gvs_deserializer_new();

    object = gvs_deserializer_deserialize(deserializer, variant);

    g_object_unref(deserializer);
    
    return object;
}

