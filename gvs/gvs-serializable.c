/* gvs-serializable.c: Interface for custom serialization of GObjects
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
#include "gvs-serializable.h"
#undef __GVS_INSIDE__

G_DEFINE_INTERFACE (GvsSerializable, gvs_serializable, G_TYPE_OBJECT)


static void
gvs_serializable_default_init (GvsSerializableInterface *iface)
{
}
