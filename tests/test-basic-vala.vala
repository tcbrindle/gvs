
class Test.Object : GLib.Object
{
    public string str_prop { get; set; }
    public int int_prop { get; set; }
    public double dbl_prop { get; set; }

    public Object()
    {
        base(str_prop: "hello world",
               int_prop: 27,
               dbl_prop: Math.PI);
    }
}


static const string serialized_object = 
"""
(uint32 1735816047,
 uint16 1,
 [('TestObject',
    <{'str-prop': <@ms 'hello world'>,
      'int-prop': <27>,
      'dbl-prop': <3.1415926535897931>}
    >)])
""";

void assert_items_identical(Test.Object t1, Test.Object t2)
{
    assert(t1.str_prop == t2.str_prop);
    assert(t1.int_prop == t2.int_prop);
    assert(t1.dbl_prop == t2.dbl_prop);
}

void test_serialize()
{
    var t = new Test.Object();

    var v1 = Gvs.gobject_serialize(t);

    assert(v1 != null);

    //print("%s\n", v1.print(true));

    try {
        var v2 = Variant.parse(null, serialized_object);
        assert(v1.equal(v2));
        var t2 = Gvs.gobject_new_deserialize(v2) as Test.Object;
        assert(t2 != null);
        assert_items_identical(t, t2);
    } catch {
        assert_not_reached();
    }
}

void main(string[] args)
{
    GLib.Test.init(ref args);

    GLib.Test.add_func("/Gvs/Vala/Basic", test_serialize);

    GLib.Test.run();
}