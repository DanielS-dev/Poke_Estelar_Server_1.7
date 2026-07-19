local OPCODE_LANGUAGE = 1
local OPCODE_POKEMON_BAR = POKEMON_BAR_OPCODE or 81

function onExtendedOpcode(player, opcode, buffer)
	if opcode == OPCODE_LANGUAGE then
		if buffer == 'en' or buffer == 'pt' then
			-- example, setting player language, because otclient is multi-language...
			-- player:setStorageValue(SOME_STORAGE_ID, SOME_VALUE)
		end
	elseif opcode == OPCODE_POKEMON_BAR then
		if buffer == 'request' then
			player:updatePokemonBar()
		end
	else
		-- other opcodes can be ignored, and the server will just work fine...
	end
end
