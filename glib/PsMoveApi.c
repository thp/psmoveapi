
#include <psmoveapi/psmove.h>

static PSMove *_glib_move;
static char _glib_move_r, _glib_move_g, _glib_move_b;

void ps_move_api_impl_init()
{
    _glib_move = psmove_connect();
}

int ps_move_api_impl_get_r()
{
    return _glib_move_r;
}

void ps_move_api_impl_set_r(int r)
{
    _glib_move_r = r;
    psmove_set_leds(_glib_move, _glib_move_r,
            _glib_move_g, _glib_move_b);
    psmove_update_leds(_glib_move);
}

void ps_move_api_impl_rumble()
{
    psmove_set_leds(_glib_move, 255, 0, 255);
    psmove_update_leds(_glib_move);
}

