/* gvs-deserializer.h: GObject deserializer
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

#ifndef __GVS_DESERIALIZER_H__
#define __GVS_DESERIALIZER_H__

#if !defined (__GVS_INSIDE__)
#error "Only <gvs.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define GVS_TYPE_DESERIALIZER             (gvs_deserializer_get_type ())
#define GVS_DESERIALIZER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVS_TYPE_DESERIALIZER, GvsDeserializer))
#define GVS_DESERIALIZER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  GVS_TYPE_DESERIALIZER, GvsDeserializerClass))
#define GVS_IS_DESERIALIZER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVS_TYPE_DESERIALIZER))
#define GVS_IS_DESERIALIZER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  GVS_TYPE_DESERIALIZAER))
#define GVS_DESERIALIZER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GVS_TYPE_DESERIALIZER, GvsSerializerClass))

typedef struct _GvsDeserializer             GvsDeserializer;
typedef struct _GvsDeserializerClass        GvsDeserializerClass;
typedef struct _GvsDeserializerPrivate      GvsDeserializerPrivate;

struct _GvsDeserializer
{
    /*<private>*/
    GObject    parent;

    GvsDeserializerPrivate *priv;
};

struct _GvsDeserializerClass
{
    /*<private>*/
    GObjectClass    parent_class;
    
    /*<public>*/
    
};

/**
 * GvsPropertyDeserializeFunc:
 * @deserializer: The #GvsDeserializer being used to serialize this property
 * @variant: (in): Serialized GObject property to be reconstructed
 * @out_value: (out): The value of the property
 * @user_data: User data passed when the function was registered
 */
typedef void     (*GvsPropertyDeserializeFunc)    (GvsDeserializer *deserializer,
                                                   GVariant        *variant,
                                                   GValue          *out_value,
                                                   gpointer         user_data);

GType             gvs_deserializer_get_type       (void) G_GNUC_CONST;

GvsDeserializer  *gvs_deserializer_new            (void);

gpointer          gvs_deserializer_deserialize    (GvsDeserializer *deserializer,
                                                   GVariant        *variant);

G_END_DECLS

#endif
