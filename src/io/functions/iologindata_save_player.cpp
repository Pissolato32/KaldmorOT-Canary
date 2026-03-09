/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019–present OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "io/functions/iologindata_save_player.hpp"

#include "config/configmanager.hpp"
#include "creatures/players/animus_mastery/animus_mastery.hpp"
#include "creatures/combat/condition.hpp"
#include "creatures/monsters/monsters.hpp"
#include "creatures/players/components/player_storage.hpp"
#include "game/game.hpp"
#include "io/ioprey.hpp"
#include "items/containers/depot/depotchest.hpp"
#include "items/containers/inbox/inbox.hpp"
#include "items/containers/rewards/reward.hpp"
#include "creatures/players/player.hpp"
#include "io/player_storage_repository.hpp"
#include "kv/kv.hpp"
#include <fmt/format.h>

bool IOLoginDataSave::saveItems(const std::shared_ptr<Player> &player, const ItemBlockList &itemList, DBInsert &query_insert, PropWriteStream &propWriteStream) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	const Database &db = Database::getInstance();
	std::ostringstream ss;

	// Initialize variables
	using ContainerBlock = std::pair<std::shared_ptr<Container>, int32_t>;
	std::list<ContainerBlock> queue;
	int32_t runningId = 100;

	// Loop through each item in itemList
	const auto &openContainers = player->getOpenContainers();
	for (const auto &it : itemList) {
		const auto &item = it.second;
		if (!item) {
			continue;
		}

		int32_t pid = it.first;

		++runningId;

		// Update container attributes if necessary
		if (const std::shared_ptr<Container> &container = item->getContainer()) {
			if (!container) {
				continue;
			}

			if (container->getAttribute<int64_t>(ItemAttribute_t::OPENCONTAINER) > 0) {
				container->setAttribute(ItemAttribute_t::OPENCONTAINER, 0);
			}

			if (!openContainers.empty()) {
				for (const auto &its : openContainers) {
					auto openContainer = its.second;
					auto opcontainer = openContainer.container;

					if (opcontainer == container) {
						container->setAttribute(ItemAttribute_t::OPENCONTAINER, ((int)its.first) + 1);
						break;
					}
				}
			}

			// Add container to queue
			queue.emplace_back(container, runningId);
		}

		// Serialize item attributes
		propWriteStream.clear();
		item->serializeAttr(propWriteStream);

		size_t attributesSize;
		const char* attributes = propWriteStream.getStream(attributesSize);

		// Build query string and add row
		ss << player->getGUID() << ',' << pid << ',' << runningId << ',' << item->getID() << ',' << item->getSubType() << ',' << db.escapeBlob(attributes, static_cast<uint32_t>(attributesSize));
		if (!query_insert.addRow(ss)) {
			g_logger().error("Error adding row to query.");
			return false;
		}
	}

	// Loop through containers in queue
	while (!queue.empty()) {
		const ContainerBlock &cb = queue.front();
		const std::shared_ptr<Container> &container = cb.first;
		if (!container) {
			continue;
		}

		int32_t parentId = cb.second;

		// Loop through items in container
		for (auto &item : container->getItemList()) {
			if (!item) {
				continue;
			}

			++runningId;

			// Update sub-container attributes if necessary
			const auto &subContainer = item->getContainer();
			if (subContainer) {
				queue.emplace_back(subContainer, runningId);
				if (subContainer->getAttribute<int64_t>(ItemAttribute_t::OPENCONTAINER) > 0) {
					subContainer->setAttribute(ItemAttribute_t::OPENCONTAINER, 0);
				}

				if (!openContainers.empty()) {
					for (const auto &it : openContainers) {
						auto openContainer = it.second;
						auto opcontainer = openContainer.container;

						if (opcontainer == subContainer) {
							subContainer->setAttribute(ItemAttribute_t::OPENCONTAINER, (it.first) + 1);
							break;
						}
					}
				}
			}

			// Serialize item attributes
			propWriteStream.clear();
			item->serializeAttr(propWriteStream);

			size_t attributesSize;
			const char* attributes = propWriteStream.getStream(attributesSize);

			// Build query string and add row
			ss << player->getGUID() << ',' << parentId << ',' << runningId << ',' << item->getID() << ',' << item->getSubType() << ',' << db.escapeBlob(attributes, static_cast<uint32_t>(attributesSize));
			if (!query_insert.addRow(ss)) {
				g_logger().error("Error adding row to query for container item.");
				return false;
			}
		}

		// Removes the object after processing everything, avoiding memory usage after freeing
		queue.pop_front();
	}

	// Execute query
	if (!query_insert.execute()) {
		g_logger().error("Error executing query.");
		return false;
	}
	return true;
}

