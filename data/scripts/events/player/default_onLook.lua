local event = Event()

event.onLook = function(self, thing, position, distance, description)
	local description = "You see " .. thing:getDescription(distance)
	if thing:isItem() then
		local pokeName = thing:getCustomAttribute("pokeName")
		if pokeName then
			local pokeLevel = thing:getCustomAttribute("pokeLevel") or 1
			local pokeHealth = thing:getCustomAttribute("pokeHealth") or 0
			local pokeMaxHealth = thing:getCustomAttribute("pokeMaxHealth") or 0
			local pokeAttack = thing:getCustomAttribute("pokeArmor") or 0
			local pokeDefense = thing:getCustomAttribute("pokeDefense") or 0
			description = string.format("%s\nPokemon: %s\nLevel: %d\nHP: %d / %d\nAttack: %d\nDefense: %d", description, pokeName, pokeLevel, pokeHealth, pokeMaxHealth, pokeAttack, pokeDefense)
		end
	end

	if self:getGroup():getAccess() then
		if thing:isItem() then
			description = string.format("%s\nItem ID: %d", description, thing:getId())

			local actionId = thing:getActionId()
			if actionId ~= 0 then
				description = string.format("%s, Action ID: %d", description, actionId)
			end

			local uniqueId = thing:getAttribute(ITEM_ATTRIBUTE_UNIQUEID)
			if uniqueId > 0 and uniqueId < 65536 then
				description = string.format("%s, Unique ID: %d", description, uniqueId)
			end

			local itemType = thing:getType()

			local transformEquipId = itemType:getTransformEquipId()
			local transformDeEquipId = itemType:getTransformDeEquipId()
			if transformEquipId ~= 0 then
				description = string.format("%s\nTransforms to: %d (onEquip)", description, transformEquipId)
			elseif transformDeEquipId ~= 0 then
				description = string.format("%s\nTransforms to: %d (onDeEquip)", description, transformDeEquipId)
			end

			local decayId = itemType:getDecayId()
			if decayId ~= -1 then
				description = string.format("%s\nDecays to: %d", description, decayId)
			end
		elseif thing:isCreature() then
			local str = "%s\nHealth: %d / %d"
			if thing:isPlayer() and thing:getMaxMana() > 0 then
				str = string.format("%s, Mana: %d / %d", str, thing:getMana(), thing:getMaxMana())
			end
			description = string.format(str, description, thing:getHealth(), thing:getMaxHealth()) .. "."
		end

		local thingPosition = thing:getPosition()
		local isPlayerInventoryItem = false
		if thing:isItem() then
			local topParent = thing:getTopParent()
			isPlayerInventoryItem = topParent and topParent:isPlayer() or false
			if position and position.x == CONTAINER_POSITION then
				isPlayerInventoryItem = true
			end
		end

		if not isPlayerInventoryItem then
			description = string.format(
				"%s\nPosition: %d, %d, %d",
				description, thingPosition.x, thingPosition.y, thingPosition.z
			)
		end

		if thing:isCreature() then
			if thing:isPlayer() then
				description = string.format("%s\nGUID: %s", description, thing:getGuid())
				description = string.format("%s\nIP: %s.", description, thing:getIp())
			end
		end
	end
	return description
end

event:register()
