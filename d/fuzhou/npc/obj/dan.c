// Quest秘籍 - 合法游戏道具（spec系统）
#include <ansi.h>
inherit ITEM;

void create()
{
        set_name(HIW"Quest秘籍"NOR, ({ "yangjing dan","dan" }) );
        set_weight(10);
        set("unit", "本");
        set("long", "一本可以增加Quest机会的书籍。\n");
        set("no_drop", "这样贵重的东西怎么能随便乱丢呢。\n");
        set("no_get", "这样东西不能离开那儿。\n");
        set_weight(100);
        set("value",3000000);
        set("no_give",1);
        set("treasure",1);
        set("degree",1);
        set("flag","spec/jiemi");
        set("rest",0);
        set("desc","一本可以增加Quest机会的书籍！");
        set("credit",1000);

       setup();
}
