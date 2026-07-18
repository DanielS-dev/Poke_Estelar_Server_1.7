function onUse(player, item, fromPosition, target, toPosition, isHotkey)
    -- PROTEÇÃO TFS 1.7: Se a função não existir na lib, o script pula sem dar erro
    if player.isSummonBlocked and player:isSummonBlocked() then 
        return true 
    end
    
    local exhaust = Condition(CONDITION_EXHAUST_WEAPON)
    if player.isDuelingWithNpc and player:isDuelingWithNpc() then
        exhaust:setParameter(CONDITION_PARAM_TICKS, 10000)
    else
        exhaust:setParameter(CONDITION_PARAM_TICKS, 1000)
    end
    
    if player:getCondition(CONDITION_EXHAUST_WEAPON) then
        player:sendTextMessage(MESSAGE_STATUS_SMALL, Game.getReturnMessage(RETURNVALUE_YOUAREEXHAUSTED))
        return true
    end
    
    local ballKey = getBallKey(item:getId())
    if not ballKey or not balls[ballKey] then
        return false
    end

    if item:getCustomAttribute("isBeingUsed") == 1 then
        local usingBall = player:getUsingBall()
        if usingBall ~= item then return true end
        
        doRemoveSummon(player:getId(), balls[ballKey].effectRelease, false, true, balls[ballKey].missile)
        player:addCondition(exhaust)
        
        -- ADAPTADO: Padrão TFS 1.7 Custom Attributes
        if item:getCustomAttribute("pokeName") == "Ditto" then
            item:setCustomAttribute("dittoTransform", "Ditto")
        end
    else
        if item:getTopParent() == player then
            -- ATENÇÃO: Verifique o aviso importante abaixo sobre estas 3 funções!
            if player:canReleaseSummon(item:getSummonLevel(), item:getSummonBoost(), item:getSummonOwner()) then
                local usingBall = player:getUsingBall()
                if usingBall and usingBall ~= item then
                    local usingBallKey = getBallKey(usingBall:getId())
                    if usingBallKey and balls[usingBallKey] then
                        doRemoveSummon(player:getId(), balls[usingBallKey].effectRelease, false, true, balls[usingBallKey].missile)
                    end
                end

                item:setCustomAttribute("isBeingUsed", 1) -- ADAPTADO
                addEvent(doReleaseSummon, 1, player:getId(), player:getPosition(), balls[ballKey].effectRelease, true, balls[ballKey].missile)
                player:addCondition(exhaust)
            end
        else
            -- ADAPTADO: Corrige o bug de usar a bola fora da bag
            if item:getCustomAttribute("isBeingUsed") == 1 then 
                item:setCustomAttribute("isBeingUsed", 0) -- ADAPTADO
            end
        end
    end
    return true
end
