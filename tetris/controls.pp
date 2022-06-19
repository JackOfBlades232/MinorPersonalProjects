unit controls; { controls.pp }
interface

type
    ControlsAction = 
        (Idle, PressDown, PressLeft, PressRight, PressRotate);

procedure ControlsGet(var action: ControlsAction);

implementation
uses crt;
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

procedure ControlsGet(var action: ControlsAction);
var
    code: integer;
begin
    if not KeyPressed then
        action := Idle
    else
    begin
        GetKey(code);
        case code of
            97: { a }
                action := PressLeft;
            100: { d }
                action := PressRight;
            115: { s }
                action := PressDown;
            32: { space }
                action := PressRotate
        end
    end
end;

end.
