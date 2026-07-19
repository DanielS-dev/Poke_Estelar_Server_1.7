balls = {
pokeball = {emptyId = 59268, usedOn = 59270, usedOff = 59268, effectFail = 350, effectSucceed = 351, missile = 192, effectRelease = 352, chanceMultiplier = 1.0}
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

local function roundNumber(value)
	return math.floor(value + 0.5)
end

local function getLevelScaledStatus(baseValue, level, perLevelBuff, minimumValue)
	local currentLevel = math.max(1, tonumber(level) or 1)
	local scaledValue = roundNumber(baseValue * (1.0 + ((currentLevel - 1) * perLevelBuff)))
	if minimumValue ~= nil then
		return math.max(minimumValue, scaledValue)
	end
	return scaledValue
end

local function getPokeballMaxHealth(monsterType, level)
	return getLevelScaledStatus(monsterType:maxHealth(), level, 0.10, 1)
end

local function getPokeballArmor(monsterType, level)
	return getLevelScaledStatus(monsterType:armor(), level, 0.05, 0)
end

local function getPokeballDefense(monsterType, level)
	return getLevelScaledStatus(monsterType:defense(), level, 0.05, 0)
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
			local maxHealth = getPokeballMaxHealth(monsterType, level)
			addBall:setCustomAttribute("pokeName", name)
			addBall:setCustomAttribute("pokeLevel", level)
			addBall:setCustomAttribute("pokeBoost", boost)
			--addBall:setAttribute("pokeExperience", getNeededExp(level))
			addBall:setCustomAttribute("pokeMaxHealth", maxHealth)
			addBall:setCustomAttribute("pokeHealth", maxHealth)
			addBall:setCustomAttribute("pokeArmor", getPokeballArmor(monsterType, level))
			addBall:setCustomAttribute("pokeDefense", getPokeballDefense(monsterType, level))
			addBall:setCustomAttribute("pokeIsDead", 0)
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
			schedulePokemonBarUpdate(player, 100)
			
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

function Player:canReleaseSummon(pokeLevel, pokeBoost, ownerName)

	-- to fix ball bug
	if not pokeLevel then return false end

	--antiga usaga a quantidade de InsÃ­gnias
	--local playerLevel = self:getLevel() + (10 * getBadgeQuantity(self))
	local playerLevel = self:getLevel()
	local minimumLevel = (pokeLevel + pokeBoost) - 10

	if playerLevel < minimumLevel then
		self:sendCancelMessage("Sorry, not possible. You need level " .. minimumLevel .. " to use this pokemon.")
		return false
	end

	if ownerName then
		if self:getStorageValue(quests.cathemAll.prizes[1].uid) < 2 then
			print("WARNING! Player " .. self:getName() .. " using unique Pokemon without finish the quest!")
		end
	end

	if ownerName and ownerName ~= self:getName() then
		self:sendCancelMessage("Sorry, it belongs to " .. ownerName .. ".")
		return false
	end

	return true
end

function Monster.getSummonLevel(self)
	if self:isPokemon() then
		local master = self:getMaster()
		local item = master:getUsingBall()
		local pokeLevel = item:getCustomAttribute("pokeLevel")
		if pokeLevel ~= nil then			
			return pokeLevel
		end
	elseif isMonster(self) then
		return self:getLevel()
	end
	return 1
end

function Monster.getSummonName(self)
	if isSummon(self) then
		local master = self:getMaster()
--		local item = master:getSlotItem(CONST_SLOT_AMMO)
		local item = master:getUsingBall()
		local pokeName = item:getCustomAttribute("pokeName")
		if pokeName ~= nil then			
			return pokeName
		end
	end
	return "unamed"
end

function Item:getSummonLevel()
	return self:getCustomAttribute("pokeLevel")
end

function Item:getSummonBoost()
	return self:getCustomAttribute("pokeBoost")
end

function Item:getSummonOwner()
	return self:getCustomAttribute("owner")
end

function Monster.getTotalSpeed(self)
	-- ADICIONE ESTE BLOCO ABAIXO BEM NO INÃCIO DO MÃ‰TODO:
    local ball = nil
    local master = self:getMaster()
    if master and master:isPlayer() then
        ball = master:getUsingBall()
    end
    
    -- Se por acaso nÃ£o achar a pokÃ©bola, define uma velocidade padrÃ£o para nÃ£o quebrar
    if not ball then 
        return 250 
    end

	local monsterType = MonsterType(self:getName())
	if self:isPokemon() then 
		local total = math.floor(monsterType:getBaseSpeed() + (2 * self:getLevel()))
		-- CÃ“DIGO CORRIGIDO COM PROTEÃ‡ÃƒO:
		local vitamins = (ball.getUsedVitaminsNumber and ball:getUsedVitaminsNumber("speed")) or 0
		if vitamins > 0 then
			total = total + math.floor(monsterType:getBaseSpeed() * vitamins / maxVitamins * vitaminStatusBuff)
		end
		return total
	elseif self:isMonster() then
		return math.floor(monsterType:getBaseSpeed() + (2 * self:getLevel()))
	end
	return 0
end

function Monster.getTotalHealth(self)
	local level = 1 -- Valor padrÃ£o de seguranÃ§a
    
		-- Descobre quem Ã© o dono do PokÃ©mon
		local master = self:getMaster()
		if master and master:isPlayer() then
			-- Pega a pokÃ©bola ativa que estÃ¡ na bag/slot do jogador
			local ball = master:getUsingBall()
			if ball then
				-- LÃª o level que salvamos na pokÃ©bola lÃ¡ no catch!
				level = ball:getCustomAttribute("pokeLevel") or 1
			end
		end

	local monsterType = MonsterType(self:getName())
	if self:isPokemon() then -- Life Formula
		local hp = monsterType:getMaxHealth()

		local lv = self:getLevel()
		local total = math.floor((lv * 20) + (hp * 0.10)) + hp
		if self:getMaster() and self:getMaster():getUsingBall() and self:getMaster():getUsingBall():getCustomAttribute("nature") then
			if isInArray({"Timid", "Hasty", "Jolly", "Naive"}, self:getMaster():getUsingBall():getCustomAttribute("nature")) then
				total = total * 1.10
			elseif isInArray({"Brave", "Relaxed", "Quiet", "Sassy"}, self:getMaster():getUsingBall():getCustomAttribute("nature")) then
				total = total * 0.90
			end
		end
		if self:getMaster():getVocation():getName() == "Blocker" then
			total = total * blockerHealthBuff
		end

		return total
	elseif self:isMonster() then
		local hp = self:getMaxHealth()
		local lv = self:getLevel()
		local formula = ((lv * 20) + (hp * 0.10)) + hp
		return math.floor(formula)
	end
	return 0
end

-- Registro correto do mÃ©todo getLevel para a classe Monster no TFS 1.7
function Monster.getLevel(self)
    -- Descobre quem Ã© o dono desse PokÃ©mon
    local master = self:getMaster()
    if master and master:isPlayer() then
        -- Busca a pokÃ©bola que estÃ¡ ativa com o jogador
        local ball = master:getUsingBall()
        if ball then
            -- LÃª o level salvo na pokÃ©bola (onde o Item sim possui getCustomAttribute)
            return ball:getCustomAttribute("pokeLevel") or 1
        end
    end
    return 1 -- Valor padrÃ£o de seguranÃ§a caso nÃ£o encontre
end

function doRemoveSummon(cid, effect, message, safeRemove, missile)
	local player = Player(cid)
	if not player then
		return false
	end

	local ball = player:getUsingBall()
	if not ball then
		return false
	end

	local summonId = ball:getCustomAttribute("summonId")
	local summon = summonId and Monster(summonId) or nil
	if not summon or summon:getMaster() ~= player then
		local summons = player:getSummons()
		summon = summons and summons[1]
	end

	if not summon then
		ball:setCustomAttribute("isBeingUsed", 0)
		ball:removeAttribute("summonId")
		schedulePokemonBarUpdate(player, 50)
		return false
	end

	local health = summon:getHealth()
	if health < 0 then
		health = 0
	end

	ball:setCustomAttribute("pokeHealth", health)
	ball:setCustomAttribute("pokeMaxHealth", summon:getMaxHealth())
	ball:setCustomAttribute("pokeLookDir", summon:getDirection())
	ball:setCustomAttribute("isBeingUsed", 0)
	ball:removeAttribute("summonId")

	if effect then
		summon:getPosition():sendMagicEffect(effect)
	end

	if missile then
		doSendDistanceShoot(summon:getPosition(), player:getPosition(), missile)
	end

	if message ~= false then
		player:say("Volte!", TALKTYPE_MONSTER_SAY)
	end

	local summonGuid = summon:getId()
	summon:setTarget(nil)
	summon:setFollowCreature(nil)
	player:removeSummon(summon)
	addEvent(doRemoveCreature, 1, summonGuid)
	schedulePokemonBarUpdate(player, 50)
	return true
end

local function isValidSummonTile(player, position)
	local tile = Tile(position)
	if not tile or not tile:isWalkable() then
		return false
	end

	if tile:getCreatureCount() > 0 then
		return false
	end

	return tile:queryAdd(player, FLAG_IGNOREBLOCKCREATURE) == RETURNVALUE_NOERROR
end

local function getValidSummonPosition(player, basePosition)
	local directions = {
		player:getDirection(),
		DIRECTION_NORTH,
		DIRECTION_EAST,
		DIRECTION_SOUTH,
		DIRECTION_WEST
	}

	for _, direction in ipairs(directions) do
		local testPos = Position(basePosition)
		testPos:getNextPosition(direction)
		if isValidSummonTile(player, testPos) then
			return testPos
		end
	end

	local fallbackPos = player:getClosestFreePosition(basePosition, 2)
	if fallbackPos and fallbackPos.x ~= 0 and isValidSummonTile(player, fallbackPos) then
		return fallbackPos
	end

	return nil
end

function doReleaseSummon(cid, pos, effect, message, missile)
	local player = Player(cid)
	if not player then return false end
	local ball = player:getUsingBall()
	if not ball then return false end
	if effect == nil then effect = CONST_ME_TELEPORT end
	if message == nil then message = true end
	local name = ball:getCustomAttribute("pokeName")
	if not name then
		ball:setCustomAttribute("isBeingUsed", 0)
		return false
	end

	local monsterType = MonsterType(name)
	if not monsterType then
		player:sendCancelMessage("Sorry, not possible. Monster invalid.")
		ball:setCustomAttribute("isBeingUsed", 0)
		schedulePokemonBarUpdate(player, 50)
		return false
	end

	if ball:getCustomAttribute("pokeIsDead") == 1 then
		player:sendCancelMessage("Sorry, not possible. Your summon is dead.")
		ball:setCustomAttribute("isBeingUsed", 0)
		schedulePokemonBarUpdate(player, 50)
		return true
	end

	local health = ball:getCustomAttribute("pokeHealth") or 0
	local pokeLevel = ball:getCustomAttribute("pokeLevel") or 1
	if health <= 0 then
		player:sendCancelMessage("Sorry, not possible. Your summon is dead.")
		ball:setCustomAttribute("pokeIsDead", 1)
		ball:setCustomAttribute("isBeingUsed", 0)
		schedulePokemonBarUpdate(player, 50)
		return true
	end	

	local newPos = getValidSummonPosition(player, player:getPosition()) or getValidSummonPosition(player, pos)
	if not newPos then
		player:sendCancelMessage("Sorry, not possible to call your summon here.")
		ball:setCustomAttribute("isBeingUsed", 0)
		schedulePokemonBarUpdate(player, 50)
		return false
	end

	local monster = Game.createMonster(name, newPos, true, true, CONST_ME_NONE, pokeLevel)
	if not monster then
		ball:setCustomAttribute("isBeingUsed", 0)
		schedulePokemonBarUpdate(player, 50)
    	return false
 	end

	if message then
		player:say(monster:getName() .. ", venha me ajudar!", TALKTYPE_MONSTER_SAY)
	end

	player:addSummon(monster)
	monster:setDirection(ball:getCustomAttribute("pokeLookDir") or DIRECTION_SOUTH)
	ball:setCustomAttribute("summonId", monster:getId())
	ball:setCustomAttribute("pokeIsDead", 0)
	monster:setTarget(nil)
	monster:setFollowCreature(player)
	monster:registerEvent("PokeSummonDeath")
	monster:registerEvent("PokeSummonHealth")

	local maxHealth = monster:getMaxHealth()
	local currentHealth = math.min(health, maxHealth)
	ball:setCustomAttribute("pokeMaxHealth", maxHealth)
	ball:setCustomAttribute("pokeHealth", currentHealth)
	ball:setCustomAttribute("pokeArmor", getPokeballArmor(monsterType, pokeLevel))
	ball:setCustomAttribute("pokeDefense", getPokeballDefense(monsterType, pokeLevel))
	monster:setHealth(currentHealth)
	schedulePokemonBarUpdate(player, 50)

	if effect then
		monster:getPosition():sendMagicEffect(effect)
	end
	if missile then
		doSendDistanceShoot(player:getPosition(), monster:getPosition(), missile)
	end

	monster:removeTarget(player)
	return monster:getId()
end


