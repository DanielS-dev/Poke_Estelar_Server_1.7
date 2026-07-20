local MAIN_BAG_ID = 59271

local function ensureMainBag(player)
	local backpack = player:getSlotItem(CONST_SLOT_BACKPACK)
	if backpack and backpack:getId() == MAIN_BAG_ID then
		return backpack
	end

	if not backpack then
		return player:addItem(MAIN_BAG_ID, 1, false, CONST_SLOT_BACKPACK)
	end

	local items = {}
	for slot = backpack:getSize() - 1, 0, -1 do
		local item = backpack:getItem(slot)
		if item then
			items[#items + 1] = item
		end
	end

	backpack:remove()

	local mainBag = player:addItem(MAIN_BAG_ID, 1, false, CONST_SLOT_BACKPACK)
	if not mainBag then
		return nil
	end

	for i = 1, #items do
		items[i]:moveTo(mainBag)
	end

	return mainBag
end

function onLogin(player)
	local serverName = configManager.getString(configKeys.SERVER_NAME)
	local loginStr = "Welcome to " .. serverName .. "!"
	if player:getLastLoginSaved() <= 0 then
		loginStr = loginStr .. " Please choose your outfit."
		player:sendOutfitWindow()
	else
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, loginStr)
		loginStr = string.format("Your last visit in %s: %s.", serverName, os.date("%d %b %Y %X", player:getLastLoginSaved()))
	end
	player:sendTextMessage(MESSAGE_STATUS_DEFAULT, loginStr)

	-- Promotion
	local vocation = player:getVocation()
	local promotion = vocation:getPromotion()
	if player:isPremium() then
		local value = player:getStorageValue(PlayerStorageKeys.promotion)
		if value and value == 1 then
			player:setVocation(promotion)
		end
	elseif not promotion then
		player:setVocation(vocation:getDemotion())
	end

	-- Update client stats
	player:updateClientExpDisplay()
	player:sendHotkeyPreset()
	player:disableLoginMusic()

	-- achievements points for highscores
	if not player:getStorageValue(PlayerStorageKeys.achievementsTotal) then
		player:setStorageValue(PlayerStorageKeys.achievementsTotal, player:getAchievementPoints())
	end

	ensureMainBag(player)

	-- Events
	player:registerEvent("PlayerDeath")
	player:registerEvent("DropLoot")
	player:registerEvent("BestiaryKills")
	schedulePokemonBarUpdate(player, 1200)
	return true
end
