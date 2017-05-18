function task_main()
    local value = 0
    cor = coroutine.create(generator)
    repeat
        value = -2
        t, value = coroutine.resume(cor)
    until value < 0
end

function generator()
    for i = 0, 1000, 1 do
        for i = 65, 90, 1 do
            coroutine.yield(i)
        end
    end
    coroutine.yield(-1)
end
