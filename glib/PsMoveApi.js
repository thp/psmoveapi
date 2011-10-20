
const PsMoveApi = imports.gi.PsMoveApi;

PsMoveApi.init();

var c = new PsMoveApi.Controller();

for (var i=0; i<1000; i++) {
    c.r = (c.r + 2) % 255;
}

