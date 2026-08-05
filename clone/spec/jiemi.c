// 金庸全集 - 解谜天书
// 合法游戏道具，获取途径：
//   1. spec系统兑换（50000 credit）
//   2. 高级任务极低概率掉落
//   3. 特殊活动奖励
// 使用条件：需要极高的天赋属性（悟性、福缘、根骨、身法、膂力综合 > 300随机判定）
// 每个角色终身只能使用一次，失败不消耗道具
#include <ansi.h>
inherit ITEM;

void create()
{
        set_name(HIR"金庸全集"NOR, ({ "book","quanji" }) );
        set_weight(10);
        set("unit", "本");
        set("long", "一本神奇的书籍，据说是金庸先生留下的武林秘籍总纲。\n"
                     "传说有缘人可从中领悟绝世武功，但需要极高的天赋和机缘。\n");
        set_weight(100);
        set("value", 3000000);
        set("no_give", 1);
        set("treasure", 1);
        set("degree", 2);
        set("flag", "spec/jiemi");
        set("rest", 1);
        set("desc", HIR"解谜天书"NOR);
        set("credit", 50000);
        setup();
}

void init()
{
        add_action("do_read", "read");
}

int do_read(string arg)
{
        object me = this_player();
        int kar, intel, dex, con, pur, total_chance, rnd;

        if (arg != "book" && arg != "quanji")
                return notify_fail("你要翻看什么？\n");
        if (me->is_busy())
                return notify_fail("你正忙着呢。\n");
        if (me->query("wizard/jiemi"))
                return notify_fail("你已经参悟过金庸全集了，无法再次领悟。\n");

        kar = me->query_kar();
        intel = me->query_int();
        dex = me->query_dex();
        con = me->query_con();
        pur = me->query("pur");

        // 基础概率：需要极高的天赋（五项属性总和作为随机上限）
        total_chance = kar + intel + dex + con + pur;
        rnd = random(300);

        if (rnd > total_chance) {
                tell_object(me, HIR"你翻开金庸全集，只觉其中文字深奥难懂，看了半天竟毫无头绪。\n"NOR);
                tell_object(me, HIY"也许你需要更高的天赋和机缘才能领悟其中奥秘。\n"NOR);
                return 1;
        }

        tell_object(me, HIR"你翻开金庸全集，眼前浮现出无数武学秘诀，你可以选择参悟其中一项：\n"NOR);
        tell_object(me, HIR"1：九阴真经上卷。\n"NOR);
        tell_object(me, HIR"2：九阴真经下卷。\n"NOR);
        tell_object(me, HIR"3：冷泉神功。\n"NOR);
        tell_object(me, HIR"4：蛤蟆功。\n"NOR);
        tell_object(me, HIR"5：凌波微步。\n"NOR);
        tell_object(me, HIR"6：葵花宝典。\n"NOR);
        tell_object(me, HIR"7：左右互搏。\n"NOR);
        tell_object(me, HIR"8：凝血神爪。\n"NOR);
        tell_object(me, HIR"你想要参悟的是："NOR);
        input_to("get_gift", 1);
        return 1;
}