bool IOLoginDataSave::savePlayerFirst(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	if (player->getHealth() <= 0) {
		player->changeHealth(1);
	}

	savePlayerSystems(player);

	Database &db = Database::getInstance();

	DBResult_ptr result = db.storeQuery(
		"SELECT `save` FROM `players` WHERE `id` = ?",
		std::vector<QueryParamVariant>{ player->getGUID() }
	);
	if (!result) {
		g_logger().warn("[IOLoginData::savePlayer] - Error for select result query from player: {}", player->getName());
		return false;
	}

	if (result->getNumber<uint16_t>("save") == 0) {
		return db.executeQuery(
			"UPDATE `players` SET `lastlogin` = ?, `lastip` = ? WHERE `id` = ?",
			{ player->lastLoginSaved, player->lastIP, player->getGUID() }
		);
	}

	// serialize conditions
	PropWriteStream propWriteStream;
	for (const auto &condition : player->conditions) {
		if (condition->isPersistent()) {
			condition->serialize(propWriteStream);
			propWriteStream.write<uint8_t>(CONDITIONATTR_END);
		}
	}

	size_t attributesSize;
	const char* attributes = propWriteStream.getStream(attributesSize);

	// serialize animus mastery
	PropWriteStream propAnimusMasteryStream;
	player->animusMastery().serialize(propAnimusMasteryStream);
	size_t animusMasterySize;
	const char* animusMastery = propAnimusMasteryStream.getStream(animusMasterySize);

	int64_t skullTime = 0;
	Skulls_t skull = SKULL_NONE;
	if (g_game().getWorldType() != WORLD_TYPE_PVP_ENFORCED) {
		if (player->skullTicks > 0) {
			auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			skullTime = now + player->skullTicks;
		}

		if (player->skull == SKULL_RED) {
			skull = SKULL_RED;
		} else if (player->skull == SKULL_BLACK) {
			skull = SKULL_BLACK;
		}
	}

	auto now = std::chrono::system_clock::now();
	auto lastLoginSavedDuration = std::chrono::system_clock::from_time_t(player->lastLoginSaved);
	auto onlineTimeAdd = std::chrono::duration_cast<std::chrono::seconds>(now - lastLoginSavedDuration).count();

	if (!db.executeQuery(
			"UPDATE `players` SET "
			"`name` = ?, `level` = ?, `group_id` = ?, `vocation` = ?, `health` = ?, `healthmax` = ?, `experience` = ?, "
			"`lookbody` = ?, `lookfeet` = ?, `lookhead` = ?, `looklegs` = ?, `looktype` = ?, `lookaddons` = ?, "
			"`lookmountbody` = ?, `lookmountfeet` = ?, `lookmounthead` = ?, `lookmountlegs` = ?, `lookfamiliarstype` = ?, "
			"`isreward` = ?, `maglevel` = ?, `mana` = ?, `manamax` = ?, `manaspent` = ?, `soul` = ?, `town_id` = ?, "
			"`posx` = ?, `posy` = ?, `posz` = ?, `prey_wildcard` = ?, `task_points` = ?, `boss_points` = ?, "
			"`forge_dusts` = ?, `forge_dust_level` = ?, `randomize_mount` = ?, `cap` = ?, `sex` = ?, `lastlogin` = ?, `lastip` = ?, "
			"`conditions` = ?, `animus_mastery` = ?, `skulltime` = ?, `skull` = ?, `lastlogout` = ?, `balance` = ?, "
			"`offlinetraining_time` = ?, `offlinetraining_skill` = ?, `stamina` = ?, "
			"`skill_fist` = ?, `skill_fist_tries` = ?, `skill_club` = ?, `skill_club_tries` = ?, `skill_sword` = ?, `skill_sword_tries` = ?, "
			"`skill_axe` = ?, `skill_axe_tries` = ?, `skill_dist` = ?, `skill_dist_tries` = ?, `skill_shielding` = ?, `skill_shielding_tries` = ?, "
			"`skill_fishing` = ?, `skill_fishing_tries` = ?, `skill_critical_hit_chance` = ?, `skill_critical_hit_chance_tries` = ?, "
			"`skill_critical_hit_damage` = ?, `skill_critical_hit_damage_tries` = ?, `skill_life_leech_chance` = ?, `skill_life_leech_chance_tries` = ?, "
			"`skill_life_leech_amount` = ?, `skill_life_leech_amount_tries` = ?, `skill_mana_leech_chance` = ?, `skill_mana_leech_chance_tries` = ?, "
			"`skill_mana_leech_amount` = ?, `skill_mana_leech_amount_tries` = ?, `manashield` = ?, `max_manashield` = ?, "
			"`xpboost_value` = ?, `xpboost_stamina` = ?, `quickloot_fallback` = ?, `onlinetime` = `onlinetime` + ?, "
			"`blessings1` = ?, `blessings2` = ?, `blessings3` = ?, `blessings4` = ?, `blessings5` = ?, `blessings6` = ?, `blessings7` = ?, `blessings8` = ? "
			"WHERE `id` = ?",
			{
				player->name, player->level, player->group->id, player->getVocationId(), player->health, player->healthMax, player->experience,
				static_cast<uint32_t>(player->defaultOutfit.lookBody), static_cast<uint32_t>(player->defaultOutfit.lookFeet), static_cast<uint32_t>(player->defaultOutfit.lookHead), static_cast<uint32_t>(player->defaultOutfit.lookLegs), player->defaultOutfit.lookType, static_cast<uint32_t>(player->defaultOutfit.lookAddons),
				static_cast<uint32_t>(player->defaultOutfit.lookMountBody), static_cast<uint32_t>(player->defaultOutfit.lookMountFeet), static_cast<uint32_t>(player->defaultOutfit.lookMountHead), static_cast<uint32_t>(player->defaultOutfit.lookMountLegs), player->defaultOutfit.lookFamiliarsType,
				static_cast<uint16_t>(player->isDailyReward), player->magLevel, player->mana, player->manaMax, player->manaSpent, static_cast<uint16_t>(player->soul), (player->town ? player->town->getID() : 0),
				player->getLoginPosition().getX(), player->getLoginPosition().getY(), player->getLoginPosition().getZ(), player->getPreyCards(), player->getTaskHuntingPoints(), player->getBossPoints(),
				player->getForgeDusts(), player->getForgeDustLevel(), static_cast<uint16_t>(player->isRandomMounted()), (player->capacity / 100), static_cast<uint16_t>(player->sex), player->lastLoginSaved, player->lastIP,
				std::string(reinterpret_cast<const char*>(attributes), attributesSize), std::string(reinterpret_cast<const char*>(animusMastery), animusMasterySize), skullTime, static_cast<int64_t>(skull), player->getLastLogout(), player->bankBalance,
				player->getOfflineTrainingTime() / 1000, std::to_string(player->getOfflineTrainingSkill()), player->getStaminaMinutes(),
				player->skills[SKILL_FIST].level, player->skills[SKILL_FIST].tries, player->skills[SKILL_CLUB].level, player->skills[SKILL_CLUB].tries, player->skills[SKILL_SWORD].level, player->skills[SKILL_SWORD].tries,
				player->skills[SKILL_AXE].level, player->skills[SKILL_AXE].tries, player->skills[SKILL_DISTANCE].level, player->skills[SKILL_DISTANCE].tries, player->skills[SKILL_SHIELD].level, player->skills[SKILL_SHIELD].tries,
				player->skills[SKILL_FISHING].level, player->skills[SKILL_FISHING].tries, player->skills[SKILL_CRITICAL_HIT_CHANCE].level, player->skills[SKILL_CRITICAL_HIT_CHANCE].tries,
				player->skills[SKILL_CRITICAL_HIT_DAMAGE].level, player->skills[SKILL_CRITICAL_HIT_DAMAGE].tries, player->skills[SKILL_LIFE_LEECH_CHANCE].level, player->skills[SKILL_LIFE_LEECH_CHANCE].tries,
				player->skills[SKILL_LIFE_LEECH_AMOUNT].level, player->skills[SKILL_LIFE_LEECH_AMOUNT].tries, player->skills[SKILL_MANA_LEECH_CHANCE].level, player->skills[SKILL_MANA_LEECH_CHANCE].tries,
				player->skills[SKILL_MANA_LEECH_AMOUNT].level, player->skills[SKILL_MANA_LEECH_AMOUNT].tries, player->getManaShield(), player->getMaxManaShield(),
				player->getXpBoostPercent(), player->getXpBoostTime(), (player->quickLootFallbackToMainContainer ? 1 : 0), (player->isOffline() ? 0 : onlineTimeAdd),
				static_cast<uint32_t>(player->getBlessingCount(1)), static_cast<uint32_t>(player->getBlessingCount(2)), static_cast<uint32_t>(player->getBlessingCount(3)), static_cast<uint32_t>(player->getBlessingCount(4)),
				static_cast<uint32_t>(player->getBlessingCount(5)), static_cast<uint32_t>(player->getBlessingCount(6)), static_cast<uint32_t>(player->getBlessingCount(7)), static_cast<uint32_t>(player->getBlessingCount(8)),
				player->getGUID()
			}
		)) {
		return false;
	}
	return true;
}

