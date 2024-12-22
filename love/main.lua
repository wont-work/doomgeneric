local doom = require 'doom'

local band = bit.band;
local rshift = bit.rshift;

local w = doom.get_width()
local h = doom.get_height()
local canvas = love.image.newImageData(w + 1, h + 1, "rgba8")
local image = love.graphics.newImage(canvas)

function love.load()
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
            canvas:setPixel(x, y, r, g, b, 1)
        end
    end

    image:replacePixels(canvas)
    love.graphics.draw(image, 0, 0)
end

function love.update(dt)
    doom.tick()
end

function love.keypressed(key, scancode, isrepeat)
    doom.press_key(key)
end

function love.keyreleased(key)
    doom.release_key(key)
end