void get_gift(string arg)
{
        object me = this_player();
        int select, kar, intel, dex, con, pur;
        string str;

        kar = me->query_kar();
        intel = me->query_int();
        dex = me->query_dex();
        con = me->query_con();
        pur = me->query("pur");

        if (!sscanf(arg, "%d", select)) {
                tell_object(me, HIR"你只能选择1-8中的一个，请重新选择："NOR);
                input_to("get_gift", 1);
                return;
        }

        switch (select) {
                case 1: // 九阴上卷 - 需要高悟性和福缘
                        if (me->query("quest/jiuyin1/pass")) {
                                tell_object(me, HIC"你已经学会了九阴神功上卷。请重新选择："NOR);
                                input_to("get_gift", 1);
                                return;
                        }
                        if (intel < 30 || kar < 26 || random(100) > kar + intel - 30) {
                                tell_object(me, HIR"你的天赋不足以参悟九阴真经上卷。\n"NOR);
                                str = "看来机缘未到，还需继续修炼。\n";
                                break;
                        }
                        tell_object(me, HIG"恭喜，你参悟了九阴真经的上卷。\n"NOR);
                        CHANNEL_D->do_channel(this_object(), "rumor", me->name(1) + "参悟金庸全集，领悟了" + HIW + "九阴神功上卷" + NOR + "。\n");
                        me->set("quest/jiuyin1/pass", 1);
                        str = "好好保重吧，英雄。\n";
                        break;

                case 2: // 九阴下卷
                        if (me->query("quest/jiuyin2/pass")) {
                                tell_object(me, HIC"你已经学会了九阴神功下卷。请重新选择："NOR);
                                input_to("get_gift", 1);
                                return;
                        }
                        if (intel < 28 || kar < 24 || random(100) > kar + intel - 25) {
                                tell_object(me, HIR"你的天赋不足以参悟九阴真经下卷。\n"NOR);
                                str = "看来机缘未到，还需继续修炼。\n";
                                break;
                        }
                        tell_object(me, HIG"恭喜，你参悟了九阴真经的下卷。\n"NOR);
                        CHANNEL_D->do_channel(this_object(), "rumor", me->name(1) + "参悟金庸全集，领悟了" + HIW + "九阴神功下卷" + NOR + "。\n");
                        me->set("quest/jiuyin2/pass", 1);
                        str = "好好保重吧，英雄。\n";
                        break;

                case 3: // 冷泉神功
                        if (me->query("quest/雪山飞狐/武功/lengquanshengong")) {
                                tell_object(me, HIC"你已经学会了冷泉神功。请重新选择："NOR);
                                input_to("get_gift", 1);
                                return;
                        }
                        if (con < 28 || intel < 28 || kar < 28 || random(100) > kar + con - 30) {
                                tell_object(me, HIR"你的天赋不足以参悟冷泉神功。\n"NOR);
                                str = "看来机缘未到，还需继续修炼。\n";
                                break;
                        }
                        tell_object(me, HIG"恭喜，你参悟了冷泉神功。\n"NOR);
                        CHANNEL_D->do_channel(this_object(), "rumor", me->name(1) + "参悟金庸全集，领悟了" + HIW + "冷泉神功" + NOR + "。\n");
                        me->set("quest/雪山飞狐/武功/lengquanshengong", 1);
                        str = "好好保重吧，英雄。\n";
                        break;

                case 4: // 蛤蟆功
                        if (me->query("oyf/son") && me->query("oyf/hamagong")) {
                                tell_object(me, HIC"你已经学会了蛤蟆功。请重新选择："NOR);
                                input_to("get_gift", 1);
                                return;
                        }
                        if (con < 26 || pur < 24 || random(100) > pur + con - 25) {
                                tell_object(me, HIR"你的天赋不足以参悟蛤蟆功。\n"NOR);
                                str = "看来机缘未到，还需继续修炼。\n";
                                break;
                        }
                        tell_object(me, HIG"恭喜，你参悟了蛤蟆功。\n"NOR);
                        CHANNEL_D->do_channel(this_object(), "rumor", me->name(1) + "参悟金庸全集，领悟了" + HIW + "蛤蟆功" + NOR + "。\n");
                        me->set("oyf/hamagong", 1);
                        me->set("oyf/son", 1);
                        me->set_skill("hamagong", 10);
                        str = "好好保重吧，英雄。\n";
                        break;

                case 5: // 凌波微步
                        if (me->query("quest/天龙八部/武功/yuxiang") && me->query("quest/天龙八部/武功/pass")) {
                                tell_object(me, HIC"你已经学会了凌波微步。请重新选择："NOR);
                                input_to("get_gift", 1);
                                return;
                        }
                        if (dex < 30 || intel < 30 || kar < 28 || random(100) > kar + dex - 30) {
                                tell_object(me, HIR"你的天赋不足以参悟凌波微步。\n"NOR);
                                str = "看来机缘未到，还需继续修炼。\n";
                                break;
                        }
                        tell_object(me, HIG"恭喜，你参悟了凌波微步。\n"NOR);
                        CHANNEL_D->do_channel(this_object(), "rumor", me->name(1) + "参悟金庸全集，领悟了" + HIW + "凌波微步" + NOR + "。\n");
                        me->set("quest/天龙八部/武功/yuxiang", 1);
                        me->set("quest/天龙八部/武功/pass", 1);
                        str = "好好保重吧，英雄。\n";
                        break;

                case 6: // 葵花宝典
                        if (me->query("quest/pixie/pass")) {
                                tell_object(me, HIC"你已经学会了葵花宝典。请重新选择："NOR);
                                input_to("get_gift", 1);
                                return;
                        }
                        if (me->query("gender") != "无性" || dex < 32 || intel < 28 || random(100) > dex + intel - 35) {
                                tell_object(me, HIR"你的天赋不足以参悟葵花宝典。\n"NOR);
                                str = "看来机缘未到，还需继续修炼。\n";
                                break;
                        }
                        tell_object(me, HIG"恭喜，你参悟了葵花宝典。\n"NOR);
                        CHANNEL_D->do_channel(this_object(), "rumor", me->name(1) + "参悟金庸全集，领悟了" + HIW + "葵花宝典" + NOR + "。\n");
                        me->set("quest/pixie/pass", 1);
                        str = "好好保重吧，英雄。\n";
                        break;

                case 7: // 左右互搏
                        if (me->query("quest/hubo/pass") && me->query("double_attack")) {
                                tell_object(me, HIC"你已经学会了左右互搏。请重新选择："NOR);
                                input_to("get_gift", 1);
                                return;
                        }
                        if (pur < 28 || intel < 26 || random(100) > pur + intel - 28) {
                                tell_object(me, HIR"你的天赋不足以参悟左右互搏。\n"NOR);
                                str = "看来机缘未到，还需继续修炼。\n";
                                break;
                        }
                        tell_object(me, HIG"恭喜，你参悟了左右互搏。\n"NOR);
                        CHANNEL_D->do_channel(this_object(), "rumor", me->name(1) + "参悟金庸全集，领悟了" + HIW + "左右互搏" + NOR + "。\n");
                        me->set("quest/hubo/pass", 1);
                        me->set("double_attack", 1);
                        str = "好好保重吧，英雄。\n";
                        break;

                case 8: // 凝血神爪
                        if (me->query("quest/ningxue/pass")) {
                                tell_object(me, HIC"你已经学会了凝血神爪。请重新选择："NOR);
                                input_to("get_gift", 1);
                                return;
                        }
                        if (con < 30 || dex < 30 || kar < 26 || random(100) > con + dex - 33) {
                                tell_object(me, HIR"你的天赋不足以参悟凝血神爪。\n"NOR);
                                str = "看来机缘未到，还需继续修炼。\n";
                                break;
                        }
                        tell_object(me, HIG"恭喜，你参悟了凝血神爪。\n"NOR);
                        CHANNEL_D->do_channel(this_object(), "rumor", me->name(1) + "参悟金庸全集，领悟了" + HIW + "凝血神爪" + NOR + "。\n");
                        me->set("quest/ningxue/pass", 1);
                        str = "好好保重吧，英雄。\n";
                        break;

                default:
                        tell_object(me, HIR"你只能选择1-8中的一个，请重新选择："NOR);
                        input_to("get_gift", 1);
                        return;
        }

        me->set("wizard/jiemi", 1);
        tell_object(me, HIR"金庸全集在你手中化为点点星光，消散于天地之间。\n"NOR);
        tell_object(me, HIR + str + NOR);
        destruct(this_object());
}