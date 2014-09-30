
interface Test.Iface : GLib.Object
{
    public abstract string string_prop { get; set; }
    public abstract int int_prop { get; construct; }
}

class Test.Impl : GLib.Object, Iface
{
    public string string_prop { get; set; }
    public int int_prop { get; construct; default = 92; }
}

class Test.Object : GLib.Object
{
    public Test.Iface iface_prop { get; set; }
    
    construct
    {
        iface_prop = new Test.Impl();
    }
}

static const string serialized_object = 
"""
(uint32 1735816047, uint16 1,
 [('TestObject', <{'iface-prop': <@mt 1>}>),
  ('TestImpl', <{'string-prop': <@ms 'A custom string'>, 'int-prop': <92>}>)
 ]
)
""";

void assert_objects_identical(Test.Object t1, Test.Object t2)
{
    assert(t1.iface_prop.string_prop == t2.iface_prop.string_prop);
    assert(t1.iface_prop.int_prop == t2.iface_prop.int_prop);
}

void test_serialize()
{
    var t1 = new Test.Object();
    t1.iface_prop.string_prop = "A custom string";

    var v1 = Gvs.gobject_serialize(t1);
    assert(v1 != null);
    //print("%s\n", v1.print(true));

    try {
        var v2 = Variant.parse(null, serialized_object);
        assert(v1.equal(v2));

        var t2 = Gvs.gobject_new_deserialize(v2) as Test.Object;
        assert(t2 != null);

        assert_objects_identical(t1, t2);
    } catch {
        assert_not_reached();
    }
}

void main(string[] args)
{
    GLib.Test.init(ref args);

    GLib.Test.add_func("/Gvs/Vala/Interfaces", test_serialize);

    GLib.Test.run();
}


