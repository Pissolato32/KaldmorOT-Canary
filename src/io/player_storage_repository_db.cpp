/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019–present OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "io/player_storage_repository_db.hpp"

#include "database/database.hpp"
#include "lib/di/container.hpp"

#ifndef USE_PRECOMPILED_HEADERS
	#include <cstdint>
	#include <map>
	#include <set>
	#include <string>
	#include <vector>
	#include <fmt/format.h>
	#include <fmt/ranges.h>
#endif

std::vector<PlayerStorageRow> DbPlayerStorageRepository::load(uint32_t id) {
	std::vector<PlayerStorageRow> out;
	if (auto result = Database::getInstance().storeQuery(
			"SELECT `key`, `value` FROM `player_storage` WHERE `player_id` = ?",
			{ id }
		)) {
		do {
			out.push_back({ result->getNumber<uint32_t>("key"), result->getNumber<int32_t>("value") });
		} while (result->next());
	}
	return out;
}

bool DbPlayerStorageRepository::deleteKeys(uint32_t id, const std::vector<uint32_t> &keys) {
	if (keys.empty()) {
		return true;
	}
	std::string placeholders;
	placeholders.reserve(keys.size() * 2);
	for (size_t i = 0; i < keys.size(); ++i) {
		if (i > 0) {
			placeholders += ',';
		}
		placeholders += '?';
	}
	std::vector<DatabaseVariant> params;
	params.reserve(keys.size() + 1);
	params.emplace_back(id);
	for (auto k : keys) {
		params.emplace_back(k);
	}
	return Database::getInstance().executeQuery(
		"DELETE FROM `player_storage` WHERE `player_id` = ? AND `key` IN (" + placeholders + ")",
		params
	);
}

bool DbPlayerStorageRepository::upsert(uint32_t id, const std::map<uint32_t, int32_t> &kv) {
	if (kv.empty()) {
		return true;
	}
	DBInsert insert("INSERT INTO `player_storage` (`player_id`,`key`,`value`) VALUES ");
	insert.upsert({ "value" });
	for (auto &[key, value] : kv) {
		std::ostringstream row;
		row << id << ',' << key << ',' << value;
		if (!insert.addRow(row)) {
			return false;
		}
	}
	return insert.execute();
}

IPlayerStorageRepository &g_playerStorageRepository() {
	return inject<DbPlayerStorageRepository>();
}
