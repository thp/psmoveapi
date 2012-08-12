
#include <IOBluetooth/objc/IOBluetoothHostController.h>
#include "psmove_osxsupport.h"

char *
macosx_get_btaddr()
{
    char *result;

    IOBluetoothHostController *controller =
        [IOBluetoothHostController defaultController];

    NSString *addr = [controller addressAsString];
    result = strdup([addr UTF8String]);

    return result;
}

