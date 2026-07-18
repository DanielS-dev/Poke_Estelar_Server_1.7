balls = {
pokeball = {emptyId = 59268, usedOn = 59270, usedOff = 26672, effectFail = 1087, effectSucceed = 1088, missile = 192, effectRelease = 1060, chanceMultiplier = 1.0}
}

function getBallKey(uid)
	for key, value in pairs(balls) do
		if uid == value.emptyId or uid == value.usedOn or uid == value.usedOff then
			return key
		end
	end
	return "pokeball"
end

function firstToUpper(str)
    if not str or str == "" then return str end
    return string.upper(string.sub(str, 1, 1)) .. string.sub(str, 2)
end

function doAddPokeball(cid, name, level, boost, ballKey, dp, msg, teratype)
	local player = Player(cid)
	local letter
	local worm
	if player then
		name = firstToUpper(name)
		if string.find(name, "Unown") then
			letter = name:split("|")[2]
			name = "Unown"
		end
		if string.find(name, "Wormadam") then
			worm = name:split("|")[2]
			name = "Wormadam"
		end
		local monsterType = MonsterType(name)
		if not monsterType then 
			print("WARNING: Monster " .. name .. " impossible to catch.") 
			player:sendCancelMessage("Sorry, not possible. This problem was reported.")
			return false 
		end
		local addBall
		if dp == false then
			addBall = player:addItem(balls[ballKey].usedOn, 1, false)
		else
			local depot = player:getInbox()
			addBall = depot:addItem(balls[ballKey].usedOn, 1, INDEX_WHEREEVER, FLAG_NOLIMIT)
		end
		if not addBall then
			if dp == false then -- try send to CP because BP is full
				local depot = player:getInbox()
				addBall = depot:addItem(balls[ballKey].usedOn, 1, INDEX_WHEREEVER, FLAG_NOLIMIT)
				addEvent(doPlayerSendTextMessage, msg, cid, MESSAGE_EVENT_ADVANCE, "Since you are at maximum capacity, your ball was sent to CP.")
--				print("WARNING! Player " .. player:getName() .. " sending pokemon " .. name .. " to CP after first try.")
				dp = true
			else
				addBall = player:addItem(balls[ballKey].usedOn, 1)
				addEvent(doPlayerSendTextMessage, msg, cid, MESSAGE_STATUS_WARNING, "Pokemon lost. Your CP is full!")
				print("WARNING! Player " .. player:getName() .. " lost pokemon " .. name .. " because CP is full.")
			end
		end
		if addBall then
			local baseHealth = monsterType:getMaxHealth()
			local maxHealth = math.floor(baseHealth * statusGainFormula(player:getLevel(), level, boost, 0))
			addBall:setCustomAttribute("pokeName", name)
			addBall:setCustomAttribute("pokeLevel", level)
			addBall:setCustomAttribute("pokeBoost", boost)
			--addBall:setAttribute("pokeExperience", getNeededExp(level))
			addBall:setCustomAttribute("pokeMaxHealth", maxHealth)
			addBall:setCustomAttribute("pokeHealth", maxHealth)
			addBall:setCustomAttribute("evhp", 0)
			addBall:setCustomAttribute("evatk", 0)
			addBall:setCustomAttribute("evdef", 0)
			addBall:setCustomAttribute("evpoints", 0)
			addBall:setCustomAttribute("evkills", 0)
			addBall:setCustomAttribute("pokeLove", 0)
			
			if teratype ~= 0 then
				addBall:setAttribute("teraType", teratype)
			end
			if letter ~= nil then
				addBall:setAttribute("unown", letter)
			end
			if worm ~= nil then
				addBall:setAttribute("wormadam", tonumber(worm))
			end
			
			player:save()
			
			return true
		else
			print("ERROR! Player " .. player:getName() .. " lost pokemon " .. name .. " for unknown reason.")
		end
	end
	print("WARNING! Player not found to add pokeball.")
	return false	
end

function statusGainFormula(playerLevel, summonLevel, summonBoost, pokeLove)
	return (1.0 + summonLevel * summonLevelDamageBuff + playerLevel * playerLevelDamageBuff + summonBoost * summonBoostDamageBuff + pokeLove * summonLoveDamageBuff)
end

summonLevelDamageBuff = 0.008 -- buff due to summon's level
playerLevelDamageBuff = 0.0005 -- buff due to player's level
summonBoostDamageBuff = 0.008 -- buff due to summon's boost
summonLoveDamageBuff = 0.0002