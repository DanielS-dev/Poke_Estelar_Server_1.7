local firstItems = {2050, 2382}
local MAIN_BAG_ID = 59271

function onLogin(player)
	if player:getLastLoginSaved() == 0 then
		for i = 1, #firstItems do
			player:addItem(firstItems[i], 1)
		end
		player:addItem(player:getSex() == 0 and 2651 or 2650, 1)
		local bag = player:addItem(MAIN_BAG_ID, 1, false, CONST_SLOT_BACKPACK)
		if bag then
			bag:addItem(2674, 1)
		end
	end
	return true
end
