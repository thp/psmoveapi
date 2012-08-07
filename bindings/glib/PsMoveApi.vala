
namespace PsMoveApi {
    namespace Impl {
        extern void init();
        extern int get_r();
        extern void set_r(int r);
    }

    public void init() {
        Impl.init();
    }

    public class Controller : Object {
        public int r {
            get { return Impl.get_r(); }
            set { Impl.set_r(value); }
        }
    }
}