bool IOLoginDataSave::savePlayerStash(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	Database &db = Database::getInstance();
	if (!db.executeQuery(
			"DELETE FROM `player_stash` WHERE `player_id` = ?",
			{ player->getGUID() }
		)) {
		return false;
	}

	DBInsert stashQuery("INSERT INTO `player_stash` (`player_id`,`item_id`,`item_count`) VALUES ");
	for (const auto &[itemId, itemCount] : player->getStashItems()) {
		const ItemType &itemType = Item::items[itemId];
		if (itemType.decayTo >= 0 && itemType.decayTime > 0) {
			continue;
		}

		auto wareId = itemType.wareId;
		if (wareId > 0 && wareId != itemType.id) {
			g_logger().warn("[{}] - Item ID {} is a ware item, for player: {}, skipping.", __FUNCTION__, itemId, player->getName());
			continue;
		}

		if (!stashQuery.addRow(fmt::format("{},{},{}", player->getGUID(), itemId, itemCount))) {
			return false;
		}
	}

	if (!stashQuery.execute()) {
		return false;
	}
	return true;
}

bool IOLoginDataSave::savePlayerSpells(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	Database &db = Database::getInstance();
	if (!db.executeQuery(
			"DELETE FROM `player_spells` WHERE `player_id` = ?",
			{ player->getGUID() }
		)) {
		return false;
	}

	DBInsert spellsQuery("INSERT INTO `player_spells` (`player_id`, `name` ) VALUES ");
	for (const std::string &spellName : player->learnedInstantSpellList) {
		if (!spellsQuery.addRow(fmt::format("{},{}", player->getGUID(), db.escapeString(spellName)))) {
			return false;
		}
	}

	if (!spellsQuery.execute()) {
		return false;
	}
	return true;
}

