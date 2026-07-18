function onLogout(player)
	local usingBall = player:getUsingBall()
	if usingBall and usingBall:getCustomAttribute("isBeingUsed") == 1 then
		local ballKey = getBallKey(usingBall:getId())
		if ballKey and balls[ballKey] then
			doRemoveSummon(player:getId(), balls[ballKey].effectRelease, false, true, balls[ballKey].missile)
		end
	end

	local playerId = player:getId()
	if nextUseStaminaTime[playerId] then
		nextUseStaminaTime[playerId] = nil
	end
	return true
end
