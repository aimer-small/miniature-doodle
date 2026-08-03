// automake group room , Fri Jul 29 21:37:44 2011
inherit ROOM;
#include <ansi.h>
#include "/d/group/gate.h"
void create()
{
set("short","成德殿[0;0m");
set("long","日光从东射来，照上一座汉白玉的巨大牌匾，
牌匾上四个金色大字“泽被苍生”
在阳光下发出闪闪金光，不由得令人肃然起敬。
大殿横阔不过三十来尺，纵深却有三百来尺，
彼端高设一座，为神教教主之位，
两侧分别摆放八张座椅，为副教主及堂主之位。
[0;0m");
set("exits",([
"south":__DIR__"1311945064.c","east":__DIR__"1311945406.c","west":__DIR__"1311945371.c",
]));
set("objects",([
"/d/group/obj/biaozhi.c":1,
]));
set("indoors","[1;31m日[1;34m月[1;37m神教[0;0m");
set("group1","rysj");
set("group2","[1;31m日[1;34m月[1;37m神教[0;0m");
setup();
setup_var();
}
