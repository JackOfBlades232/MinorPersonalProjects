program FieldDemo; { field_demo.pas }
uses crt, field;
var
    i, j: integer;
    f: TetrisField;
    p: FieldPosition;
    SaveTextAttr: integer;
begin
    SaveTextAttr := TextAttr;
    clrscr;
    delay(1000);
    FieldCheckTerminalWindow;
    FieldDrawBounds;
    FieldInit(f);
    delay(2000);
    { FieldSetPosition(2, 2, p);
    FieldDrawSquare(p, red, true, f);
    delay(2000); }
    FieldSetPosition(10, 13, p);
    FieldDrawSquare(p, white, false, f);
    FieldSetPosition(5, 6, p);
    FieldDrawSquare(p, white, false, f);
    delay(2000);
    { FieldSetPosition(2, 2, p);
    FieldHideSquare(p, f); }
    for i := 1 to 4 do
        for j := 1 to FieldWidth do
        begin
            FieldSetPosition(j, i, p);
            FieldDrawSquare(p, white, false, f)
        end;
    delay(3000);
    FieldDeleteFullRows(i, f);
    delay(10000);
    FieldClear(f);
    delay(2000);
    FieldDeinit(f);
    delay(1000);
    TextAttr := SaveTextAttr
end.
