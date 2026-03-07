import re

with open('src/io/iomap.hpp', 'r') as f:
    content = f.read()

# Remove the includes
content = re.sub(r'// TODO: move to \.cpp for avoid circular dependencies\n', '', content)
content = re.sub(r'#include "config/configmanager\.hpp"\n', '', content)
content = re.sub(r'#include "map/house/house\.hpp"\n', '', content)
content = re.sub(r'#include "items/item\.hpp"\n', '', content)
content = re.sub(r'#include "map/map\.hpp"\n', '', content)
content = re.sub(r'#include "creatures/monsters/spawns/spawn_monster\.hpp"\n', '', content)
content = re.sub(r'#include "creatures/npcs/spawns/spawn_npc\.hpp"\n', '', content)
content = re.sub(r'#include "game/zones/zone\.hpp"\n', '', content)

# Add class Map; forward declaration
content = re.sub(r'class IOMap \{', 'class Map;\n\nclass IOMap {', content)

with open('src/io/iomap.hpp', 'w') as f:
    f.write(content)
