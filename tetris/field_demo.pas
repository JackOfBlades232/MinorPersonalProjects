program FieldDemo; { field_demo.pas }
uses crt, field;
var
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
    FieldSetPosition(2, 2, p);
    FieldDrawSquare(p, red, true, f);
    delay(2000);
    FieldSetPosition(10, 13, p);
    FieldDrawSquare(p, white, false, f);
    FieldSetPosition(5, 6, p);
    FieldDrawSquare(p, white, false, f);
    delay(2000);
    FieldSetPosition(2, 2, p);
    FieldHideSquare(p, f);
    delay(1000);
    FieldClear(f);
    delay(2000);
    FieldDeinit(f);
    delay(1000);
    TextAttr := SaveTextAttr
end.
