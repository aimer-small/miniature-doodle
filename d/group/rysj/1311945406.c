// automake group room , Fri Jul 29 21:50:40 2011
inherit ROOM;
#include <ansi.h>
#include "/d/group/gate.h"
void create()
{
set("short","密室[0;0m");
set("long","这是日月神教密室
里面放着日月神教四处征战抢来的宝物。
[0;0m");
set("exits",([
"west":__DIR__"1311945302.c",
]));
set("objects",([
"/d/group/obj/qiangui.c":1,
]));
set("indoors","[1;31m日[1;34m月[1;37m神教[0;0m");
set("group1","rysj");
set("group2","[1;31m日[1;34m月[1;37m神教[0;0m");
setup();
setup_var();
}
