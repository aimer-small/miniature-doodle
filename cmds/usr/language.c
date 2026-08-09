// language.c
// Command to switch between Chinese (cn) and English (en) display language.
// Usage: language [cn|en]
//        language     - shows current language setting
//        language cn  - switch to Chinese
//        language en  - switch to English

#include <ansi.h>

int help()
{
	write(@TEXT
指令格式：language [cn|en]

这个指令让你切换游戏界面语言。

language      - 查看当前语言设定
language cn   - 切换到中文界面
language en   - 切换到英文界面

当前支持的语言：
  cn  - 中文 (Chinese) [默认]
  en  - 英文 (English)

TEXT
	);
	return 1;
}

int main(object me, string arg)
{
	string current;

	current = me->query("env/language");
	if (!current) current = "cn";

	if (!arg) {
		if (current == "en")
			write("Current language setting: English (en)\n");
		else
			write("当前语言设定：中文 (cn)\n");
		write("使用 "HIC"language cn"NOR" 切换到中文，使用 "HIC"language en"NOR" 切换到英文。\n");
		return 1;
	}

	arg = lower_case(arg);

	if (arg != "cn" && arg != "en") {
		write("不支持的语言。请使用 cn (中文) 或 en (英文)。\n");
		write("Unsupported language. Please use cn (Chinese) or en (English).\n");
		return 1;
	}

	if (arg == current) {
		if (arg == "en")
			write("Already in English mode.\n");
		else
			write("已经是中文模式了。\n");
		return 1;
	}

	me->set("env/language", arg);

	if (arg == "en") {
		write(HIG "Language switched to English. Use 'language cn' to switch back.\n" NOR);
		// Also notify nearby players
		message("vision", me->name() + " switched to English mode.\n", me);
	} else {
		write(HIG "语言已切换到中文。输入 language en 可切换到英文。\n" NOR);
		message("vision", me->name() + " 切换到中文模式。\n", me);
	}

	return 1;
}