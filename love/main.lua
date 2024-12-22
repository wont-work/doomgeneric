local doom = require 'doom'

local canvas;

local band = bit.band;
local rshift = bit.rshift;

local w, h;

function love.load()
    w = doom.get_width()
    h = doom.get_height()

    print("resolution",w,h)

    canvas = love.image.newImageData(w + 1, h + 1, "rgba8")
    doom.start()
end

function love.draw()
    if not doom.get_want_redraw() then return end

    for x = 0, w do
        for y = 0, h do
            local bgra = doom.get_pixel_at(x, y)
            local b = band(bgra, 0xff) / 255
            local g = band(rshift(bgra, 8), 0xff) / 255
            local r = band(rshift(bgra, 16), 0xff) / 255
            --local a = rshift(bgra, 24) / 255
            canvas:setPixel(x, y, r, g, b, 1)
        end
    end

    love.graphics.clear()
    local image = love.graphics.newImage(canvas)
    love.graphics.draw(image, 0, 0)
end

function love.update(dt)
    doom.tick()
end
