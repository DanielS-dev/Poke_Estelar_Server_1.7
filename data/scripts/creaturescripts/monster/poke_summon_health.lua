local creatureevent = CreatureEvent("PokeSummonHealth")

function creatureevent.onHealthChange(creature, attacker, primaryDamage, primaryType, secondaryDamage, secondaryType, origin)
	local master = creature:getMaster()
	if not master or not master:isPlayer() then
		return primaryDamage, primaryType, secondaryDamage, secondaryType
	end

	local creatureId = creature:getId()
	local playerId = master:getId()
	addEvent(function(cid, pid)
		local player = Player(pid)
		if not player then
			return
		end

		local summon = Monster(cid)
		local ball = player:getUsingBall()
		if summon and ball and ball:getCustomAttribute("summonId") == summon:getId() then
			ball:setCustomAttribute("pokeHealth", math.max(0, summon:getHealth()))
			ball:setCustomAttribute("pokeMaxHealth", summon:getMaxHealth())
		end

		schedulePokemonBarUpdate(player, 80)
	end, 1, creatureId, playerId)

	return primaryDamage, primaryType, secondaryDamage, secondaryType
end

creatureevent:register()
