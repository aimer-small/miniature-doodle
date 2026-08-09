// translated.c
// Translation Daemon - Bilingual Chinese/English support for Shujian MUD
// Provides text translation services for rooms, items, NPCs, and game messages.

#include <ansi.h>
#include <database.h>

#define TRANSLATION_DATA_DIR	"/data/translation/"
#define TRANSLATION_SAVE		TRANSLATION_DATA_DIR + "translations"

inherit F_SAVE;

// Main translation mapping: ([ "cn_text" : "en_text" ])
nosave mapping translations;
// Room-specific translations: ([ room_file : ([ "short" : "en", "long" : "en", ... ]) ])
nosave mapping room_translations;
// Item/NPC translations: ([ file_path : ([ "name" : "en", "desc" : "en", ... ]) ])
nosave mapping object_translations;

// Auto-translation cache for frequently used phrases
nosave mapping phrase_cache;

void create()
{
	seteuid(getuid());
	translations = ([]);
	room_translations = ([]);
	object_translations = ([]);
	phrase_cache = ([]);
	restore();
	// Load additional translation files if available
	load_translations();
}

// Return the save file location
string query_save_file()
{
	return TRANSLATION_SAVE;
}

// Load all translation files from the translation data directory
void load_translations()
{
	string *files;
	int i;

	if (file_size(TRANSLATION_DATA_DIR) != -2) {
		mkdir(TRANSLATION_DATA_DIR);
		return;
	}

	// Load both .txt and .json files
	files = get_dir(TRANSLATION_DATA_DIR + "*.txt");
	if (files && sizeof(files)) {
		for (i = 0; i < sizeof(files); i++) {
			load_translation_file(TRANSLATION_DATA_DIR + files[i]);
		}
	}
	
	files = get_dir(TRANSLATION_DATA_DIR + "*.json");
	if (files && sizeof(files)) {
		for (i = 0; i < sizeof(files); i++) {
			load_translation_file(TRANSLATION_DATA_DIR + files[i]);
		}
	}
}

// Load a single translation data file
void load_translation_file(string file)
{
	string content;
	string *lines;
	int i;

	content = read_file(file);
	if (!content || strlen(content) == 0) return;

	lines = explode(content, "\n");
	for (i = 0; i < sizeof(lines); i++) {
		string line = lines[i];
		string key, value;
		
		// Skip empty lines and comments
		line = trim(line);
		if (strlen(line) == 0) continue;
		if (line[0] == '#') continue;
		
		// Parse "key = value" format
		if (sscanf(line, "%s = %s", key, value) == 2) {
			key = trim(key);
			value = trim(value);
			if (strlen(key) > 0 && strlen(value) > 0) {
				translations[key] = value;
			}
		}
	}
}

// Add a single translation entry
void add_translation(string cn_text, string en_text)
{
	if (!cn_text || !en_text) return;
	translations[cn_text] = en_text;
	save();
}

// Remove a translation entry
void remove_translation(string cn_text)
{
	if (!cn_text) return;
	map_delete(translations, cn_text);
	save();
}

// Translate a Chinese text string to English
// Returns the English translation if available, or the original Chinese text
string translate(string cn_text)
{
	if (!cn_text || strlen(cn_text) == 0)
		return cn_text;

	// Check cache first
	if (translations[cn_text])
		return translations[cn_text];

	// Check phrase cache
	if (phrase_cache[cn_text])
		return phrase_cache[cn_text];

	return cn_text;
}

// Check if a translation exists for the given text
int has_translation(string cn_text)
{
	return (translations[cn_text] != undefinedp(translations[cn_text]));
}

// Get the user's language preference
string get_user_language(object user)
{
	if (!user) return "cn";
	string lang = user->query("env/language");
	if (!lang) return "cn";
	return lang;
}

// Translate text for a specific viewer
// If the viewer prefers English and a translation exists, return the English version
string translate_for(string cn_text, object viewer)
{
	if (!cn_text) return cn_text;
	if (!viewer) return cn_text;

	string lang = get_user_language(viewer);
	if (lang == "en") {
		string en = translate(cn_text);
		if (en != cn_text) return en;
	}
	return cn_text;
}

// Add a room translation (short + long descriptions)
void add_room_translation(string room_file, string short_cn, string short_en, string long_cn, string long_en)
{
	if (!room_file) return;

	if (!room_translations[room_file])
		room_translations[room_file] = ([]);

	if (short_cn && short_en)
		room_translations[room_file]["short"] = short_en;
	if (long_cn && long_en)
		room_translations[room_file]["long"] = long_en;

	// Also add individual translations
	if (short_cn && short_en)
		add_translation(short_cn, short_en);
	if (long_cn && long_en)
		add_translation(long_cn, long_en);

	save();
}

// Get room translation
mapping get_room_translation(string room_file)
{
	if (!room_file) return 0;
	return room_translations[room_file];
}

// Batch load translations from a mapping
void load_translation_map(mapping trans_map)
{
	string *keys;
	int i;

	if (!trans_map || !mapp(trans_map)) return;

	keys = keys(trans_map);
	for (i = 0; i < sizeof(keys); i++) {
		translations[keys[i]] = trans_map[keys[i]];
	}
	save();
}

// Clear all translations
void clear_all()
{
	translations = ([]);
	room_translations = ([]);
	object_translations = ([]);
	phrase_cache = ([]);
	save();
}

// Get translation statistics
mapping get_stats()
{
	return ([
		"total_translations" : sizeof(translations),
		"total_rooms" : sizeof(room_translations),
		"total_objects" : sizeof(object_translations),
	]);
}

// Export all translations as a mapping (for backup)
mapping dump_all()
{
	return ([
		"translations" : translations,
		"room_translations" : room_translations,
		"object_translations" : object_translations,
	]);
}