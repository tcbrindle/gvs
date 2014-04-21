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

#ifndef __GVS_SERIALIZABLE_H__
#define __GVS_SERIALIZABLE_H__

#if !defined (__GVS_INSIDE__)
#error "Only <gvs.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define GVS_TYPE_SERIALIZABLE           (gvs_serializable_get_type ())
#define GVS_SERIALIZABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVS_TYPE_SERIALIZABLE, GvsSerializable))
#define GVS_IS_SERIALIZABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVS_TYPE_SERIALIZABLE))
#define GVS_SERIALIZABLE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GVS_TYPE_SERIALIZABLE, GvsSerializableInterface))

typedef struct _GvsSerializable             GvsSerializable;
typedef struct _GvsSerializableInterface    GvsSerializableInterface;

struct _GvsSerializableInterface
{
    /*<private>*/
    GTypeInterface    parent_iface;
    
    /*<public>*/
    
};

GType       gvs_serializable_get_type         (void) G_GNUC_CONST;



G_END_DECLS

#endif
