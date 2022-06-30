unit human; { human.pp }
interface
uses field;
type
    HumanState = (standing, crouching);
    TetrisHuman = record
        position: FieldPosition;
        state: HumanState;
        dx, dy: integer;
        IsAlive: boolean;
        IsDrawn: boolean;
    end;

procedure HumanSpawn(var human: TetrisHuman; var f: TetrisField);
procedure HumanMove(var human: TetrisHuman; var f: TetrisField);
procedure HumanDecideIfDead(var human: TetrisHuman; var f: TetrisField);

implementation
uses crt;
const
    TorsoLine1 = ' o';
    TorsoLine2 = ' -<';
    TorsoLineRev2 = '>- ';
    LegsLine1 = ' |';
    LegsLine2 = '/ >';
    LegsLineRev2 = '< \';
    CrouchLine1 = ' -o';
    CrouchLineRev1 = 'o- ';
    CrouchLine2 = '>^<';

procedure DrawTorso(var human: TetrisHuman; var f: TetrisField);
var
    lines: FieldLines;
    p: FieldPosition;
begin
    if human.dx = 1 then
        lines[1] := TorsoLine2
    else
        lines[1] := TorsoLineRev2;
    lines[2] := TorsoLine1;
    p := human.position;
    FieldModifyPosition(0, 1, p);
    FieldDrawSquareCustom(p, lines, f)
end;

procedure DrawLegs(var human: TetrisHuman; var f: TetrisField);
var
    lines: FieldLines;
begin
    if human.dx = 1 then
        lines[1] := LegsLine2
    else
        lines[1] := LegsLineRev2;
    lines[2] := LegsLine1;
    FieldDrawSquareCustom(human.position, lines, f)
end;

procedure DrawCrouch(var human: TetrisHuman; var f: TetrisField);
var
    lines: FieldLines;
begin
    lines[1] := CrouchLine2;
    if human.dx = 1 then
        lines[2] := CrouchLine1
    else
        lines[2] := CrouchLineRev1;
    FieldDrawSquareCustom(human.position, lines, f)
end;

procedure HumanDraw(var human: TetrisHuman; var f: TetrisField);
begin
    if not human.IsAlive then
        exit;
    TextColor(white);
    human.IsDrawn := true;
    if human.state = standing then
    begin
        DrawTorso(human, f);
        DrawLegs(human, f)
    end
    else
        DrawCrouch(human, f)
end;

procedure HumanHide(var human: TetrisHuman; var f: TetrisField);
var
    p: FieldPosition;
    s, UpS: FieldSquare;
begin
    FieldGetSquare(human.position, f, s);
    human.IsDrawn := false;
    if human.state = standing then
    begin
        p := human.position;
        if not s.IsFilled then 
            FieldHideSquareCustom(p, f);
        FieldModifyPosition(0, 1, p);
        FieldGetSquare(p, f, UpS);
        if not UpS.IsFilled then
            FieldHideSquareCustom(p, f)
    end
    else if not s.IsFilled then 
        FieldHideSquareCustom(human.position, f)
end;

procedure HumanSpawn(var human: TetrisHuman; var f: TetrisField);
begin
    human.IsAlive := true;
    human.dx := 1;
    human.dy := 0;
    human.state := standing;
    FieldSetPosition(1, 1, human.position);
    HumanDraw(human, f)
end;

procedure HumanSetDirection(dx: integer; var human: TetrisHuman);
begin
    if dx < 0 then
        human.dx := -1
    else if dx > 0 then
        human.dx := 1
    else
        human.dx := 0
end;

procedure DecideState(var human: TetrisHuman; var f: TetrisField);
var
    p: FieldPosition;
    s, UpS: FieldSquare;
begin
    p := human.position;
    FieldGetSquare(p, f, s);
    if s.IsFilled then
        human.IsAlive := false
    else
    begin
        FieldModifyPosition(0, 1, p);
        FieldGetSquare(p, f, UpS);
        if UpS.IsFilled then
            human.state := crouching
        else
            human.state := standing
    end
end;

function PositionIsFilled(p: FieldPosition; var f: TetrisField): boolean;
var
    s: FieldSquare;
begin
    PositionIsFilled := not FieldPointIsInBounds(p);
    if PositionIsFilled then
        exit;
    FieldGetSquare(p, f, s);
    PositionIsFilled := s.IsFilled
end;

procedure HandleOneSide(var p: FieldPosition; 
    var human: TetrisHuman; var f: TetrisField; var ok: boolean);
begin
    ok := true;
    if not PositionIsFilled(p, f) then
    begin
        repeat
            FieldModifyPosition(0, -1, p);
            human.dy := human.dy - 1
        until PositionIsFilled(p, f);
        human.dy := human.dy + 1;
        exit
    end;
    FieldModifyPosition(0, 1, p);
    if not PositionIsFilled(p, f) then
    begin
        human.dy := 1;
        exit
    end;
    ok := false
end;

procedure DecideDirection(var human: TetrisHuman; var f: TetrisField);
var
    p: FieldPosition;
    ok: boolean;
begin
    p := human.position;
    FieldModifyPosition(human.dx, 0, p);
    HandleOneSide(p, human, f, ok);
    if ok then
        exit;
    human.dx := -human.dx;
    FieldModifyPosition(2 * human.dx, -1, p);
    HandleOneSide(p, human, f, ok);
    if ok then
        exit;
    human.dx := 0
end;

procedure HumanMove(var human: TetrisHuman; var f: TetrisField);
begin
    HumanHide(human, f);
    DecideDirection(human, f);
    FieldModifyPosition(human.dx, human.dy, human.position);
    human.dy := 0
end;

procedure HumanDecideIfDead(var human: TetrisHuman; var f: TetrisField);
begin
    DecideState(human, f);
    if human.IsAlive and (not human.IsDrawn) then
        HumanDraw(human, f)
    else if (not human.IsAlive) and human.IsDrawn then
        HumanHide(human, f)
end;

end.
