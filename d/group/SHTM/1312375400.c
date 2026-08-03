// automake group room , Wed Aug  3 20:50:42 2011
inherit ROOM;
#include <ansi.h>
#include "/d/group/gate.h"
void create()
{
set("short","房间");
set("long","这是一座刚刚建造好，还没有粉刷的房屋。
");
set("exits",([
"southeast":"/d/group/entry/dlndao2.c","up":__DIR__"1312375573.c",
]));
set("objects",([
"/d/group/obj/biaozhi.c":1,
]));
set("indoors","[1;32m四海同盟[0;0m");
set("group1","SHTM");
set("group2","[1;32m四海同盟[0;0m");
setup();
setup_var();
}
