program game; { game.pas }
uses crt, snake, score, controls;
const
    FrameDuration = 3;
    InitDelayFrames = 100;
    MovementIntervalFrames = 30;
    MinFoodSpawnIntervalFrames = 250;
    MaxFoodSpawnIntervalFrames = 750;
    ScoreStep = 10;

procedure NextFrame;
begin
    delay(FrameDuration)
end;

procedure SkipFrames(n: integer);
begin
    delay(FrameDuration * n)
end;

function GetNextFoodInterval: integer;
begin
    GetNextFoodInterval := 
        random(MaxFoodSpawnIntervalFrames - MinFoodSpawnIntervalFrames + 1)
        + MinFoodSpawnIntervalFrames
end;

label
    StartGame;
var
    s: SnakeHead;
    buf: SnakeFoodBuffer;
    res: SnakeMoveResult;
    action: ControlsAction;
    SaveTextAttr, sc, FramesSinceMoved,
        FramesSinceSpawn, NextFoodInterval: integer;
begin
    SaveTextAttr := TextAttr;
    randomize;
StartGame:
    clrscr;
    SkipFrames(InitDelayFrames);
    ScoreReset(sc);
    SnakeFoodBufferInit(buf);
    SnakeSpawn(s);
    SnakeSetDirection(left, s);
    ScoreDraw(sc);
    FramesSinceMoved := 0;
    FramesSinceSpawn := 0;
    NextFoodInterval := GetNextFoodInterval;
    while true do
    begin
        ControlsGet(action);
        case action of
            PressUp:
                SnakeSetDirection(up, s);
            PressRight:
                SnakeSetDirection(right, s);
            PressDown:
                SnakeSetDirection(down, s);
            PressLeft:
                SnakeSetDirection(left, s);
            PressExit:
                break
        end;
        FramesSinceMoved := FramesSinceMoved + 1;
        FramesSinceSpawn := FramesSinceSpawn + 1;
        if FramesSinceSpawn >= NextFoodInterval then
        begin
            SnakeSpawnFood(s, buf);
            FramesSinceSpawn := 0;
            NextFoodInterval := GetNextFoodInterval
        end;
        if FramesSinceMoved >= MovementIntervalFrames then
        begin
            SnakeMove(s, buf, res);
            FramesSinceMoved := 0;
            if res = eat then
            begin
                ScoreIncrement(ScoreStep, sc);
                ScoreDraw(sc)
            end
            else if res = fail then
            begin
                SnakeDespawn(s);
                goto StartGame
            end
        end;
        NextFrame
    end;
    SnakeDespawn(s);
    clrscr;
    TextAttr := SaveTextAttr
end.
