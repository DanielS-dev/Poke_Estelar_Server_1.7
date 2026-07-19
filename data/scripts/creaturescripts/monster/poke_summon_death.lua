local creatureevent = CreatureEvent("PokeSummonDeath")

function creatureevent.onDeath(creature, corpse, killer, mostDamageKiller, lastHitUnjustified, mostDamageUnjustified)
	local master = creature:getMaster()
	if not master or not master:isPlayer() then
		return true
	end

	local ball = master:getUsingBall()
	if not ball then
		return true
	end

	ball:setCustomAttribute("pokeHealth", 0)
	ball:setCustomAttribute("pokeMaxHealth", creature:getMaxHealth())
	ball:setCustomAttribute("pokeIsDead", 1)
	ball:setCustomAttribute("isBeingUsed", 0)
	ball:removeAttribute("summonId")
	schedulePokemonBarUpdate(master, 50)
	return true
end

creatureevent:register()
