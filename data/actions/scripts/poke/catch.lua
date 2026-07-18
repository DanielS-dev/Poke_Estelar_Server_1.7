local initialLevel = 1
local initialBoost = 0
local multiplierExpFirstNormal = 100
local multiplierExpNormal = 5
local multiplierExpFirstShiny = 3000
local multiplierExpShiny = 1000

local function doPlayerSendEffect(cid, effect)
	local player = Player(cid)
	if player then
		player:getPosition():sendMagicEffect(effect)
	end
	return true
end

local function doPlayerAddExperience(cid, exp)
	local player = Player(cid)
	if player then
		player:addExperience(exp, true)
	end
	return true
end

function onUse(player, item, fromPosition, target, toPosition, isHotkey)
	local tile = Tile(toPosition)
	if not tile or not tile:getTopDownItem() or not ItemType(tile:getTopDownItem():getId()):isCorpse() then
		return false
	end
	if Tile(player:getPosition()):getGround():getId() == 10769 then
		player:sendCancelMessage("Nao e possivel capturar pokemons em um trial.")
		return true
	end
	local targetCorpse = tile:getTopDownItem()

	local owner = targetCorpse:getAttribute(ITEM_ATTRIBUTE_CORPSEOWNER)
	if player:getParty() and not isInArray(getPartyMembers(player), owner) then
		player:sendCancelMessage(tostring(owner))
		return true
	end
	if owner ~= 0 and owner ~= player:getId() and not player:getParty() then
		player:sendCancelMessage("Sorry, not possible. You are not the owner.")
		return true
	end
	local ballKey = getBallKey(item:getId())
	local playerPos = getPlayerPosition(player)
	local d = getDistanceBetween(playerPos, toPosition)
	local delay = d * 80
	local delayMessage = delay + 2800
	local name = targetCorpse:getName()
	if name == "dead human" then		
		playerPos:sendMagicEffect(CONST_ME_POFF)
		return false
	end
	if name == "dead enlightened of the cult" then
		name = "enlightened of the cult"
	elseif name == "slain undead dragon" then
		name = "undead dragon"
	else
		name = string.gsub(name, "the ", "")
		name = string.gsub(name, "remains of ferumbras", "Ferumbras")
		name = string.gsub(name, "remains of", "")
		name = string.gsub(name, " a ", "")
		name = string.gsub(name, " an ", "")
		name = string.gsub(name, "slain ", "")
		name = string.gsub(name, "fainted ", "")
		name = string.gsub(name, "defeated ", "")
		name = string.gsub(name, "dead ", "")
	end
	
	local monsterType = MonsterType(name)
    
	if not monsterType then
		print("WARNING! Monster " .. name .. " with bug on catch!")
		player:sendCancelMessage("Sorry, not possible. This problem was reported.")
		return true
	end
	local monsterNumber = monsterType:getNumber()
    local baseStorageCatches = 0
    local baseStorageTries = 0
	local storageCatch = baseStorageCatches + monsterNumber
	local storageTry = baseStorageTries + monsterNumber
	local level = targetCorpse:getAttribute("corpseLevel") or initialLevel
	local teraType = targetCorpse:getAttribute("teraType") or 0
	local chance = 0

	if level < 10 then
		chance = (100 - level + monsterType:getCatchChance()) * balls[ballKey].chanceMultiplier
	else
		chance = (math.random(1, math.ceil(math.random((monsterType:getCatchChance() / 20), (monsterType:getCatchChance() / 10)))) + (100 - level)) * balls[ballKey].chanceMultiplier
		chance = math.random(1, math.ceil(chance))
	end
	
	-- Unown System
	
	if name == "unown" then
		name = "unown|"..targetCorpse:getSpecialAttribute("unown")
	end
	if name == "wormadam" then
		name = "Wormadam|"..targetCorpse:getSpecialAttribute("wormadam")
	end
	
	if chance == 0 then
		playerPos:sendMagicEffect(CONST_ME_POFF)
		player:sendCancelMessage("Sorry, it is impossible to catch this monster.")
		return true
	end
	
	doSendDistanceShoot(playerPos, toPosition, balls[ballKey].missile)
	item:remove(1)
	targetCorpse:remove()
	if player:getStorageValue(storageTry) < 0 then
		player:setStorageValue(storageTry, 1)
	else
		player:setStorageValue(storageTry, player:getStorageValue(storageTry) + 1)
	end
	
	chance = chance + 1000
	
	if math.random(1, 1000) <= chance or ballKey == "masterball" then
		
		-- Verificar a quantidade de pokemons na bag
		local backpack = player:getSlotItem(CONST_SLOT_BACKPACK)
		local quantityPokeball = 0

		for i = 0, backpack:getSize() - 1 do
			local item = backpack:getItem(i)
			if item and item:getId() == 59270 then
				quantityPokeball = quantityPokeball + 1
			end
		end

		if player:getSlotItem(CONST_SLOT_BACKPACK) and player:getSlotItem(CONST_SLOT_BACKPACK):getEmptySlots() >= 1 and player:getFreeCapacity() >= 1 and quantityPokeball <= 5 then -- add to backpack
			addEvent(doAddPokeball, delayMessage, player:getId(), name, level, initialBoost, ballKey, false, delayMessage, teraType)
		else -- send to cp
			print("Entrou no CP")
			local addPokeball = doAddPokeball(player:getId(), name, level, initialBoost, ballKey, true, delayMessage + 4000, teraType)
			if not addPokeball then
				print("ERROR! Player " .. player:getName() .. " lost pokemon " .. name .. "! addPokeball false")
			end
			addEvent(doPlayerSendTextMessage, delayMessage + 2000, player:getId(), MESSAGE_EVENT_ADVANCE, "Since you are at maximum capacity, your ball was sent to CP.")
		end
		
		
		--local playerLevel = player:getLevel()
		--local maxExp = getNeededExp(playerLevel + 2) - getNeededExp(playerLevel)
		--local maxExpShiny = getNeededExp(playerLevel + 5) - getNeededExp(playerLevel)

		--local givenExp = monsterType:getExperience() * configManager.getNumber(configKeys.RATE_EXPERIENCE)
		--if not msgcontains(name, 'Shiny') and player:getStorageValue(storageCatch) == -1 then
		--	givenExp = givenExp * multiplierExpFirstNormal
		--	if givenExp > maxExp then
		--		givenExp = maxExp
		--	end
		--	addEvent(doPlayerSendTextMessage, delayMessage + 1000, player:getId(), MESSAGE_EVENT_ADVANCE, "You got a bonus exp for your first catch of " .. name .. "!")
		--else
		--	givenExp = givenExp * multiplierExpNormal
		--	if givenExp > maxExp then
		--		givenExp = maxExp
		--	end

		--end

		if player:getStorageValue(storageCatch) == -1 then
			player:setStorageValue(storageCatch, 1)
		else
			player:setStorageValue(storageCatch, player:getStorageValue(storageCatch) + 1)
		end

		--addEvent(doPlayerAddExperience, delayMessage, player:getId(), givenExp)
		addEvent(doSendMagicEffect, delay, toPosition, balls[ballKey].effectSucceed)
		addEvent(doPlayerSendTextMessage, delayMessage, player:getId(), MESSAGE_EVENT_ADVANCE, "Congratulations! You have caught a " .. name .. "!")
		addEvent(doPlayerSendEffect, delayMessage, player:getId(), 297)
	else -- missed		
		addEvent(doSendMagicEffect, delay, toPosition, balls[ballKey].effectFail)
		addEvent(doPlayerSendEffect, delayMessage, player:getId(), 286)
		return true
	end	

	return true
end
