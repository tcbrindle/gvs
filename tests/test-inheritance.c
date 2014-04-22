/*
 * Tests that serialization works correctly with inheritance, including property
 * overrides
 */

#include <gvs/gvs.h>

/* TestBase object */

#define TEST_TYPE_BASE            (test_base_get_type())
#define TEST_BASE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_BASE, TestBase))
#define TEST_BASE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TEST_TYPE_BASE, TestBaseClass))
#define TEST_IS_BASE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_BASE))
#define TEST_IS_BASE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TEST_TYPE_BASE))
#define TEST_BASE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TEST_TYPE_BASE, TestBaseClass))

typedef struct _TestBase        TestBase;
typedef struct _TestBaseClass   TestBaseClass;
typedef struct _TestBasePrivate TestBasePrivate;

struct _TestBase
{
    GObject parent;

    TestBasePrivate *priv;
};

struct _TestBaseClass
{
    GObjectClass parent_class;
};

struct _TestBasePrivate
{
    int int_prop;
    double dbl_prop;
    float float_prop;
    char *str_prop;
};

G_DEFINE_TYPE_WITH_PRIVATE(TestBase, test_base, G_TYPE_OBJECT);

enum
{
    BASE_PROP_0,
    BASE_PROP_INT_PROP,
    BASE_PROP_DBL_PROP,
    BASE_PROP_FLOAT_PROP,
    BASE_PROP_STR_PROP
};