bool IOLoginDataSave::savePlayerKills(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	Database &db = Database::getInstance();
	if (!db.executeQuery(
			"DELETE FROM `player_kills` WHERE `player_id` = ?",
			{ player->getGUID() }
		)) {
		return false;
	}

	DBInsert killsQuery("INSERT INTO `player_kills` (`player_id`, `target`, `time`, `unavenged`) VALUES");
	for (const auto &kill : player->unjustifiedKills) {
		if (!killsQuery.addRow(fmt::format("{},{},{},{}", player->getGUID(), kill.target, kill.time, kill.unavenged))) {
			return false;
		}
	}

	if (!killsQuery.execute()) {
		return false;
	}
	return true;
}

bool IOLoginDataSave::savePlayerBestiarySystem(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	Database &db = Database::getInstance();

	PropWriteStream charmsStream;
	for (uint8_t id = magic_enum::enum_value<charmRune_t>(1); id <= magic_enum::enum_count<charmRune_t>(); id++) {
		const auto &charm = player->charmsArray[id];
		charmsStream.write<uint16_t>(charm.raceId);
		charmsStream.write<uint8_t>(charm.tier);
		g_logger().debug("Player {} saved raceId {} and tier {} to charm Id {}", player->name, charm.raceId, charm.tier, id);
	}
	size_t charmsSize;
	const char* charmsList = charmsStream.getStream(charmsSize);

	PropWriteStream propBestiaryStream;
	for (const auto &trackedType : player->getCyclopediaMonsterTrackerSet(false)) {
		propBestiaryStream.write<uint16_t>(trackedType->info.raceid);
	}
	size_t trackerSize;
	const char* trackerList = propBestiaryStream.getStream(trackerSize);

	if (!db.executeQuery(
			"UPDATE `player_charms` SET "
			"`charm_points` = ?, `minor_charm_echoes` = ?, `max_charm_points` = ?, `max_minor_charm_echoes` = ?, "
			"`charm_expansion` = ?, `UsedRunesBit` = ?, `UnlockedRunesBit` = ?, `charms` = ?, `tracker list` = ? "
			"WHERE `player_id` = ?",
			{
				player->charmPoints, player->minorCharmEchoes, player->maxCharmPoints, player->maxMinorCharmEchoes,
				(player->charmExpansion ? 1 : 0), player->UsedRunesBit, player->UnlockedRunesBit,
				std::string(reinterpret_cast<const char*>(charmsList), charmsSize), std::string(reinterpret_cast<const char*>(trackerList), trackerSize),
				player->getGUID()
			}
		)) {
		g_logger().warn("[IOLoginData::savePlayer] - Error saving bestiary data from player: {}", player->getName());
		return false;
	}
	return true;
}

