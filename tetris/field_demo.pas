program FieldDemo; { field_demo.pas }
uses crt, field;
var
    f: TetrisField;
    SaveTextAttr: integer;
begin
    SaveTextAttr := TextAttr;
    clrscr;
    delay(1000);
    FieldCheckTerminalWindow;
    FieldDrawBounds;
    FieldInit(f);
    delay(2000);
    f.squares[2][2].color := red;
    FieldDrawSquare(2, 2, f);
    delay(2000);
    FieldDrawSquare(10, 13, f);
    FieldDrawSquare(5, 6, f);
    delay(2000);
    FieldHideSquare(2, 2, f);
    delay(1000);
    FieldClear(f);
    delay(2000);
    TextAttr := SaveTextAttr;
    clrscr
end.
