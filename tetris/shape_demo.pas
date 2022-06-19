program ShapeDemo; { shape_demo.pas }
uses crt, shape, field;
var
    f: TetrisField;
    buf: ShapeBuffer;
    s: ShapePtr;
    SaveTextAttr: integer;
    ok: boolean;
begin
    SaveTextAttr := TextAttr;
    randomize;
    clrscr;
    delay(1000);
    FieldCheckTerminalWindow;
    FieldDrawBounds;
    FieldInit(f);
    delay(2000);
    ShapeReadFromFile('figure1', buf[1]);
    ShapeReadFromFile('figure2', buf[2]);
    ShapeReadFromFile('figure3', buf[3]);
    ShapeReadFromFile('figure4', buf[4]);
    ShapeSpawn(buf, s, f, ok);
    delay(1000);
    while not ShapeIsOnSurface(s^, f) do
    begin
        ShapeMove(MoveDown, s^, f, ok);
        delay(300)
    end;
    ShapeRelease(s^, f);
    delay(2000);
    ShapeSpawn(buf, s, f, ok);
    delay(1000);
    ShapeMove(MoveDown, s^, f, ok);
    delay(500);
    ShapeRotate(s^, f, ok);
    delay(500);
    ShapeRotate(s^, f, ok);
    delay(500);
    ShapeRotate(s^, f, ok);
    delay(500);
    ShapeRotate(s^, f, ok);
    delay(500);
    ShapeRotate(s^, f, ok);
    while not ShapeIsOnSurface(s^, f) do
    begin
        ShapeMove(MoveDown, s^, f, ok);
        delay(300)
    end;
    ShapeRelease(s^, f);
    delay(2000);
    ShapeSpawn(buf, s, f, ok);
    delay(1000);
    FieldClear(f);
    delay(1000);
    FieldDeinit(f);
    TextAttr := SaveTextAttr
end.