bool IOLoginDataSave::savePlayerItem(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	Database &db = Database::getInstance();
	PropWriteStream propWriteStream;
	if (!db.executeQuery(
			"DELETE FROM `player_items` WHERE `player_id` = ?",
			{ player->getGUID() }
		)) {
		g_logger().warn("[IOLoginData::savePlayer] - Error delete query 'player_items' from player: {}", player->getName());
		return false;
	}
	DBInsert itemsQuery("INSERT INTO `player_items` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");

	ItemBlockList itemList;
	for (int32_t slotId = CONST_SLOT_FIRST; slotId <= CONST_SLOT_LAST; ++slotId) {
		const auto &item = player->inventory[slotId];
		if (item) {
			itemList.emplace_back(slotId, item);
		}
	}

	if (!saveItems(player, itemList, itemsQuery, propWriteStream)) {
		g_logger().warn("[IOLoginData::savePlayer] - Failed for save items from player: {}", player->getName());
		return false;
	}
	return true;
}

bool IOLoginDataSave::savePlayerDepotItems(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	Database &db = Database::getInstance();
	PropWriteStream propWriteStream;
	ItemDepotList depotList;
	if (player->lastDepotId != -1) {
		if (!db.executeQuery(
				"DELETE FROM `player_depotitems` WHERE `player_id` = ?",
				{ player->getGUID() }
			)) {
			return false;
		}

		DBInsert depotQuery("INSERT INTO `player_depotitems` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");

		for (const auto &[pid, depotChest] : player->depotChests) {
			for (const std::shared_ptr<Item> &item : depotChest->getItemList()) {
				depotList.emplace_back(pid, item);
			}
		}

		if (!saveItems(player, depotList, depotQuery, propWriteStream)) {
			return false;
		}
		return true;
	}
	return true;
}

