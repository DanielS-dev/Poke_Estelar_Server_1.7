POKEMON_BAR_OPCODE = 81

local pokemonBarUpdateEvents = {}

local function sanitizePokemonBarText(value)
    if type(value) ~= "string" then
        return ""
    end

    value = value:gsub("[\r\n\t]+", " ")
    value = value:gsub("%s+", " ")
    return value:trim()
end

local function isPokemonBarBall(item)
    return item and item:getCustomAttribute("pokeName") ~= nil
end

local function appendPokemonBarItems(container, entries)
    if not container then
        return false
    end

    for slot = 0, container:getSize() - 1 do
        local item = container:getItem(slot)
        if item then
            if isPokemonBarBall(item) then
                local health = tonumber(item:getCustomAttribute("pokeHealth")) or 0
                local maxHealth = tonumber(item:getCustomAttribute("pokeMaxHealth")) or 0
                local isDead = tonumber(item:getCustomAttribute("pokeIsDead")) or 0
                if isDead == 1 then
                    health = 0
                end

                entries[#entries + 1] = {
                    itemId = item:getId(),
                    name = sanitizePokemonBarText(item:getCustomAttribute("pokeName") or "Pokemon"),
                    level = tonumber(item:getCustomAttribute("pokeLevel")) or 1,
                    health = math.max(0, health),
                    maxHealth = math.max(0, maxHealth),
                    isDead = isDead == 1 and 1 or 0,
                }

                if #entries >= 6 then
                    return true
                end
            end

            if item:isContainer() and appendPokemonBarItems(item, entries) then
                return true
            end
        end
    end

    return false
end

function buildPokemonBarPayload(player)
    local entries = {}
    local backpack = player:getSlotItem(CONST_SLOT_BACKPACK)
    if backpack and backpack:isContainer() then
        appendPokemonBarItems(backpack, entries)
    end

    local lines = {}
    for index, entry in ipairs(entries) do
        lines[#lines + 1] = table.concat({
            index,
            entry.itemId,
            entry.name,
            entry.level,
            entry.health,
            entry.maxHealth,
            entry.isDead,
        }, "\t")
    end

    return table.concat(lines, "\n")
end

function sendPokemonBarData(player)
    if not player or not player:isPlayer() or not player:isUsingOtClient() then
        return false
    end

    return player:sendExtendedOpcode(POKEMON_BAR_OPCODE, buildPokemonBarPayload(player))
end

function updatePokemonBarForPlayer(playerOrId)
    local player = type(playerOrId) == "number" and Player(playerOrId) or playerOrId
    if not player or not player:isPlayer() then
        return false
    end

    return sendPokemonBarData(player)
end

function schedulePokemonBarUpdate(playerOrId, delay)
    local player = type(playerOrId) == "number" and Player(playerOrId) or playerOrId
    if not player or not player:isPlayer() or not player:isUsingOtClient() then
        return false
    end

    local playerId = player:getId()
    if pokemonBarUpdateEvents[playerId] then
        stopEvent(pokemonBarUpdateEvents[playerId])
    end

    pokemonBarUpdateEvents[playerId] = addEvent(function(cid)
        pokemonBarUpdateEvents[cid] = nil
        updatePokemonBarForPlayer(cid)
    end, delay or 100, playerId)

    return true
end

function Player.updatePokemonBar(self)
    return updatePokemonBarForPlayer(self)
end
