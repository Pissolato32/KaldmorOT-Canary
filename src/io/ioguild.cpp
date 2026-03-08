/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019–present OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "io/ioguild.hpp"

#include "database/database.hpp"
#include "creatures/players/grouping/guild.hpp"

std::shared_ptr<Guild> IOGuild::loadGuild(uint32_t guildId) {
	Database &db = Database::getInstance();
	if (DBResult_ptr result = db.storeQuery(
			"SELECT `name`, `balance` FROM `guilds` WHERE `id` = ?",
			{ guildId }
		)) {
		const auto guild = std::make_shared<Guild>(guildId, result->getString("name"));
		guild->setBankBalance(result->getNumber<uint64_t>("balance"));

		if ((result = db.storeQuery(
				"SELECT `id`, `name`, `level` FROM `guild_ranks` WHERE `guild_id` = ?",
				{ guildId }
			))) {
			do {
				guild->addRank(result->getNumber<uint32_t>("id"), result->getString("name"), result->getNumber<uint16_t>("level"));
			} while (result->next());
		}
		return guild;
	}
	return nullptr;
}

void IOGuild::saveGuild(const std::shared_ptr<Guild> &guild) {
	if (!guild) {
		return;
	}
	Database::getInstance().executeQuery(
		"UPDATE `guilds` SET `balance` = ? WHERE `id` = ?",
		{ guild->getBankBalance(), guild->getId() }
	);
}

uint32_t IOGuild::getGuildIdByName(const std::string &name) {
	DBResult_ptr result = Database::getInstance().storeQuery(
		"SELECT `id` FROM `guilds` WHERE `name` = ?",
		{ name }
	);
	if (!result) {
		return 0;
	}
	return result->getNumber<uint32_t>("id");
}

	DBResult_ptr result = Database::getInstance().storeQuery(
		"SELECT `guild1`, `guild2` FROM `guild_wars` WHERE (`guild1` = ? OR `guild2` = ?) AND `status` = 1",
		{ guildId, guildId }
	);
	if (!result) {
		return;
	}

	do {
		auto guild1 = result->getNumber<uint32_t>("guild1");
		if (guildId != guild1) {
			guildWarVector.push_back(guild1);
		} else {
			guildWarVector.push_back(result->getNumber<uint32_t>("guild2"));
		}
	} while (result->next());
}
