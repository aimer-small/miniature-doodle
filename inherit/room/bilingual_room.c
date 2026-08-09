// bilingual_room.c
// Bilingual room inherit for Chinese/English language switching support.
// Inherit this instead of ROOM to add bilingual support to your room.
//
// Usage:
//   inherit BILINGUAL_ROOM;
//   // instead of: inherit ROOM;
//   
//   void create() {
//       set("short", "中文名称");
//       set("short_en", "English Name");
//       set("long", @LONG
// 中文描述...
// LONG
//       );
//       set("long_en", @LONG
// English description...
// LONG
//       );
//       // ... rest of room setup
//   }

#include <dbase.h>
#include <room.h>
#include <ansi.h>

inherit ROOM;

// Check if a viewer (player) prefers English
int viewer_prefers_english(object viewer)
{
    if (!viewer) return 0;
    string lang = viewer->query("env/language");
    return (lang == "en");
}

// Get the effective viewer - try this_player() first
object get_viewer()
{
    object me = this_player();
    if (me) return me;
    
    // Fallback: check if there's a querying object
    // (called from look_room which passes the player)
    return 0;
}

// Override query to support bilingual short/long descriptions
varargs mixed query(string prop, int raw)
{
    // For short and long descriptions, check language preference
    if (prop == "short" || prop == "long") {
        object viewer = get_viewer();
        if (viewer && viewer_prefers_english(viewer)) {
            // Try English version first
            mixed en_text = ::query(prop + "_en", 1);
            if (en_text) {
                // If raw is requested, return the English text directly
                if (raw) return en_text;
                // Otherwise evaluate it (handles function pointers)
                return evaluate(en_text, this_object());
            }
            // Fallback: try TRANSLATE_D
            mixed cn_text = ::query(prop, 1);
            if (cn_text) {
                string cn_str;
                if (stringp(cn_text))
                    cn_str = cn_text;
                else if (functionp(cn_text))
                    cn_str = evaluate(cn_text, this_object());
                else
                    cn_str = sprintf("%O", cn_text);
                
                string en_translated = TRANSLATE_D->translate(cn_str);
                if (en_translated != cn_str)
                    return en_translated;
            }
        }
    }
    
    // For all other properties, or when no English version is available
    return ::query(prop, raw);
}

// Set both Chinese and English text for a property
// Usage: set_bilingual("short", "中文", "English")
void set_bilingual(string prop, string cn, string en)
{
    if (prop && cn)
        ::set(prop, cn);
    if (prop && en)
        ::set(prop + "_en", en);
}

// Convenience function: set English short description
void set_short_en(string en)
{
    if (en)
        ::set("short_en", en);
}

// Convenience function: set English long description
void set_long_en(string en)
{
    if (en)
        ::set("long_en", en);
}

// Get the short description in the viewer's preferred language
string get_short_for(object viewer)
{
    if (!viewer) return ::query("short");
    
    if (viewer_prefers_english(viewer)) {
        string en = ::query("short_en", 1);
        if (en) return en;
        
        string cn = ::query("short", 1);
        if (cn) {
            string translated = TRANSLATE_D->translate(cn);
            if (translated != cn) return translated;
        }
    }
    return ::query("short");
}

// Get the long description in the viewer's preferred language
string get_long_for(object viewer)
{
    if (!viewer) return ::query("long");
    
    if (viewer_prefers_english(viewer)) {
        string en = ::query("long_en", 1);
        if (en) return en;
        
        string cn = ::query("long", 1);
        if (cn) {
            string translated = TRANSLATE_D->translate(cn);
            if (translated != cn) return translated;
        }
    }
    return ::query("long");
}