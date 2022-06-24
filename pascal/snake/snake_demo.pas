program SnakeDemo; { snake_demo.pas }
uses crt, snake;
var
    i: integer;
    s: SnakeHead;
    buf: SnakeFoodBuffer;
    res: SnakeMoveResult;
    SaveTextAttr: integer;
begin
    SaveTextAttr := TextAttr;
    randomize;
    clrscr;
    delay(1000);
    SnakeFoodBufferInit(buf);
    SnakeSpawn(s);
    delay(500);
    for i := 1 to 7 do
    begin
        SnakeSpawnFood(s, buf);
        delay(200)
    end;
    SnakeSetDirection(left, s);
    for i := 1 to 7 do
    begin
        SnakeMove(s, buf, res);
        delay(350)
    end;
    delay(2000);
    clrscr;
    TextAttr := SaveTextAttr
end.
