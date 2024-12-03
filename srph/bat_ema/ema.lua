-- ema.lua
local period = 150
local alpha = 2 / (period + 1)  -- smoothing factor

local samples = {}
local ema = nil

function calculate_ema(current_sample)
    table.insert(samples, current_sample)
    
    if #samples > period then
        table.remove(samples, 1)  -- keep only the last 'period' samples
    end

    if ema == nil then
        ema = current_sample
    else
        ema = (current_sample - ema) * alpha + ema
    end

    return ema
end

return {
    calculate_ema = calculate_ema
}
