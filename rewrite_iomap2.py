import re

with open('src/io/iomap.hpp', 'r') as f:
    content = f.read()

# Replace full methods with just declarations
replacements = [
    (r'static bool loadMonsters\(Map\* map\) \{.*?\n\t\}', 'static bool loadMonsters(Map* map);'),
    (r'static bool loadZones\(Map\* map\) \{.*?\n\t\}', 'static bool loadZones(Map* map);'),
    (r'static bool loadNpcs\(Map\* map\) \{.*?\n\t\}', 'static bool loadNpcs(Map* map);'),
    (r'static bool loadHouses\(Map\* map\) \{.*?\n\t\}', 'static bool loadHouses(Map* map);'),
    (r'static bool loadMonstersCustom\(Map\* map, const std::string &mapName, int customMapIndex\) \{.*?\n\t\}', 'static bool loadMonstersCustom(Map* map, const std::string &mapName, int customMapIndex);'),
    (r'static bool loadZonesCustom\(Map\* map, const std::string &mapName, int customMapIndex\) \{.*?\n\t\}', 'static bool loadZonesCustom(Map* map, const std::string &mapName, int customMapIndex);'),
    (r'static bool loadNpcsCustom\(Map\* map, const std::string &mapName, int customMapIndex\) \{.*?\n\t\}', 'static bool loadNpcsCustom(Map* map, const std::string &mapName, int customMapIndex);'),
    (r'static bool loadHousesCustom\(Map\* map, const std::string &mapName, int customMapIndex\) \{.*?\n\t\}', 'static bool loadHousesCustom(Map* map, const std::string &mapName, int customMapIndex);')
]

for pat, rep in replacements:
    content = re.sub(pat, rep, content, flags=re.DOTALL)

with open('src/io/iomap.hpp', 'w') as f:
    f.write(content)
