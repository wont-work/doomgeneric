local doom = require 'doom'

local x, y = 0, 0;
local doomW = doom.get_width()
local doomH = doom.get_height()
local imageData = love.image.newImageData(doomW, doomH, "rgba8")
local image = love.graphics.newImage(imageData)

doom.on_quit(function()
    love.event.quit()
end)

function love.load()
    local _, _, w, h = love.window.getSafeArea()
    love.resize(w, h)

    local path = love.filesystem.getSource()
    print(path)
    doom.start(path .. "/doom1.wad")
end

function love.draw()
    if not doom.get_want_redraw() then return end

    for px = 0, doomW - 1 do
        for py = 0, doomH - 1 do
            local r, g, b = doom.get_pixel_at(px, py)
            imageData:setPixel(px, py, r, g, b, 1)
        end
    end

    image:replacePixels(imageData)
    love.graphics.draw(image, x, y)
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

function love.resize(winW, winH)
    x = (winW - doomW) / 2
    y = (winH - doomH) / 2
end