bool IOLoginDataSave::saveRewardItems(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	if (!Database::getInstance().executeQuery(
			"DELETE FROM `player_rewards` WHERE `player_id` = ?",
			{ player->getGUID() }
		)) {
		return false;
	}

	std::vector<uint64_t> rewardList;
	player->getRewardList(rewardList);

	ItemRewardList rewardListItems;
	if (!rewardList.empty()) {
		for (const auto &rewardId : rewardList) {
			auto reward = player->getReward(rewardId, false);
			if (!reward->empty() && (getTimeMsNow() - rewardId <= 1000 * 60 * 60 * 24 * 7)) {
				rewardListItems.emplace_back(0, reward);
			}
		}

		DBInsert rewardQuery("INSERT INTO `player_rewards` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");
		PropWriteStream propWriteStream;
		if (!saveItems(player, rewardListItems, rewardQuery, propWriteStream)) {
			return false;
		}
	}
	return true;
}

bool IOLoginDataSave::savePlayerInbox(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	Database &db = Database::getInstance();
	PropWriteStream propWriteStream;
	ItemInboxList inboxList;
	if (!db.executeQuery(
			"DELETE FROM `player_inboxitems` WHERE `player_id` = ?",
			{ player->getGUID() }
		)) {
		return false;
	}

	DBInsert inboxQuery("INSERT INTO `player_inboxitems` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");

	for (const auto &item : player->getInbox()->getItemList()) {
		inboxList.emplace_back(0, item);
	}

	if (!saveItems(player, inboxList, inboxQuery, propWriteStream)) {
		return false;
	}
	return true;
}

bool IOLoginDataSave::savePlayerPreyClass(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	if (g_configManager().getBoolean(PREY_ENABLED)) {
		DBInsert preyQuery("INSERT INTO `player_prey` (`player_id`, `slot`, `state`, `raceid`, `option`, `bonus_type`, `bonus_rarity`, `bonus_percentage`, `bonus_time`, `free_reroll`, `monster_list`) VALUES ");
		preyQuery.upsert({ "state", "raceid", "option", "bonus_type", "bonus_rarity", "bonus_percentage", "bonus_time", "free_reroll", "monster_list" });

		for (uint8_t slotId = PreySlot_First; slotId <= PreySlot_Last; slotId++) {
			if (const auto &slot = player->getPreySlotById(static_cast<PreySlot_t>(slotId))) {
				PropWriteStream propPreyStream;
				[[maybe_unused]] auto lambda = std::ranges::for_each(slot->raceIdList, [&propPreyStream](uint16_t raceId) {
					propPreyStream.write<uint16_t>(raceId);
				});

				size_t preySize;
				const char* preyList = propPreyStream.getStream(preySize);

				if (!preyQuery.addRow(fmt::format("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
												  player->getGUID(),
												  static_cast<uint16_t>(slot->id),
												  static_cast<uint16_t>(slot->state),
												  slot->selectedRaceId,
												  static_cast<uint16_t>(slot->option),
												  static_cast<uint16_t>(slot->bonus),
												  static_cast<uint16_t>(slot->bonusRarity),
												  slot->bonusPercentage,
												  slot->bonusTimeLeft,
												  slot->freeRerollTimeStamp,
												  Database::getInstance().escapeBlob(preyList, static_cast<uint32_t>(preySize))))) {
					g_logger().warn("[IOLoginData::savePlayer] - Error adding row to prey slot data query for player: {}", player->getName());
					return false;
				}
			}
		}

		if (!preyQuery.execute()) {
			g_logger().warn("[IOLoginData::savePlayer] - Error saving prey slot data from player: {}", player->getName());
			return false;
		}
	}
	return true;
}

