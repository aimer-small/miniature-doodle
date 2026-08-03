string chinese_number(int i)
{
	return CHINESE_D->chinese_number(i);
}

string to_chinese(string str)
{
	return CHINESE_D->chinese(str);
}

int is_chinese(string str)
{
	// FluffOS: str[0] returns Unicode codepoint, not byte value
	int c;

	if (strlen(str) < 1) return 0;
	c = str[0];

	// CJK Unified Ideographs: U+4E00 - U+9FFF
	if (c >= 0x4e00 && c <= 0x9fff) return 1;
	// CJK Extension A: U+3400 - U+4DBF
	if (c >= 0x3400 && c <= 0x4dbf) return 1;
	// CJK Compatibility Ideographs: U+F900 - U+FAFF
	if (c >= 0xf900 && c <= 0xfaff) return 1;
	// Fullwidth forms / Chinese punctuation
	// CJK Unified Ideographs Extension B..H — too rare for names, skip

	return 0;
}
