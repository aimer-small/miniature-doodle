// automake group room , Wed Aug  3 19:37:54 2011
inherit ROOM;
#include <ansi.h>
#include "/d/group/gate.h"
void create()
{
set("short","财源滚滚[0;0m");
set("long"," [1;31m财源滚滚来!
[0;0m");
set("exits",([
"south":__DIR__"1312371061.c",
]));
set("objects",([
"/d/group/obj/qiangui.c":1,
]));
set("indoors","[1;36m和平[1;37m饭店[0;0m");
set("group1","peace");
set("group2","[1;36m和平[1;37m饭店[0;0m");
setup();
setup_var();
}