bool IOLoginDataSave::savePlayerTaskHuntingClass(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	if (g_configManager().getBoolean(TASK_HUNTING_ENABLED)) {
		DBInsert taskHuntingQuery("INSERT INTO `player_taskhunt` (`player_id`, `slot`, `state`, `raceid`, `upgrade`, `rarity`, `kills`, `disabled_time`, `free_reroll`, `monster_list`) VALUES ");
		taskHuntingQuery.upsert({ "state", "raceid", "upgrade", "rarity", "kills", "disabled_time", "free_reroll", "monster_list" });

		for (uint8_t slotId = PreySlot_First; slotId <= PreySlot_Last; slotId++) {
			if (const auto &slot = player->getTaskHuntingSlotById(static_cast<PreySlot_t>(slotId))) {
				PropWriteStream propTaskHuntingStream;
				[[maybe_unused]] auto lambda = std::ranges::for_each(slot->raceIdList, [&propTaskHuntingStream](uint16_t raceId) {
					propTaskHuntingStream.write<uint16_t>(raceId);
				});

				size_t taskHuntingSize;
				const char* taskHuntingList = propTaskHuntingStream.getStream(taskHuntingSize);

				if (!taskHuntingQuery.addRow(fmt::format("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
														 player->getGUID(),
														 static_cast<uint16_t>(slot->id),
														 static_cast<uint16_t>(slot->state),
														 slot->selectedRaceId,
														 (slot->upgrade ? 1 : 0),
														 static_cast<uint16_t>(slot->rarity),
														 slot->currentKills,
														 slot->disabledUntilTimeStamp,
														 slot->freeRerollTimeStamp,
														 Database::getInstance().escapeBlob(taskHuntingList, static_cast<uint32_t>(taskHuntingSize))))) {
					g_logger().warn("[IOLoginData::savePlayer] - Error adding row to task hunting slot data query for player: {}", player->getName());
					return false;
				}
			}
		}

		if (!taskHuntingQuery.execute()) {
			g_logger().warn("[IOLoginData::savePlayer] - Error saving task hunting slot data from player: {}", player->getName());
			return false;
		}
	}
	return true;
}

bool IOLoginDataSave::savePlayerForgeHistory(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginDataSave::savePlayerForgeHistory] - Player nullptr");
		return false;
	}

	return player->forgeHistory().save();
}

bool IOLoginDataSave::savePlayerBosstiary(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[IOLoginData::savePlayer] - Player nullptr: {}", __FUNCTION__);
		return false;
	}

	if (!Database::getInstance().executeQuery(
			"DELETE FROM `player_bosstiary` WHERE `player_id` = ?",
			{ player->getGUID() }
		)) {
		return false;
	}

	DBInsert insertQuery("INSERT INTO `player_bosstiary` (`player_id`, `bossIdSlotOne`, `bossIdSlotTwo`, `removeTimes`, `tracker`) VALUES");

	// Bosstiary tracker
	PropWriteStream stream;
	for (const auto &monsterType : player->getCyclopediaMonsterTrackerSet(true)) {
		if (!monsterType) {
			continue;
		}

		stream.write<uint16_t>(monsterType->info.raceid);
	}
	size_t size;
	const char* chars = stream.getStream(size);

	if (!insertQuery.addRow(fmt::format("{},{},{},{},{}",
										player->getGUID(),
										player->getSlotBossId(1),
										player->getSlotBossId(2),
										player->getRemoveTimes(),
										Database::getInstance().escapeBlob(chars, static_cast<uint32_t>(size))))) {
		return false;
	}

	if (!insertQuery.execute()) {
		return false;
	}

	return true;
}

bool IOLoginDataSave::savePlayerStorage(const std::shared_ptr<Player> &player) {
	if (!player) {
		g_logger().warn("[{}] - Player nullptr", __FUNCTION__);
		return false;
	}

	auto &storage = player->storage();
	storage.prepareForPersist();
	auto delta = storage.delta();
	auto guid = player->getGUID();
	auto &repo = g_playerStorageRepository();

	if (!delta.deletions.empty() && !repo.deleteKeys(guid, delta.deletions)) {
		return false;
	}

	if (!delta.upserts.empty() && !repo.upsert(guid, delta.upserts)) {
		return false;
	}

	storage.clearDirty();
	return true;
}

void IOLoginDataSave::savePlayerSystems(const std::shared_ptr<Player> &player) {
	if (!player) {
		return;
	}

	// Save the player's Virtue to persistent storage if it's set
	auto virtue = player->getVirtue();
	if (virtue > Virtue_t::None) {
		player->kv()->scoped("spells")->set("virtue", static_cast<uint8_t>(virtue));
	}

	auto harmony = player->getHarmony();
	if (harmony > 0) {
		player->kv()->scoped("spells")->set("harmony", harmony);
	}
}