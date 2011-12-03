
# 2011-12-03: This doesn't yet work with Python-GObject 3.0.2?

from gi.repository import PsMoveApi

PsMoveApi.init()

c = PsMoveApi.Controller()

for i in range(1000):
    c.r = (c.r + 2) % 255

