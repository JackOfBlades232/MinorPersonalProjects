unit shape; { shape.pp }
interface
uses field;
const
    ShapeNumSquares = 4;
    ShapeTypeCount = 4;
type
    ShapeOrientation = (OrientNormal, OrientInverted);
    ShapeRotation = (RotUp, RotRight, RotDown, RotLeft);
    ShapeMoveDirection = (MoveDown, MoveLeft, MoveRight);
    TetrisShape = record
        color: word;
        position: FieldPosition;
        orientation: ShapeOrientation;
        rotation: ShapeRotation;
        LocalCoordinates: array [1..ShapeNumSquares] of FieldPosition;
    end;
    ShapePtr = ^TetrisShape;
    ShapeBuffer = array [1..ShapeTypeCount] of TetrisShape;

procedure ShapeReadFromFile(filename: string; var shape: TetrisShape);
function ShapePointGlobalCoordinates(index: integer;
    var shape: TetrisShape): FieldPosition;
function ShapeIsViable(var shape: TetrisShape; var f: TetrisField): boolean;
function ShapeIsOnSurface(var shape: TetrisShape; var f: TetrisField): boolean;
procedure ShapeDraw(var shape: TetrisShape; var f: TetrisField);
procedure ShapeHide(var shape: TetrisShape; var f: TetrisField);
procedure ShapeMove(dir: ShapeMoveDirection;
    var shape: TetrisShape; var f: TetrisField; var ok: boolean);
procedure ShapeRotate(var shape: TetrisShape; var f: TetrisField; var ok: boolean);
procedure ShapeSpawn(var buf: ShapeBuffer; var CurrentShapePtr: ShapePtr;
    var f: TetrisField; var ok: boolean);
procedure ShapeRelease(var shape: TetrisShape; var f: TetrisField);

implementation
uses crt;
const
	ColorCount = 13;
	RotationCount = 4;
var
	AllColors: array [1..ColorCount] of word = 
	(
		Blue, Green, Cyan,
		Red, Magenta, LightGray,
		LightBlue, LightGreen, LightCyan,
		LightRed, LightMagenta, Yellow, White
	);
	AllRotations: array [1..RotationCount] of ShapeRotation =
        (RotUp, RotRight, RotDown, RotLeft);

procedure ShapeReadFromFile(filename: string; var shape: TetrisShape);
var
    f: text;
    i, coord: integer;
begin
    {$I-}
    assign(f, filename);
    reset(f);
    if IOResult <> 0 then
    begin
        writeln(ErrOutput, 'Couldnt read shape from ', filename);
        halt(1)
    end;
    for i := 1 to ShapeNumSquares do
    begin
        read(f, coord);
        shape.LocalCoordinates[i][1] := coord;
        read(f, coord);
        shape.LocalCoordinates[i][2] := coord;
        if IOResult <> 0 then
        begin
            writeln('Incorrect shape file format: ', filename);
            halt(1)
        end
    end;
    close(f)
end;

function ShapePointGlobalCoordinates(index: integer;
    var shape: TetrisShape): FieldPosition;
var
    LocalX, LocalY: integer;
begin
    ShapePointGlobalCoordinates := shape.position;
    case shape.rotation of
        RotUp: begin
            LocalX := shape.LocalCoordinates[index][1];
            LocalY := shape.LocalCoordinates[index][2]
        end;
        RotRight: begin
            LocalX := shape.LocalCoordinates[index][2];
            LocalY := -shape.LocalCoordinates[index][1]
        end;
        RotDown: begin
            LocalX := -shape.LocalCoordinates[index][1];
            LocalY := -shape.LocalCoordinates[index][2]
        end;
        RotLeft: begin
            LocalX := -shape.LocalCoordinates[index][2];
            LocalY := shape.LocalCoordinates[index][1]
        end
    end;
    if shape.orientation = OrientInverted then
        LocalX := -LocalX;
    ShapePointGlobalCoordinates[1] := ShapePointGlobalCoordinates[1] + LocalX;
    ShapePointGlobalCoordinates[2] := ShapePointGlobalCoordinates[2] + LocalY
end;

function PositionIsNotEmptyOrInShape(p: FieldPosition; var f: TetrisField):
    boolean;
var
    s: FieldSquare;
begin
    FieldGetSquare(p, f, s);
    PositionIsNotEmptyOrInShape := (not FieldPointIsInBounds(p)) or
        (s.IsFilled and (not s.IsInFigure))
