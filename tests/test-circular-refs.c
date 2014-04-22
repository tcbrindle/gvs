/*
 * Tests [de]serialization of objects with circular references to other objects
 */

#include <gvs/gvs.h>

/* TestItem object */

#define TEST_TYPE_ITEM            (test_item_get_type())
#define TEST_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_ITEM, TestItem))
#define TEST_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TEST_TYPE_ITEM, TestItemClass))
#define TEST_IS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_ITEM))
#define TEST_IS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TEST_TYPE_ITEM))
#define TEST_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TEST_TYPE_ITEM, TestItemClass))

typedef struct _TestItem        TestItem;
typedef struct _TestItemClass   TestItemClass;
typedef struct _TestItemPrivate TestItemPrivate;

struct _TestItem
{
    GObject parent;

    TestItemPrivate *priv;
};

struct _TestItemClass
{
    GObjectClass parent_class;
};

struct _TestItemPrivate
{
    TestItem *child;
    TestItem *parent;
    char *name;
};

G_DEFINE_TYPE_WITH_PRIVATE(TestItem, test_item, G_TYPE_OBJECT);

enum
{
    PROP_0,
    PROP_CHILD,
    PROP_PARENT,
    PROP_NAME
};

static void
test_item_set_property(GObject *obj,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *pspec)
{
    TestItem *self = TEST_ITEM(obj);
    TestItemPrivate *priv = self->priv;
    
    switch (prop_id)
    {
        case PROP_CHILD:
            g_clear_object(&priv->child);
            priv->child = g_value_dup_object (value);
            break;

        case PROP_PARENT:
            priv->parent = g_value_get_object(value);
            break;

        case PROP_NAME:
            g_free(priv->name);
            priv->name = g_value_dup_string (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
    }
}

static void
test_item_get_property(GObject *obj,
                       guint prop_id,
                       GValue *value,
                       GParamSpec *pspec)
{
    TestItem *self = TEST_ITEM(obj);
    TestItemPrivate *priv = self->priv;
    
    switch (prop_id)
    {
        case PROP_CHILD:
            g_value_set_object(value, priv->child);
            break;

        case PROP_PARENT:
            g_value_set_object(value, priv->parent);
            break;

        case PROP_NAME:
            g_value_set_string (value, priv->name);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
    }
}

static TestItem *
test_item_get_parent(TestItem *self)
{
    return self->priv->parent;
}

static TestItem *
test_item_get_child(TestItem *self)
{
    return self->priv->child;
}

static void
test_item_finalize(GObject *obj)
{
    TestItemPrivate *priv = TEST_ITEM(obj)->priv;

    g_clear_object (&priv->child);
    priv->parent = NULL; /* No reference held */

    G_OBJECT_CLASS(test_item_parent_class)->finalize(obj);
}

static void
test_item_class_init(TestItemClass *klass)
{
    GParamSpec *pspec;
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->set_property = test_item_set_property;
    gobject_class->get_property = test_item_get_property;
    gobject_class->finalize = test_item_finalize;

    pspec = g_param_spec_object("child", "child", "Child",
                                TEST_TYPE_ITEM,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_CHILD, pspec);

    pspec = g_param_spec_object("parent", "parent", "Parent",
                                TEST_TYPE_ITEM,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_PARENT, pspec);

    pspec = g_param_spec_string ("name", "name", "name", NULL,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                 G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_NAME, pspec);

}

static void
test_item_init(TestItem *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, TEST_TYPE_ITEM, TestItemPrivate);
}

static TestItem *
test_item_new(const char* name)
{
    return g_object_new(TEST_TYPE_ITEM, "name", name, NULL);
}

static const char serialized_object[] =
"[('TestItem', <{'child': <@mt 1>, 'parent': <@mt nothing>, 'name': <@ms 'parent'>}>),"
" ('TestItem', <{'child': <@mt nothing>, 'parent': <@mt 0>, 'name': <@ms 'child'>}>)]";


static void
test_serialize(void)
{
    TestItem *parent = NULL;
    TestItem *child = NULL;
    TestItem *created_parent = NULL;
    TestItem *created_child = NULL;

    GVariant *variant1 = NULL;
    GVariant *variant2 = NULL;
    GError *error = NULL;
    
    parent = test_item_new("parent");
    child = test_item_new("child");

    g_object_set(parent, "child", child, NULL);
    g_object_set(child, "parent", parent, NULL);

    variant1 = gvs_gobject_serialize(G_OBJECT(parent));
    //g_print("%s\n", g_variant_print(variant1, TRUE));

    variant2 = g_variant_parse(NULL, serialized_object, NULL, NULL, &error);
    g_assert_no_error(error);

    g_assert(g_variant_equal(variant1, variant2));

    created_parent = gvs_gobject_new_deserialize(variant2);
    g_assert(created_parent);
    g_assert(test_item_get_parent(created_parent) == NULL);

    created_child = test_item_get_child(created_parent);
    g_assert(created_child);
    g_assert(test_item_get_parent(created_child) == created_parent);
    g_assert(test_item_get_child(created_child) == NULL);
}

int
main(int argc, char *argv[])
{
   g_test_init(&argc, &argv, NULL);
   g_test_add_func("/Gvs/CircularRefs", test_serialize);
   return g_test_run();
}
