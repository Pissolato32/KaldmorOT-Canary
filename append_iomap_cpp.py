import re

impls = """
bool IOMap::loadMonsters(Map* map) {
\tif (map->monsterfile.empty()) {
\t\t// OTBM file doesn't tell us about the monsterfile,
\t\t// Lets guess it is mapname-monster.xml.
\t\tmap->monsterfile = g_configManager().getString(MAP_NAME);
\t\tmap->monsterfile += "-monster.xml";
\t}

\treturn map->spawnsMonster.loadFromXML(map->monsterfile);
}

bool IOMap::loadZones(Map* map) {
\tif (map->zonesfile.empty()) {
\t\t// OTBM file doesn't tell us about the zonesfile,
\t\t// Lets guess it is mapname-zone.xml.
\t\tmap->zonesfile = g_configManager().getString(MAP_NAME);
\t\tmap->zonesfile += "-zones.xml";
\t}

\treturn Zone::loadFromXML(map->zonesfile);
}

bool IOMap::loadNpcs(Map* map) {
\tif (map->npcfile.empty()) {
\t\t// OTBM file doesn't tell us about the npcfile,
\t\t// Lets guess it is mapname-npc.xml.
\t\tmap->npcfile = g_configManager().getString(MAP_NAME);
\t\tmap->npcfile += "-npc.xml";
\t}

\treturn map->spawnsNpc.loadFromXml(map->npcfile);
}

bool IOMap::loadHouses(Map* map) {
\tif (map->housefile.empty()) {
\t\t// OTBM file doesn't tell us about the housefile,
\t\t// Lets guess it is mapname-house.xml.
\t\tmap->housefile = g_configManager().getString(MAP_NAME);
\t\tmap->housefile += "-house.xml";
\t}

\treturn map->houses.loadHousesXML(map->housefile);
}

bool IOMap::loadMonstersCustom(Map* map, const std::string &mapName, int customMapIndex) {
\tif (map->monsterfile.empty()) {
\t\t// OTBM file doesn't tell us about the monsterfile,
\t\t// Lets guess it is mapname-monster.xml.
\t\tmap->monsterfile = mapName;
\t\tmap->monsterfile += "-monster.xml";
\t}
\treturn map->spawnsMonsterCustomMaps[customMapIndex].loadFromXML(map->monsterfile);
}

bool IOMap::loadZonesCustom(Map* map, const std::string &mapName, int customMapIndex) {
\tif (map->zonesfile.empty()) {
\t\t// OTBM file doesn't tell us about the zonesfile,
\t\t// Lets guess it is mapname-zones.xml.
\t\tmap->zonesfile = mapName;
\t\tmap->zonesfile += "-zones.xml";
\t}
\treturn Zone::loadFromXML(map->zonesfile, customMapIndex);
}

bool IOMap::loadNpcsCustom(Map* map, const std::string &mapName, int customMapIndex) {
\tif (map->npcfile.empty()) {
\t\t// OTBM file doesn't tell us about the npcfile,
\t\t// Lets guess it is mapname-npc.xml.
\t\tmap->npcfile = mapName;
\t\tmap->npcfile += "-npc.xml";
\t}

\treturn map->spawnsNpcCustomMaps[customMapIndex].loadFromXml(map->npcfile);
}

bool IOMap::loadHousesCustom(Map* map, const std::string &mapName, int customMapIndex) {
\tif (map->housefile.empty()) {
\t\t// OTBM file doesn't tell us about the housefile,
\t\t// Lets guess it is mapname-house.xml.
\t\tmap->housefile = mapName;
\t\tmap->housefile += "-house.xml";
\t}
\treturn map->housesCustomMaps[customMapIndex].loadHousesXML(map->housefile);
}
"""

with open('src/io/iomap.cpp', 'r') as f:
    content = f.read()

# Add necessary includes to the top of the file
includes = """#include "config/configmanager.hpp"
#include "map/house/house.hpp"
#include "items/item.hpp"
#include "map/map.hpp"
#include "creatures/monsters/spawns/spawn_monster.hpp"
#include "creatures/npcs/spawns/spawn_npc.hpp"
#include "game/zones/zone.hpp"
"""

content = content.replace('#include "io/iomap.hpp"\n', '#include "io/iomap.hpp"\n\n' + includes)
content += "\n" + impls

with open('src/io/iomap.cpp', 'w') as f:
    f.write(content)
