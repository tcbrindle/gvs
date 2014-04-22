/*
 * Tests serialization and deserialization of fundemental types
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
    int int_prop;
    double dbl_prop;
    float float_prop;
    char *str_prop;
};

G_DEFINE_TYPE_WITH_PRIVATE(TestItem, test_item, G_TYPE_OBJECT);

enum
{
    PROP_0,
    PROP_INT_PROP,
    PROP_DBL_PROP,
    PROP_FLOAT_PROP,
    PROP_STR_PROP
};

static void
test_item_set_property(GObject *obj,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *pspec)
{
    TestItem *self = TEST_ITEM(obj);
    
    switch (prop_id)
    {
        case PROP_INT_PROP:
            self->priv->int_prop = g_value_get_int(value);
            break;

        case PROP_DBL_PROP:
            self->priv->dbl_prop = g_value_get_double(value);
            break;

        case PROP_FLOAT_PROP:
            self->priv->float_prop = g_value_get_float(value);
            break;

        case PROP_STR_PROP:
            g_free(self->priv->str_prop);
            self->priv->str_prop = g_value_dup_string(value);
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
    
    switch (prop_id)
    {
        case PROP_INT_PROP:
            g_value_set_int(value, self->priv->int_prop);
            break;

        case PROP_DBL_PROP:
            g_value_set_double(value, self->priv->dbl_prop);
            break;

        case PROP_FLOAT_PROP:
            g_value_set_float(value, self->priv->float_prop);
            break;

        case PROP_STR_PROP:
            g_value_set_string(value, self->priv->str_prop);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
    }
}

static void
test_item_finalize(GObject *obj)
{
    TestItem *self = TEST_ITEM(obj);

    g_free(self->priv->str_prop);

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

    pspec = g_param_spec_int("int-prop", "int-prop", "int-prop",
                             0, G_MAXINT, 0,
                             G_PARAM_READWRITE |
                             G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_INT_PROP, pspec);

    pspec = g_param_spec_double("dbl-prop", "dbl-prop", "dbl-prop",
                                -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                G_PARAM_READWRITE |
                                G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_DBL_PROP, pspec);

    pspec = g_param_spec_float("float-prop", "float-prop", "float-prop",
                                -G_MAXFLOAT, G_MAXFLOAT, 0.0,
                                G_PARAM_READWRITE |
                                G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, PROP_FLOAT_PROP, pspec);

    pspec = g_param_spec_string("str-prop", "str-prop", "str-prop", "Test",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                                G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (gobject_class, PROP_STR_PROP, pspec);

}

static void
test_item_init(TestItem *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, TEST_TYPE_ITEM, TestItemPrivate);
}

static void
assert_items_identical(TestItem *item1, TestItem *item2)
{
    TestItemPrivate *priv1 = item1->priv;
    TestItemPrivate *priv2 = item2->priv;

    g_assert(priv1->int_prop == priv2->int_prop);
    g_assert(priv1->dbl_prop == priv2->dbl_prop);
    g_assert(priv1->float_prop == priv2->float_prop);
    //g_assert(g_str_equal(priv1->str_prop, priv2->str_prop));
}

static const char serialised_object[] = 
"[('TestItem', <{"
"    'int-prop': <17>,"
"    'dbl-prop': <3.1415926535897931>,"
"    'float-prop': <1.5707963705062866>,"
"    'str-prop': <@ms nothing>"
"}>)]";

static void
test_serialize(void)
{
    TestItem *item1 = NULL;
    TestItem *item2 = NULL;
    GVariant *variant1 = NULL;
    GVariant *variant2 = NULL;
    GError *error = NULL;
    
    item1 = g_object_new(TEST_TYPE_ITEM,
                         "int-prop", 17,
                         "dbl-prop", G_PI,
                         "float-prop", G_PI_2,
                         "str-prop", NULL,
                          NULL);

    variant1 = gvs_gobject_serialize(G_OBJECT(item1));
    g_assert(variant1);

    //g_print("%s\n", g_variant_print(variant1, TRUE));

    variant2 = g_variant_parse(NULL, serialised_object, NULL, NULL, &error);
    g_assert_no_error(error);

    g_assert(g_variant_equal(variant1, variant2));

    item2 = gvs_gobject_new_deserialize(variant2);
    g_assert(item2);

    assert_items_identical(item1, item2);
    
    g_object_unref(item2);
    g_object_unref(item1);
    g_variant_unref(variant2);
    g_variant_unref (variant1);
}

int
main(int argc, char *argv[])
{
   g_test_init(&argc, &argv, NULL);
   g_test_add_func("/Gvs/BasicTypes", test_serialize);
   return g_test_run();
}
