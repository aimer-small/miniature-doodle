#include <ansi.h>
#include <job_mul.h>
inherit NPC;
string ask_job();
string ask_finish();
void create()
{
        set_name("苦头陀",({"ku toutuo", "ku"}));
        set("long", "一个长发披肩的头陀，身材魁伟，满面横七竖八的都是刀疤，本来相貌已全不可辨。\n");
        set("age", 56);
        set("attitude", "peaceful");
        set("shen", -10000);
        set("str", 26);
        set("per", 28);
        set("int", 29);
        set("con", 27);
        set("dex", 30);
        set("max_qi", 3500);
        set("max_jing", 2200);
        set("eff_jingli", 2500);
        set("jiali", 80);
        set("combat_exp", 1500000);
        set("unique", 1);

        set_skill("sword",170);
        set_skill("dodge",170);
        set_skill("force", 170);
        set_skill("blade",170);
        set_skill("literate",150);
        set_skill("hand",170);
        set_skill("strike",170);
        set_skill("hanbing-mianzhang",170);
        set_skill("yingzhua-shou",170);
        set_skill("piaoyi-shenfa", 170);
        set_skill("shenghuo-shengong", 170);
        set_skill("lieyan-dao",170);
        set_skill("liehuo-jian",170);
        set_skill("parry", 170);

        map_skill("force", "shenghuo-shengong");
        map_skill("dodge", "piaoyi-shenfa");
        map_skill("sword", "liehuo-jian");
        map_skill("parry", "liehuo-jian");
        map_skill("blade", "lieyan-dao");
        map_skill("hand", "yingzhua-shou");
        map_skill("strike", "hanbing-mianzhang");
        prepare_skill("strike", "hanbing-mianzhang");
        prepare_skill("hand","yingzhua-shou");

        set("inquiry", ([
                 "营救" : (: ask_job :),
                 "脱险" : (: ask_finish :),
        ]));
        setup();
        carry_object(BINGQI_D("blade"))->wield();
        carry_object("/d/mingjiao/obj/white-cloth")->wear();
}

int check_ip(object *team)
{
        int i, j;
        for (i = 0;i < sizeof(team);i ++)
                for (j = i + 1;j < sizeof(team);j ++)
                        if (team[i] && query_ip_number(team[i]) == query_ip_number(team[j]))
                                return 0;
        return 1;
}

void reset_room(int num)
{
        int i;
        for (i = 1;i <= 13;i ++)
        {
                "/cmds/app/update"->main(this_object(), "/d/jingcheng/wanan-si/baota-" + i);
                ("/d/jingcheng/wanan-si/baota-" + i)->setup_shouwei(num + random(3));
        }
}

string ask_job()
{
        object me = this_player(), *team = me->query_team();
        int i, max, min, total = 0;
        object ob_list = filter_array(children(USER_OB), (: clonep($1) && !wizardp($1) :));
        
        foreach (object user in ob_list)
                if (user->query_temp("was_job"))
                        return "正有人在万安寺内救人，不劳大驾了。\n";
        if (sizeof(team) < 2)
                return "救人之事十分危险，仅凭一人之力恐怕不能胜任。\n";
        if (sizeof(team) > 3)
                return "万安寺内防守极其严密，人数太多更容易暴露。\n";
        if (team[0] != me) 
                return "只有队伍的首领才可以带队救人。\n";
        if (!check_ip(team))
                return "你的队伍有问题，无法给你任务。\n";
                
        max = min = me->query("combat_exp");
        for (i = 0;i < sizeof(team);i ++)
        {
                if (!team[i])
                        return "你的队伍出现了问题，请解散并重新组建。";
                if (!present(team[i]))
                        return "怎么好象人不全啊？" + team[i]->query("name") + "怎么没来？\n";
              if (team[i]->query("combat_exp") < 2000000)
                        return "救人之事十分危险，" + team[i]->query("name") + "恐怕不能胜任。\n";
                if (team[i]->query_condition("was_job"))
                        return "你队伍中的" + team[i]->query("name") + "已经十分辛苦，换其他人吧。\n";

                if (me == team[i])
                        continue;
                if (!interactive(team[i]))
                        return "你还是将" + team[i]->query("name") + "换成其他人吧。\n";
                total += team[i]->query("combat_exp");
                if (team[i]->query("combat_exp") > max)
                        max = team[i]->query("combat_exp"); 
                if (team[i]->query("combat_exp") < min)
                        min = team[i]->query("combat_exp");
        }
        if ((total < 1000000))
                return "救人之事十分危险，恐怕你们不能胜任。\n";
        if (min / (max / 100) < 80)
                return "救人之事十分危险，你们的差距太大了。\n";
        
        command("say 寺内十分危险，务必小心！");
//         CHANNEL_D->do_channel( this_object(), "rumor",sprintf(HBMAG+HIW"%s组队开始万安寺营救任务！", me->name(1)));
        reset_room(sizeof(team));
        for (i = 0;i < sizeof(team);i ++)
        {
                team[i]->delete_temp("was_job");
                team[i]->set_temp("was_job/asked", 1);
                team[i]->set_temp("was_job/num", sizeof(team));
                team[i]->apply_condition("job_busy", 20);
                team[i]->apply_condition("was_job", 90);
                team[i]->apply_condition("was_check",90);
                team[i]->set("job_name", "万安寺救人");
                team[i]->move("/d/jingcheng/wanan-si/wanan-siyuan");
                tell_object(team[i], HIR"你来到宝塔前，发现宝塔已经起火，救人迫在眉睫！\n"NOR);
        }
        return "希望此行一切顺利。\n";
}

