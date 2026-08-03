// automake group room , Fri Jul 29 21:15:29 2011
inherit ROOM;
#include <ansi.h>
#include "/d/group/gate.h"
void create()
{
set("short","石路");
set("long","这是一条刚刚建造好，还没有粉刷的石路。
");
set("exits",([
"south":"/d/group/entry/xstulu2.c","north":__DIR__"1311945302.c","east":__DIR__"1311945143.c","west":__DIR__"1311945329.c",
]));
set("objects",([
"/d/group/obj/door.c":1,
]));
set("outdoors","[1;31m日[1;34m月[1;37m神教[0;0m");
set("group1","rysj");
set("group2","[1;31m日[1;34m月[1;37m神教[0;0m");
setup();
setup_var();
}