static void
test_base_set_property(GObject *obj,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *pspec)
{
    TestBase *self = TEST_BASE(obj);
    
    switch (prop_id)
    {
        case BASE_PROP_INT_PROP:
            self->priv->int_prop = g_value_get_int(value);
            break;

        case BASE_PROP_DBL_PROP:
            self->priv->dbl_prop = g_value_get_double(value);
            break;

        case BASE_PROP_FLOAT_PROP:
            self->priv->float_prop = g_value_get_float(value);
            break;

        case BASE_PROP_STR_PROP:
            g_free(self->priv->str_prop);
            self->priv->str_prop = g_value_dup_string(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
    }
}

static void
test_base_get_property(GObject *obj,
                       guint prop_id,
                       GValue *value,
                       GParamSpec *pspec)
{
    TestBase *self = TEST_BASE(obj);
    
    switch (prop_id)
    {
        case BASE_PROP_INT_PROP:
            g_value_set_int(value, self->priv->int_prop);
            break;

        case BASE_PROP_DBL_PROP:
            g_value_set_double(value, self->priv->dbl_prop);
            break;

        case BASE_PROP_FLOAT_PROP:
            g_value_set_float(value, self->priv->float_prop);
            break;

        case BASE_PROP_STR_PROP:
            g_value_set_string(value, self->priv->str_prop);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
    }
}

static void
test_base_finalize(GObject *obj)
{
    TestBase *self = TEST_BASE(obj);

    g_free(self->priv->str_prop);

    G_OBJECT_CLASS(test_base_parent_class)->finalize(obj);
}

static void
test_base_class_init(TestBaseClass *klass)
{
    GParamSpec *pspec;
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->set_property = test_base_set_property;
    gobject_class->get_property = test_base_get_property;
    gobject_class->finalize = test_base_finalize;

    pspec = g_param_spec_int("int-prop", "int-prop", "int-prop",
                             0, G_MAXINT, 0,
                             G_PARAM_READWRITE |
                             G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, BASE_PROP_INT_PROP, pspec);

    pspec = g_param_spec_double("dbl-prop", "dbl-prop", "dbl-prop",
                                -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                G_PARAM_READWRITE |
                                G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, BASE_PROP_DBL_PROP, pspec);

    pspec = g_param_spec_float("float-prop", "float-prop", "float-prop",
                                -G_MAXFLOAT, G_MAXFLOAT, 0.0,
                                G_PARAM_READWRITE |
                                G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, BASE_PROP_FLOAT_PROP, pspec);

    pspec = g_param_spec_string("str-prop", "str-prop", "str-prop", "Test",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                                G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (gobject_class, BASE_PROP_STR_PROP, pspec);

}

static void
test_base_init(TestBase *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, TEST_TYPE_BASE, TestBasePrivate);
}


/* TestDerived object */

#define TEST_TYPE_DERIVED            (test_derived_get_type())
#define TEST_DERIVED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_DERIVED, TestDerived))
#define TEST_DERIVED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TEST_TYPE_DERIVED, TestDerivedClass))
#define TEST_IS_DERIVED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_DERIVED))
#define TEST_IS_DERIVED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TEST_TYPE_DERIVED))
#define TEST_DERIVED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TEST_TYPE_DERIVED, TestDerivedClass))

typedef struct _TestDerived        TestDerived;
typedef struct _TestDerivedClass   TestDerivedClass;
typedef struct _TestDerivedPrivate TestDerivedPrivate;

struct _TestDerived
{
    TestBase parent;

    TestDerivedPrivate *priv;
};

struct _TestDerivedClass
{
    TestBaseClass parent_class;
};

struct _TestDerivedPrivate
{
    guint64 uint64_prop;
    char *str_prop;
};

G_DEFINE_TYPE_WITH_PRIVATE(TestDerived, test_derived, TEST_TYPE_BASE);

enum
{
    DERIVED_PROP_0,
    DERIVED_PROP_UINT64_PROP,
    DERIVED_PROP_STR_PROP /* override */
};

static void
test_derived_set_property(GObject *obj,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
    TestDerived *self = TEST_DERIVED(obj);
    TestDerivedPrivate *priv = self->priv;
    
    switch (prop_id)
    {
        case DERIVED_PROP_UINT64_PROP:
            priv->uint64_prop = g_value_get_uint64(value);
            break;

        case DERIVED_PROP_STR_PROP:
            g_free(priv->str_prop);
            priv->str_prop = g_value_dup_string(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
    }
}

static void
test_derived_get_property(GObject *obj,
                          guint prop_id,
                          GValue *value,
                          GParamSpec *pspec)
{
    TestDerived *self = TEST_DERIVED(obj);
    TestDerivedPrivate *priv = self->priv;

    switch (prop_id)
    {
        case DERIVED_PROP_UINT64_PROP:
            g_value_set_uint64(value, priv->uint64_prop);
            break;

        case DERIVED_PROP_STR_PROP:
            g_value_set_string(value, priv->str_prop);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
    }
}

static void
test_derived_finalize(GObject *obj)
{
    TestDerived *self = TEST_DERIVED(obj);

    g_free(self->priv->str_prop);

    G_OBJECT_CLASS(test_derived_parent_class)->finalize(obj);
}

static void
test_derived_class_init(TestDerivedClass *klass)
{
    GParamSpec *pspec;
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->set_property = test_derived_set_property;
    gobject_class->get_property = test_derived_get_property;
    gobject_class->finalize = test_derived_finalize;

    pspec = g_param_spec_uint64 ("uint64-prop", "uint64-prop", "uint64-prop",
                                 0, 10, 4,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                 G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(gobject_class, DERIVED_PROP_UINT64_PROP, pspec);

    g_object_class_override_property (gobject_class, DERIVED_PROP_STR_PROP, "str-prop");
}

static void
test_derived_init(TestDerived *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, TEST_TYPE_DERIVED, TestDerivedPrivate);
}


static const char serialized_object[] = 
"[('TestDerived', <{"
"    'int-prop': <17>,"
"    'dbl-prop': <3.1415926535897931>,"
"    'float-prop': <1.5707963705062866>,"
"    'str-prop': <@ms 'GVS Test'>,"
"    'uint64-prop': <uint64 4>"
"}>)]";

static void
test_serialize(void)
{
    TestDerived *item1 = NULL;
    TestDerived *item2 = NULL;
    GVariant *variant1 = NULL;
    GVariant *variant2 = NULL;
    GError *error = NULL;

    item1 = g_object_new(TEST_TYPE_DERIVED,
                         "int-prop", 17,
                         "dbl-prop", G_PI,
                         "float-prop", G_PI_2,
                         "str-prop", "GVS Test",
                          NULL);

    variant1 = gvs_gobject_serialize(G_OBJECT(item1));
    //g_print("%s\n", g_variant_print(variant1, TRUE));
    g_assert(variant1);

    variant2 = g_variant_parse(NULL, serialized_object, NULL, NULL, &error);
    g_assert_no_error(error);
    g_assert(variant2);

    g_assert(g_variant_equal(variant1, variant2));

    item2 = gvs_gobject_new_deserialize(variant2);
    g_assert(item2);


    g_object_unref(item1);
    g_object_unref(item2);
    g_variant_unref(variant1);
    g_variant_unref(variant2);
}

int
main(int argc, char *argv[])
{
   g_test_init(&argc, &argv, NULL);
   g_test_add_func("/Gvs/Inheritance", test_serialize);
   return g_test_run();
}
