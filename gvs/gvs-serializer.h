/* gvs-serializable.h: Interface for custom serialization of GObjects
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

#ifndef __GVS_SERIALIZER_H__
#define __GVS_SERIALIZER_H__

#if !defined (__GVS_INSIDE__)
#error "Only <gvs.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define GVS_TYPE_SERIALIZER             (gvs_serializer_get_type ())
#define GVS_SERIALIZER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVS_TYPE_SERIALIZER, GvsSerializer))
#define GVS_SERIALIZER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  GVS_TYPE_SERIALIZER, GvsSerializerClass))
#define GVS_IS_SERIALIZER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVS_TYPE_SERIALIZER))
#define GVS_IS_SERIALIZER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  GVS_TYPE_ADAPTER))
#define GVS_SERIALIZER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GVS_TYPE_SERIALIZER, GvsSerializerClass))

typedef struct _GvsSerializer             GvsSerializer;
typedef struct _GvsSerializerClass        GvsSerializerClass;
typedef struct _GvsSerializerPrivate      GvsSerializerPrivate;

struct _GvsSerializer
{
    /*<private>*/
    GObject    parent;

    GvsSerializerPrivate *priv;
};

struct _GvsSerializerClass
{
    /*<private>*/
    GObjectClass    parent_class;
    
    /*<public>*/
    
};

/**
 * GvsPropertySerializeFunc:
 * @serializer: The #GvsSerializer being used to serialize this property
 * @value: (in): The value of the property
 * @user_data: User data passed when the function was registered
 */
typedef GVariant *(*GvsPropertySerializeFunc)     (GvsSerializer *serializer,
                                                   const GValue *value,
                                                   gpointer user_data);

GType             gvs_serializer_get_type         (void) G_GNUC_CONST;

GvsSerializer    *gvs_serializer_new              (void);

GVariant         *gvs_serializer_serialize_object (GvsSerializer *serializer,
                                                   GObject       *object);



G_END_DECLS

#endif
