// zhizhu.c 蜘蛛

inherit NPC;

void create()
{
	
        set_name("巨型蜘蛛", ({ "zhi zhu", "zhizhu" }) );
	set("race", "野兽");
	set("age", 2);
	set("long", "一只张牙舞爪的巨型蜘蛛。\n");
	
	set("max_qi", 4000);
	set("eff_qi", 4000);
	set("qi", 4000);
	set("str", 100);
	set("con", 30);
	set("dex", 100);

	set("limbs", ({ "头部", "身体", "前脚", "后脚", "尾巴" }) );
	set("verbs", ({ "bite", "claw" }) );

	set("combat_exp", 800000);
	
		
        set_temp("apply/attack", 100);
        set_temp("apply/damage", 100);
	set_temp("apply/armor", 800);

	setup();
}
void init()
{
	object ob;
	::init();
	if (interactive(ob = this_player()) && living(this_object()) &&
		(int)ob->query_condition("killer")) {
		remove_call_out("kill_ob");
		call_out("kill_ob", 0, ob);
		ob->start_busy(3);
	}
}

