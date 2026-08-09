#!/usr/bin/env python3
"""
Room Translation Tool - 房间翻译工具
Scans room files and generates translation data for the bilingual system.

Usage:
  python3 translate_rooms.py [directory]
  python3 translate_rooms.py /workspace/d/city  # scan all rooms in city
  
Output:
  Generates translation data files in /workspace/data/translation/
  - room_names.txt: Room name translations
  - room_descriptions.txt: Room description translations
"""

import os
import re
import sys
import json

# Translation data directory
TRANS_DIR = "/workspace/data/translation"
# Existing translation files (loaded first to avoid duplicates)
EXISTING_TRANSLATIONS = {}

def load_existing_translations():
    """Load existing translations from data files"""
    if not os.path.exists(TRANS_DIR):
        return
    
    for fname in os.listdir(TRANS_DIR):
        if fname.endswith(".txt"):
            fpath = os.path.join(TRANS_DIR, fname)
            with open(fpath, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        if '=' in line:
                            cn, en = line.split('=', 1)
                            EXISTING_TRANSLATIONS[cn.strip()] = en.strip()

def extract_room_short(content):
    """Extract set('short', ...) from room file content"""
    # Match various patterns: set("short", "...") or set('short', '...')
    patterns = [
        r'set\s*\(\s*["\']short["\']\s*,\s*["\']([^"\']+)["\']\s*\)',
        r'set\s*\(\s*["\']short["\']\s*,\s*["\']([^"\']+)["\']\s*\)',
    ]
    # Handle ANSI color codes
    ansi_pattern = r'\x1b\[[0-9;]*m'
    
    for pattern in patterns:
        match = re.search(pattern, content)
        if match:
            short = match.group(1)
            # Remove ANSI codes
            short = re.sub(ansi_pattern, '', short)
            return short
    return None

def extract_room_long(content):
    """Extract set('long', @LONG ... LONG) from room file content"""
    # Match @LONG ... LONG heredoc style
    long_match = re.search(r'set\s*\(\s*["\']long["\']\s*,\s*@LONG\s*\n(.*?)\nLONG', content, re.DOTALL)
    if long_match:
        long_text = long_match.group(1)
        # Clean up: remove trailing/leading whitespace and concatenation
        lines = []
        for line in long_text.split('\n'):
            line = line.strip()
            # Remove string concatenation
            line = line.rstrip('+')
            line = line.strip()
            # Remove leading/trailing quotes
            line = line.strip('"\'')
            if line:
                lines.append(line)
        return '\n'.join(lines)
    
    # Match set("long", "...") inline style
    inline_match = re.search(r'set\s*\(\s*["\']long["\']\s*,\s*["\']([^"\']+)["\']\s*\)', content)
    if inline_match:
        return inline_match.group(1)
    
    return None

def extract_room_name(content):
    """Try to extract the English name from comments or other hints"""
    # Look for comments with English names
    eng_match = re.search(r'#\s*English:\s*(.+)$', content, re.MULTILINE)
    if eng_match:
        return eng_match.group(1).strip()
    return None

def generate_english_short(cn_short):
    """Generate a placeholder English translation for a Chinese room name"""
    # Common Chinese room name patterns mapped to English
    translations = {
        '广场': 'Plaza',
        '大街': 'Street',
        '北门': 'North Gate',
        '南门': 'South Gate',
        '东门': 'East Gate',
        '西门': 'West Gate',
        '客栈': 'Inn',
        '茶馆': 'Teahouse',
        '酒楼': 'Restaurant',
        '当铺': 'Pawnshop',
        '钱庄': 'Bank',
        '药铺': 'Pharmacy',
        '书院': 'Academy',
        '武馆': 'Martial Arts School',
        '衙门': 'Government Office',
        '赌场': 'Casino',
        '铁匠铺': 'Blacksmith',
        '杂货铺': 'General Store',
        '兵器铺': 'Weapon Shop',
        '书店': 'Bookstore',
        '首饰店': 'Jewelry Shop',
        '服装店': 'Tailor Shop',
        '花园': 'Garden',
        '树林': 'Forest',
        '竹林': 'Bamboo Grove',
        '小河': 'River',
        '码头': 'Dock',
        '山路': 'Mountain Path',
        '山道': 'Mountain Trail',
        '山峰': 'Mountain Peak',
        '山谷': 'Valley',
        '山洞': 'Cave',
        '石洞': 'Stone Cave',
        '草地': 'Grassland',
        '草原': 'Prairie',
        '沙漠': 'Desert',
        '平原': 'Plain',
        '关口': 'Pass',
        '桥': 'Bridge',
        '渡口': 'Ferry',
        '塔': 'Pagoda',
        '庙': 'Temple',
        '寺': 'Temple',
        '观': 'Temple',
        '庵': 'Nunnery',
        '宫': 'Palace',
        '殿': 'Hall',
        '阁': 'Pavilion',
        '楼': 'Tower',
        '亭': 'Pavilion',
        '台': 'Terrace',
        '厅': 'Hall',
        '院': 'Courtyard',
        '园': 'Garden',
        '庄': 'Manor',
        '府': 'Mansion',
        '铺': 'Shop',
        '店': 'Shop',
        '馆': 'Inn',
        '站': 'Station',
    }
    
    # Try to build a translation from known parts
    result = cn_short
    for cn, en in translations.items():
        if cn in result:
            result = result.replace(cn, en)
    
    return result

def scan_rooms(directory):
    """Scan all room files in a directory and extract translation data"""
    room_names = {}
    room_descs = {}
    
    for root, dirs, files in os.walk(directory):
        for fname in files:
            if fname.endswith('.c'):
                fpath = os.path.join(root, fname)
                try:
                    with open(fpath, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                    
                    short = extract_room_short(content)
                    if short and short not in EXISTING_TRANSLATIONS:
                        en_short = generate_english_short(short)
                        room_names[short] = en_short
                    
                    long_text = extract_room_long(content)
                    if long_text and long_text not in EXISTING_TRANSLATIONS:
                        # Use the first line of the long description as a key
                        first_line = long_text.split('\n')[0].strip()
                        if first_line and len(first_line) > 10:
                            room_descs[first_line] = f"[TODO] {first_line}"
                    
                except Exception as e:
                    print(f"Error processing {fpath}: {e}")
    
    return room_names, room_descs

def write_translation_file(filename, data, header):
    """Write translation data to a file"""
    if not data:
        print(f"No data to write to {filename}")
        return
    
    fpath = os.path.join(TRANS_DIR, filename)
    with open(fpath, 'w', encoding='utf-8') as f:
        f.write(f"# {header}\n")
        f.write(f"# Auto-generated by translate_rooms.py\n")
        f.write(f"# Generated on: {__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"# Format: Chinese = English\n\n")
        
        for cn, en in sorted(data.items()):
            f.write(f"{cn} = {en}\n")
    
    print(f"Written {len(data)} translations to {fpath}")

def main():
    # Load existing translations
    load_existing_translations()
    print(f"Loaded {len(EXISTING_TRANSLATIONS)} existing translations")
    
    # Determine which directories to scan
    if len(sys.argv) > 1:
        directories = sys.argv[1:]
    else:
        # Default: scan all room directories
        base_dir = "/workspace/d"
        directories = [os.path.join(base_dir, d) for d in os.listdir(base_dir) 
                      if os.path.isdir(os.path.join(base_dir, d))]
    
    all_names = {}
    all_descs = {}
    
    for directory in directories:
        if not os.path.exists(directory):
            print(f"Directory not found: {directory}")
            continue
        
        print(f"Scanning {directory}...")
        names, descs = scan_rooms(directory)
        all_names.update(names)
        all_descs.update(descs)
        print(f"  Found {len(names)} room names, {len(descs)} descriptions")
    
    # Write translation files
    write_translation_file("room_names_auto.txt", all_names, "Auto-generated room name translations")
    write_translation_file("room_descs_auto.txt", all_descs, "Auto-generated room description translations (TODO)")

if __name__ == "__main__":
    main()