string ask_finish()
{
        object me = this_player(), *team = me->query_team();
        object xy_letter;
        int i,j,z,exp,shen,pot,SJCredit,gold,gold2,dbexp,ext;
        
        if (!me->query_temp("was_job/asked"))
                        return "你没领取过任务,不要来捣乱。\n";

        if (sizeof(team) != 0 && (team[0]!=me))
		  return "只有队伍的首领才可以完成任务。\n";

        if (!check_ip(team))
                return "你的队伍有问题，现在无法完成任务。\n";

        if( !me->query_team() )
{ 
	
              me->delete_temp("was_job");           
	            me->clear_condition("was_check");
	           return "你把自己的队友都丢了，还有脸来问奖励？\n";
             
}
                 
       
        for (i = 0;i < sizeof(team);i ++)
        {
                if (!team[i])
                        continue;
                if (!present(team[i]))
                        return "怎么好象人不全啊？" + team[i]->query("name") + "怎么没来？\n";
                if (!interactive(team[i]))
                        return "由于" + team[i]->query("name") + "的问题，你现在无法完成任务。\n";
                     }
                     
        z = (int)me->query_temp("was_job/num");
        z = 100-z*5;
    if(sizeof(team) != 0){
        if(sizeof(team)==(int)me->query_temp("was_job/num")&&!random(8)){
            write(HIR"\n突然，苦头陀悄悄地塞了一封信给你，也不知道是想干什么。。。\n"NOR);
            xy_letter=new("/clone/gift/xyjobletter");
            xy_letter->move(me);
            log_file("job/was", sprintf(HIG"%s(%s)%d次万安寺营救任务得到开启襄阳任务密函！"NOR,me->name(1),me->query("id"),me->query("job_time/万安寺营救")));
        }
        for (i = 0;i < sizeof(team);i ++)
        {         
                if (team[i] && team[i]->query_temp("was_job/asked"))
                {	   
		          j = team[i]->query_temp("was_job/floor");
                        exp = (j*450 + (j-1)*j*120 + random(100))*z/250;
                     if(wizardp(me)){
                         write("初始化j="+j+",exp = "+exp+" !\n"NOR);
                     }
                     team[i]->add("combat_exp",exp);
                     pot = exp/4;
		          SJCredit =( j/z+j*4) /10 ;
                     if((int)team[i]->query_temp("was_job/floor")>6)
                         SJCredit = SJCredit;
		          shen = exp/2 + pot/2;
		          gold=80000+random(100000);
		          gold2=gold/10000;
		          team[i]->add("balance",gold);
		          team[i]->apply_condition("job_busy", 5);
		          team[i]->apply_condition("was_job", 30);
		          team[i]->clear_condition("was_check");
		          
		          team[i]->add("job_time/万安寺营救",1); // 修改写法，以前是me->
		          team[i]->set("job_name", "万安寺营救");
                        tell_object(team[i],sprintf("[43;1m你获得了"+CHINESE_D->chinese_number(exp)+                      
                        "点[1;32m经验[43;1m，"+ CHINESE_D->chinese_number(pot)+
                        "点[1;34m潜能[43;1m的奖励。\n"NOR, team[i]));
                        tell_object(team[i],"[43;1m你获得了" + CHINESE_D->chinese_number(gold2) + "锭黄金存款。[0m\n\n");
			    		team[i]->add( "potential", pot );
		    			team[i]->add("shen", shen );
		    		//	team[i]->add("SJ_Credit", SJCredit );


        if( team[i]->query_condition("db_exp") )     {
		  dbexp = exp/2 + random(exp/2);
     	          team[i]->add("combat_exp", dbexp );
                                                       }
		 

 	if( team[i]->query("relife/exp_ext") ){ 
			ext = team[i]->query("relife/exp_ext");
			team[i]->add("combat_exp", exp* ext /20 );
			                                          }

	
       if( team[i]->query("relife/exp_ext") )     
               tell_object(team[i],HBGRN"你仔细回想着万安寺营救的过程，又额外增加了"+chinese_number(exp*ext/20)+"点实战经验！\n"NOR);


	if( team[i]->query_condition("db_exp") )     
		
               tell_object(team[i],HBGRN"双倍经验奖励期间，你额外地增加了"+chinese_number(dbexp)+"点实战经验！\n"NOR);





                                   log_file("job/was", sprintf("%s %s(%s)%d次万安寺营救任务%d层,得到经验%d，潜能%d。\n",
                                  ctime(time())[4..19],team[i]->name(1),team[i]->query("id"),(int)team[i]->query("job_time/万安寺营救"),j,exp,pot));
        }
                                  team[i]->delete_temp("was_job");
			    		
                }
    }
    else{
                if (me->query_temp("was_job/asked")){
                     j = me->query_temp("was_job/floor");
                     exp = (j*300 + (j-1)*j*80 + random(100))*z/250; //update by lsxk@hsbbs 2007/8/14
                     exp=exp/3+random(exp/3+1);
                     me->add("combat_exp",exp);
                     pot = exp/4;
                     SJCredit =1+ random(2);
                     shen = exp/2 + pot/2;
                     me->apply_condition("job_busy", 5);
                     me->apply_condition("was_job", 30);
                     me->clear_condition("was_check");
                     me->add("job_time/万安寺营救",1); // 修改写法，以前是me->
                     me->set("job_name", "万安寺营救");
                     tell_object(me,sprintf(HIW"你获得了"+CHINESE_D->chinese_number(exp)+
                        "点经验，"+ CHINESE_D->chinese_number(pot)+
                        "点潜能的奖励。\n"NOR, me));
                                   me->add( "potential", pot );
                                   me->add("shen", shen );
                           //        me->add("SJ_Credit", SJCredit );



if( me->query_condition("db_exp") )    { 
		  dbexp = exp/2 + random(exp/2);
     	me->add("combat_exp", dbexp );
               tell_object(me,HBGRN"你仔细回想着万安寺营救的过程，又额外增加了"+chinese_number(exp*ext/20)+"点实战经验！\n"NOR);
}
		 

 	if( me->query("relife/exp_ext") ){ 
			ext = me->query("relife/exp_ext");
			me->add("combat_exp", exp* ext /20 );
               tell_object(team[i],HBGRN"双倍经验奖励期间，你额外地增加了"+chinese_number(dbexp)+"点实战经验！\n"NOR);
			                                          }


		




                                   log_file("job/was", sprintf("%s %s(%s)%d次万安寺营救任务得到经验%d，潜能%d。\n",
                                  ctime(time())[4..19],me->name(1),me->query("id"),(int)me->query("job_time/万安寺营救"),j,exp,pot));
                                  me->delete_temp("was_job");

                    if(random(4)){
                        write(HIR"\n突然，苦头陀悄悄地塞了一封信给你，也不知道是想干什么。。。\n"NOR);
                        xy_letter=new("/clone/gift/xyjobletter");
                        xy_letter->move(me);
                        log_file("job/was", sprintf(HIG"%s(%s)%d次万安寺营救任务得到开启襄阳任务密函！"NOR,me->name(1),me->query("id"),me->query("job_time/万安寺营救")));
                    }
          }
    }
        return "各位辛苦了，此次任务已经完成，回去休息吧。\n";
}

