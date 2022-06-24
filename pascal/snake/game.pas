program game; { game.pas }
uses crt, snake, score;
const
    FrameDuration = 10;
    MovementIntervalFrames = 30;
    FoodSpawnIntervalFrames = 500;

procedure GetKey(var code: integer);
var
    c: char;
begin
    c := ReadKey;
    if c = #0 then
    begin
        c := ReadKey;
        code := -ord(c)
    end
    else
    begin
        code := ord(c)
    end
end;

var
    s: SnakeHead;
    buf: SnakeFoodBuffer;
    res: SnakeMoveResult;
    SaveTextAttr: integer;
begin
    SaveTextAttr := TextAttr;
    randomize;
    clrscr;
    SnakeFoodBufferInit(buf);
    SnakeSpawn(s);

    clrscr;
    TextAttr := SaveTextAttr
end.
