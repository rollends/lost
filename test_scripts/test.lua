function task_main()
    local value = 0
    local t = False
    cor = coroutine.create(generator)
    repeat
        value = -2
        t, value = coroutine.resume(cor)
    until value < 0
    worker.new_job(
        function ()
            local value = 0
            local t = False
            for i = 1, 100, 1 do
                for i = 65, 90, 1 do end
            end
        end,
        1
    )
end

function generator()
    for i = 1, 100, 1 do
    for i = 65, 90, 1 do
        coroutine.yield(i)
    end
    end
    coroutine.yield(-1)
end