end;

function ShapeIsViable(var shape: TetrisShape; var f: TetrisField): boolean;
var
    i: integer;
    p: FieldPosition;
begin
    for i := 1 to ShapeNumSquares do
    begin
        p := ShapePointGlobalCoordinates(i, shape);
        ShapeIsViable := not PositionIsNotEmptyOrInShape(p, f);
        if not ShapeIsViable then
            exit
    end
end;

function ShapeIsOnSurface(var shape: TetrisShape; var f: TetrisField): boolean;
var
    i: integer;
    p: FieldPosition;
begin
    for i := 1 to ShapeNumSquares do
    begin
        p := ShapePointGlobalCoordinates(i, shape);
        FieldModifyPosition(0, -1, p);
        ShapeIsOnSurface := PositionIsNotEmptyOrInShape(p, f);
        if ShapeIsOnSurface then
            exit
    end
end;

procedure ShapeDraw(var shape: TetrisShape; var f: TetrisField);
var
    i: integer; 
begin
    if not ShapeIsViable(shape, f) then
        exit;
    for i := 1 to ShapeNumSquares do
        FieldDrawSquare(ShapePointGlobalCoordinates(i, shape),
            shape.color, true, f)
end;

procedure ShapeHide(var shape: TetrisShape; var f: TetrisField);
var
    i: integer; 
begin
    if not ShapeIsViable(shape, f) then
        exit;
    for i := 1 to ShapeNumSquares do
        FieldHideSquare(ShapePointGlobalCoordinates(i, shape), f)
end;

procedure ShapeMove(dir: ShapeMoveDirection;
    var shape: TetrisShape; var f: TetrisField; var ok: boolean);
var
    PrevPosition: FieldPosition;
begin
    ok := true;
    PrevPosition := shape.position;
    ShapeHide(shape, f);
    case dir of
        MoveDown:
            FieldModifyPosition(0, -1, shape.position);
        MoveRight:
            FieldModifyPosition(1, 0, shape.position);
        MoveLeft:
            FieldModifyPosition(-1, 0, shape.position)
    end;
    if not ShapeIsViable(shape, f) then
    begin
        ok := false;
        shape.position := PrevPosition
    end;
    ShapeDraw(shape, f)
end;

procedure GoToNextRotation(var r: ShapeRotation);
begin
    if r = RotLeft then
        r := RotUp
    else
        r := succ(r)
end;

procedure ShapeRotate(var shape: TetrisShape; var f: TetrisField; var ok: boolean);
var
    PrevRotation: ShapeRotation;
begin
    ok := true;
    PrevRotation := shape.rotation;
    ShapeHide(shape, f);
    GoToNextRotation(shape.rotation);
    if not ShapeIsViable(shape, f) then
    begin
        ok := false;
        shape.rotation := PrevRotation
    end;
    ShapeDraw(shape, f)
end;

procedure ResetShape(var shape: TetrisShape);
begin
    shape.position[1] := FieldWidth div 2;
    shape.position[2] := FieldHeight;
    shape.rotation := AllRotations[random(RotationCount) + 1];
    if random(2) = 0 then
        shape.orientation := OrientNormal
    else
        shape.orientation := OrientInverted;
    shape.color := AllColors[random(ColorCount) + 1] 
end;

procedure ShapeSpawn(var buf: ShapeBuffer; var CurrentShapePtr: ShapePtr;
    var f: TetrisField; var ok: boolean);
begin
    ok := true;
    CurrentShapePtr := @buf[random(ShapeTypeCount) + 1];
    ResetShape(CurrentShapePtr^);
    while (not ShapeIsViable(CurrentShapePtr^, f))
        and FieldPointIsInBounds(CurrentShapePtr^.position) do
        FieldModifyPosition(0, -1, CurrentShapePtr^.position);
    if not FieldPointIsInBounds(CurrentShapePtr^.position) then
        ok := false
    else
        ShapeDraw(CurrentShapePtr^, f)
end;

procedure ShapeRelease(var shape: TetrisShape; var f: TetrisField);
var
    i: integer; 
begin
    if not ShapeIsViable(shape, f) then
        exit;
    for i := 1 to ShapeNumSquares do
        FieldReleaseSquare(ShapePointGlobalCoordinates(i, shape), f)
end;

end.
